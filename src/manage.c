/* pal
 *
 * Copyright (C) 2004--2006, Scott Kuhl
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "main.h"

#include <readline/readline.h>
#include <ncurses.h>
#include <term.h>
#include <signal.h>
#include <sys/ioctl.h> /* get # columns for terminal */

#include <unistd.h>
#include <time.h>

#include "output.h"
#include "event.h"
#include "colorize.h"
#include "edit.h"
#include "add.h"
#include "del.h"
#include "rl.h"
#include "search.h"
#include "manage.h"

static int selected_event = -1;
static int events_on_day = 0;

static struct tm* selected_day = NULL;

/* Currently active window for g_print to output to */
WINDOW *pal_curwin = NULL;

void pal_manage(void);

/* Redisplays the main calendar + event list screen.  This function
 * does not clear the first two lines of the screen before
 * drawing---so it will not clear any prompts if they exist.
 *
 * If you wish to clear the entire screen before drawing, call
 * pal_manage_refresh_at() first.
 */
static void pal_manage_refresh_at(void)
{
	char *date_text;
	int linecount = 0;
	struct tm *date;
	int saved_cols;

	gboolean finished_printing = FALSE;

	/* Don't touch the first two lines---they are reserved for
	 * prompts! */
	move(2,0);

	pal_output_cal(settings->cal_lines, *selected_day);
	g_print("\n");

	date = selected_day;

	/* If we are viewing a event, screen is split in two to display
	 * details. Set this so pal_output_wrap works properly */
	saved_cols = settings->term_cols;
	if( selected_event >= 0 )
	  settings->term_cols /= 2;

	/* Note: Because the output of g_print is redirected through
	 * ncurses, we don't have to worry about unexpected scrolling */

	while(!finished_printing) {
		int thisdaycount = 0;
		bool isselectedday = ( difftime(mktime(date),mktime(selected_day))/(24*3600) == 0);
		thisdaycount = pal_get_event_count(date);

		if(linecount > 6)
			settings->term_cols = saved_cols;

		if( isselectedday ) {
			events_on_day = thisdaycount;

		if(selected_event >= events_on_day)
			selected_event = events_on_day - 1;
		}

		if( thisdaycount > 0 || isselectedday ) {
			int x,y;
			getyx( stdscr, y, x );


			linecount += pal_output_date(date, TRUE, isselectedday ? selected_event : -1 );

			/* if the last thing we printed fell off the screen, erase it */
			if(linecount + settings->cal_lines+3 > settings->term_rows-1) {
				move(y,x);
				clrtobot();
				finished_printing = TRUE; /* break out of loop */
			}
		}

	  date->tm_mday += 1;
	  mktime(date);
	}
	free(date);


	/* Draw the event information box if an event is selected */
	if( selected_event >= 0 && events_on_day > 0 )
	{
		GList *events = get_events(selected_day);
		char *ptr = NULL;
		PalEvent *curevent = NULL;


	/* If we are viewing a event, screen is split in two to display
	 * details. Set this so pal_output_wrap works properly */
	if( selected_event >= 0 )
		settings->term_cols /= 2;

	pal_curwin = subwin(stdscr, 5, saved_cols/2,
									settings->cal_lines + 4, saved_cols/2);

		curevent = g_list_nth_data(events, (selected_event >= 0) ? selected_event : 0 );
		g_list_free(events);

		wmove(pal_curwin, 0, 0);
		pal_output_fg(BRIGHT, GREEN, "Event Type: " );
		ptr = curevent->eventtype->get_descr( selected_day );
		pal_output_wrap( ptr, 12, 5 );
		free(ptr);

		pal_output_fg(BRIGHT, GREEN, "Skip count: " );
		g_print( "%d\n", curevent->period_count );

		pal_output_fg(BRIGHT, GREEN, "Start date: " );
		if( curevent->start_date ) {
			date_text = asctime(curevent->start_date);
		} else {
			date_text = (char[5]){0};
			sprintf( date_text, "None" );
		}
		g_print( "%s\n", date_text );

		pal_output_fg(BRIGHT, GREEN, "End date:	" );
		if( curevent->end_date ) {
			date_text = asctime(curevent->end_date);
		} else {
			date_text = (char[5]){0};
			sprintf( date_text, "None" );
		}
		g_print( "%s\n", date_text );

		pal_output_fg(BRIGHT, GREEN, "Key:		" );
		g_print( "%s\n", curevent->key );

		delwin( pal_curwin );
		pal_curwin = stdscr;
	}

	settings->term_cols = saved_cols;

	refresh();
}

/* pal_manage_refresh clears the entire screen and redraws the
 * calendar and list of events.  */
static void pal_manage_refresh(void)
{
	/* clear the first two prompt lines */
	move(0,0);
	g_print("\n\n");

	pal_manage_refresh_at();
}


static void pal_manage_finish(int sig)
{
	char hostname[128];

	endwin();

	/* Clear the terminal (code from ncurses /usr/bin/clear program).
	 * If we don't do this sometimes the terminal we return to gets
	 * screwed up.	This usually occurs when the terminal was made
	 * larger while pal was running.  This isn't our fault---the same
	 * thing happens on other apps like "emacs -nw" too.  */
	setupterm((char *) 0, STDOUT_FILENO, (int *) 0);
	tputs(clear_screen, lines > 0 ? lines : 1, putchar);

	/* set the xterm title to something reasonable */
	if(gethostname(hostname, 128) == 0)
	colorize_xterm_title(hostname);

	exit(0);
}




/* Normally, ncurses has a window resize handler that causes getch()
to return KEY_RESIZE.  However, using readline+ncurses somehow screws
it up.	So, we implement here what ncurses does by default. */

static void pal_manage_resize(int sig)
{

#ifndef __CYGWIN__ /* figure out the terminal width if possible */
	{
	struct winsize wsz;
	if(ioctl(0, TIOCGWINSZ, &wsz) != -1)
	{
		settings->term_cols = wsz.ws_col;
		settings->term_rows = wsz.ws_row;
	}
	}
#endif

	/* Tell curses that the screen size has been resized */
	if(is_term_resized(settings->term_rows, settings->term_cols)) {
		resizeterm(settings->term_rows, settings->term_cols);
	}

	/* Let the key handler deal with redrawing the screen.	This means
	 * that if we are waiting for input from readline, we won't
	 * refresh the screen until after readline is finished. */
	ungetch(KEY_RESIZE);
	return;
}



static gboolean isearch_direction;

/* Refresh function for the isearch */
static int
pal_manage_isearch_refresh(void)
{
	struct tm *searchdate;
	int searchselect = selected_event;

	searchdate = selected_day;

	move(0,0);
	clrtoeol();

	char *prettydate = asctime(selected_day);

	pal_output_fg(BRIGHT, GREEN,
		  "%s Isearch starting from %s",
		  isearch_direction ? "Forward" : "Backward", prettydate);
	g_print(" ");


	/* Only search if there is text to search for */
	if( *rl_line_buffer && !pal_search_isearch_event( &searchdate, &searchselect,
							  rl_line_buffer, isearch_direction ) ) {
		pal_output_fg(BRIGHT, RED, "No matches found!");
		rl_ding();
	} else {
		free(selected_day);
		selected_day = searchdate;
		selected_event = searchselect;
		pal_manage_refresh_at();
	}

	pal_rl_ncurses_hack();	/* Update cursor */
	return 0; //exit success
}

/* Does a basic interactive search in the given direction. This is different
 * from the full search handled in search.c */
static void pal_manage_isearch(gboolean forward)
{
	char *searchstring = NULL;
	struct tm *searchdate;
	int searchselect = selected_event;

	searchdate = selected_day;

	isearch_direction = forward;

	/* Clear the first two lines */
	move(0, 0);
	clrtoeol();

	move(1, 0);
	clrtoeol();


	/* Temporarily set the refresh function to update while typing */
	rl_redisplay_function = pal_manage_isearch_refresh;
	searchstring = pal_rl_get_raw_line("Isearch: ", 1, 0);
	rl_redisplay_function = pal_rl_ncurses_hack;

	/* If the user typed something and we can find event, set selected event */
	if( *searchstring ) {
		if( pal_search_isearch_event( &searchdate, &searchselect, rl_line_buffer, isearch_direction ) ) {
			memcpy( selected_day, searchdate, sizeof(struct tm) );
			selected_event = searchselect;
		}
	}

	pal_manage_refresh();
	free(searchdate);
}

/* Scans for the next event in the given direction */
static void
pal_manage_scan_for_event( struct tm **date, int *eventnum, int dir )
{
  /* Note, the way this code is written handles the case where eventnum is
   * -1. In that case it places on the first event following this date */
  if( dir > 0 ) {
	int thisdaycount;
	int count = 0;

	(*eventnum)++;
	if( *eventnum < events_on_day )
	  return;

	*eventnum = 0;
	while( count < 60 ) { /* No more than two months */
	  (*date)->tm_mday += 1;
	  mktime(*date);
	  thisdaycount = pal_get_event_count(*date);

	  if( thisdaycount > 0 )
		return;

	  count++;
	}
	rl_ding();
	return;
  } else {
	int thisdaycount;
	int count = 0;

	(*eventnum)--;
	if( *eventnum >= 0 )
	  return;

	while( count < 60 ) { /* No more than two months */
	  (*date)->tm_mday -= 1;
	  thisdaycount = pal_get_event_count(*date);

		if( thisdaycount > 0 ) {
			*eventnum = thisdaycount - 1;
		return;
	  }

	  count++;
	}
	rl_ding();
	return;
  }
}



void pal_manage(void)
{
	time_t currenttime = time(NULL);
	memcpy(selected_day, &today, sizeof(struct tm));

	colorize_xterm_title("pal calendar");

	/* set up readline */
	rl_initialize();	 /* Initialise readline so we can fiddle stuff */
	rl_already_prompted = 1;
	rl_redisplay_function = pal_rl_ncurses_hack;
	rl_pre_input_hook = pal_rl_ncurses_hack;


	/* initialize curses */
	(void) signal(SIGINT, pal_manage_finish);	   /* arrange interrupts to terminate */

	(void) initscr();	   /* initialize the curses library */
	keypad(stdscr, TRUE);  /* enable keyboard mapping */
	(void) nonl();		   /* tell curses not to do NL->CR/NL on output */
	(void) cbreak();	   /* take input chars one at a time, no wait for \n */
	(void) halfdelay(1);   /* always return from getch() within a tenth of a second */
	(void) noecho();	   /* echo input - in color */

	signal(SIGINT, pal_manage_finish);	 /* setup function to call on Ctrl+C */
	signal(SIGWINCH, pal_manage_resize); /* setup function to call on term resize */


	/* adjust some settings if necessary */
	getmaxyx(stdscr,settings->term_rows,settings->term_cols);
	settings->cal_lines = 5;
	settings->curses = TRUE;
	pal_curwin = stdscr;

	if(has_colors()) {
		start_color();
		init_pair(COLOR_BLACK,	 COLOR_BLACK,	COLOR_BLACK);
		init_pair(COLOR_GREEN,	 COLOR_GREEN,	COLOR_BLACK);
		init_pair(COLOR_RED,	 COLOR_RED,		COLOR_BLACK);
		init_pair(COLOR_CYAN,	 COLOR_CYAN,	COLOR_BLACK);
		init_pair(COLOR_WHITE,	 COLOR_WHITE,	COLOR_BLACK);
		init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(COLOR_BLUE,	 COLOR_BLUE,	COLOR_BLACK);
		init_pair(COLOR_YELLOW,  COLOR_YELLOW,	COLOR_BLACK);
	} else {
		set_colorize(-2);
	}


	pal_manage_refresh();

	move(0, 0);
	pal_output_fg(BRIGHT, GREEN, "pal %s", PAL_VERSION);
	g_print(" - ");
	g_print("Press 'h' for help, 'q' to quit.");

	while(1) {
		int c;
		while((c = getch()) == ERR);


		switch(c) {
			case KEY_RESIZE:
				pal_manage_refresh();
				break;
			case 'q':
			case 'Q':
				pal_manage_finish(0);
			return;

			case KEY_LEFT:
				selected_day->tm_mday -= 1;
				pal_manage_refresh();
				break;
			case KEY_RIGHT:
			case ' ':
			selected_day->tm_mday += 1;
			pal_manage_refresh();
			break;
			case KEY_UP:
			if(selected_event == -1) {
				selected_day->tm_mday -= 7;
				mktime(selected_day);
			} else
				pal_manage_scan_for_event( &selected_day, &selected_event, -1 );
			pal_manage_refresh();
			break;
			case KEY_DOWN:
			if(selected_event == -1) {
				selected_day->tm_mday += 7;
				mktime(selected_day);
			} else {
				pal_manage_scan_for_event( &selected_day, &selected_event, 1 );
			}
			pal_manage_refresh();
			break;

			case '\t':
			case '\r':
			case KEY_ENTER:

			if(selected_event != -1)
				selected_event = -1;

			else if(events_on_day != 0)
				selected_event = 0;

			else /* If no event on current day, scan till we find one */
				pal_manage_scan_for_event( &selected_day, &selected_event, 1 );


			pal_manage_refresh();
			break;

			case 't': /* today */
			case 'T':
			{
			selected_day = localtime(&currenttime);
			pal_manage_refresh();
			break;
			}
			case 'g': /* goto date */
			case 'G':
				{
				char* str = pal_rl_get_raw_line("Goto date: ", 0, 0);

				if(strlen(str) > 0) {
					struct tm* new_date;
					get_query_date(new_date, str, FALSE);

					if(new_date == NULL) {
						move(0, 0);
						clrtoeol();
						colorize_fg(BRIGHT, RED);
						g_print("%s is an invalid date!", str);
						colorize_reset();
					} else {
						free(selected_day);
						selected_day = new_date;
						pal_manage_refresh();
					}
				} else {
					move(0,0);
					clrtoeol();
				}

				break;
				}

			case 'e': /* edit description */
			case 'E':
				/* Shortcut for days with one event */
				if(selected_event == -1 && events_on_day == 1 )
					selected_event = 0;
			if(selected_event != -1)
			{
				PalEvent* e = pal_output_event_num(selected_day, selected_event+1);
				if(e != NULL) {
					if(e->global) {
						move(0, 0);
						clrtoeol();
						pal_output_fg(BRIGHT, RED, "Can't edit global event!");
					} else {
//						char* new_text = pal_edit_desc(e->text);
						char* new_text = pal_rl_get_line_default("New description: ", 0, 0, e->text);

						pal_del_write_file(e);
						pal_add_write_file(e->file_name, e->date_string, new_text);
						/* need to check for error here! */

						free(new_text);
						pal_main_reload();

						pal_manage_refresh();
					}
				}
			} else {
				move(0,0);
				clrtoeol();
				pal_output_fg(BRIGHT, RED, "No event selected.");
			}
			break;

			case 'v':
			case 'V':
			if(selected_event != -1)
			{
				pal_edit_event(pal_output_event_num(selected_day, selected_event+1), selected_day);
				pal_manage_refresh();
			}
			break;

			case 'd': /* edit event date */
			case 'D':
			break;

			case 'a': /* add event */
			case 'A':
				pal_add_event( selected_day );
				break;

			case KEY_DC: /* delete key - kill event */
			if(selected_event != -1)
			{
				PalEvent* e = pal_output_event_num(selected_day, selected_event+1);
				if(e != NULL)
				{
				move(0, 0);
				if(e->global)
					pal_output_fg(BRIGHT, RED, "Can't delete global event!");
				else
				{
								if(pal_rl_get_y_n("Are you sure you want to delete this event? [y/n]: "))
					{
					selected_event = -1;
									pal_del_write_file(e);
					}

					pal_main_reload();
					pal_manage_refresh();
				}
				}
			}
			break;

			case 'r': /* reminder */
			case 'R': break;

			case 's': /* search */
			case 'S': break;

			case '/': /* forward i-search */
				pal_manage_isearch( TRUE );
				break;
			case '?': /* backward i-search */
				pal_manage_isearch( FALSE );
				break;

			case 'h': /* help */
			case 'H':
			clear();
			move(0,0);
			pal_output_fg(BRIGHT, GREEN, "pal %s\n\n", PAL_VERSION);

			pal_output_fg(BRIGHT, GREEN, "LeftArrow");
			g_print(" - %s\n", "Back one day");

			pal_output_fg(BRIGHT, GREEN, "RightArrow");
			g_print(" - %s\n", "Forward one day");

			pal_output_fg(BRIGHT, GREEN, "UpArrow");
			g_print(" - %s\n", "Back one week or event (if in event selection mode)");

			pal_output_fg(BRIGHT, GREEN, "DownArrow");
			g_print(" - %s\n", "Forward one week or event (if in event selection mode)");

			pal_output_fg(BRIGHT, GREEN, "h");
			g_print(" - %s\n", "This help screen.");

			pal_output_fg(BRIGHT, GREEN, "q");
			g_print(" - %s\n", "Quit");

			pal_output_fg(BRIGHT, GREEN, "Tab, Enter");
			g_print(" - %s\n", "Toggle event selection");

			pal_output_fg(BRIGHT, GREEN, "g");
			g_print(" - %s\n", "Jump to a specific date.");

			pal_output_fg(BRIGHT, GREEN, "t");
			g_print(" - %s\n", "Jump to today.");

			pal_output_fg(BRIGHT, GREEN, "e");
			g_print(" - %s\n", "Edit description of the selected event.");

			pal_output_fg(BRIGHT, GREEN, "a");
			g_print(" - %s\n", "Add an event to the selected date.");

			pal_output_fg(BRIGHT, GREEN, "/, ?");
			g_print(" - %s\n", "Forward/reverse i-search");

			pal_output_fg(BRIGHT, GREEN, "Delete");
			g_print(" - %s\n", "Delete selected event.");

			g_print("\n");

			pal_output_fg(BRIGHT, RED, "UNIMPLEMENTED:\n");
			g_print("d - %s\n", "Edit date for selected event.");
			g_print("s - %s\n", "Search for event.");
			g_print("r - %s\n", "Remind me about an event.");

			g_print("\n");
			g_print("Press any key to exit help.");
			g_print("\n");

			do {
				c = getch();
			} while(c == ERR || c == KEY_RESIZE);

			pal_manage_refresh();

			break;

		}
	}
}




