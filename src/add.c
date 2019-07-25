/* pal
 *
 * Copyright (C) 2006, Scott Kuhl
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

#include <curses.h>

#include "main.h"
#include "output.h"
#include "event.h"
#include "rl.h"


/* This only works when 'number' is between 1 and 10 inclusive */
void
pal_add_suffix(int number, char* suffix, int buf_size)
{
	number = number % 10;
	switch(number) {
		case 1:  snprintf(suffix, buf_size, "%s", "1st"); return;
		case 2:  snprintf(suffix, buf_size, "%s", "2nd"); return;
		case 3:  snprintf(suffix, buf_size, "%s", "3rd"); return;
		case 4:  snprintf(suffix, buf_size, "%s", "4th"); return;
		case 5:  snprintf(suffix, buf_size, "%s", "5th"); return;
		case 6:  snprintf(suffix, buf_size, "%s", "6th"); return;
		case 7:  snprintf(suffix, buf_size, "%s", "7th"); return;
		case 8:  snprintf(suffix, buf_size, "%s", "8th"); return;
		case 9:  snprintf(suffix, buf_size, "%s", "9th"); return;
		case 10: snprintf(suffix, buf_size, "%s","10th"); return;

		default: *suffix = '\0'; return;
	}
}

static char*
pal_add_get_range(struct tm *date)
{
	pal_output_fg(BRIGHT, GREEN, "> ");
	g_print("Does the event have start and end dates? ");

	if(pal_rl_get_y_n("[y/n]: ")) {
		struct tm* d1 = NULL;
		struct tm* d2 = NULL;
		char buf[1024] = "";
		char* s = NULL;

		g_print("\n");

		int x,y;
		getyx( stdscr, y, x );

		do {
			/* set back to null for loop to work properly */
			d1 = NULL;
			d2 = NULL;

			move(y,x);
			clrtobot();
			refresh();

			while(d1 == NULL) {
				s = get_key(date);
				strcpy( buf, s );
				free(s);

				s = pal_rl_get_line_default("Start date: ", y, 0, buf);
				g_print("\n");

				d1 = get_query_date(s, TRUE);
				if( !d1 )
					rl_ding();
				free(s);
			}

			clrtobot();
			while(d2 == NULL) {
				s = pal_rl_get_raw_line("End date (blank is none): ", y+1, 0);

				if( *s == 0 ) {
					free(s);
					s = g_strdup( "30000101" );
				}

				g_print("\n");
				d2 = get_query_date(s, TRUE);
				if( !d2 ) {
					rl_ding();
				} else {
					if( difftime(mktime(d1),mktime(d2))/(24*3600) < 1) {
						pal_output_error("ERROR: End date must be after start date.\n");
						clrtobot();
						d2 = NULL;
					}
				}
				free(s);
			}
			g_print("\n");

			move(y,0);
			clrtobot();

			strftime(buf, 1024, "%a %e %b %Y", d1);
			pal_output_fg(BRIGHT, GREEN, "Start date: ");
			g_print("%s\n", buf);

			strftime(buf, 1024, "%a %e %b %Y", d2);
			pal_output_fg(BRIGHT, GREEN, "End date: ");
			g_print("%s\n", buf);

			snprintf(buf, 1024, "%s ", "Accept? [y/n]:");


		} while(!pal_rl_get_y_n(buf));

		s = g_strconcat(":", get_key(d1), ":", get_key(d2), NULL);
		return s;

	} else {
		return g_strdup("");
	}

}


/* reuturned string should be freed */
static char*
pal_add_get_recur(struct tm* date)
{
	char* selection = NULL;

	int x,y;
	getyx( stdscr, y, x );

	pal_output_fg(BRIGHT, GREEN, "> ");
	g_print("Select how often this event occurs\n");

	for( int i=0; i < PAL_NUM_EVENTTYPES; i++ ) {
		char buffer[16] = "";
		char *descr = NULL;

		descr = PalEventTypes[i].get_descr(date);
		if( !descr )   /* Not applicable */
			continue;

		sprintf( buffer, " %2d ", i );
		pal_output_attr(BRIGHT, buffer);
		g_print("- %s\n", descr);

		free(descr);
	}

	do {
		int sel;
		char selkey[MAX_KEYLEN] = "";
		char *descr = NULL;

		int promptx,prompty;
		getyx( stdscr, prompty, promptx );

		selection = pal_rl_get_line("Select type: ", prompty, 0);

		if( sscanf( selection, "%d%*s", &sel ) != 1 ) {
			rl_ding();
			continue;
		}

		if( sel < 0 || sel >= PAL_NUM_EVENTTYPES ) {
			rl_ding();
			continue;
		}

		if( PalEventTypes[sel].get_key( date, selkey ) != TRUE ) {
			rl_ding();
			continue;
		}

		descr = PalEventTypes[sel].get_descr(date);
		move( y, 0 );
		pal_output_fg(BRIGHT, GREEN, "Event type: ");
		g_print("%s\n", descr);
	clrtobot();
		free(descr);

		if( PalEventTypes[sel].period == PAL_ONCEONLY )
			return g_strdup(selkey);

		return g_strconcat(selkey, pal_add_get_range(date), NULL);
	}while(1);
}


static char*
pal_add_get_desc(void)
{
	char* desc = NULL;
	char* olddesc = NULL;
	int y,x;
	pal_output_fg(BRIGHT, GREEN, "> ");
	g_print("What is the description of the event?\n");

	getyx( stdscr, y, x );

	do {
		move(y,x);
		clrtobot();

		olddesc = desc;

		/*
		if(desc != NULL)
		{
			pal_rl_default_text = desc;
			rl_pre_input_hook = (rl_hook_func_t*) pal_rl_default_text_fn;
		}

		desc = pal_rl_get_line("Description: ", y, x);
		rl_pre_input_hook = NULL;
		*/

		desc = pal_rl_get_line_default("Description: ", y, x, olddesc);
		free(olddesc);

		g_print("\n");
	} while(!pal_rl_get_y_n("Is this description correct? [y/n]: "));

	return desc;
}


/* prompts for a file name */
static char*
pal_add_get_file(void)
{
	char* filename = NULL;
	gboolean prompt_again = FALSE;

	pal_output_fg(BRIGHT, GREEN, "> ");
	g_print("Calendar file (usually ending with \".pal\") to add event to:\n");

	int y,x;
	getyx( stdscr, y, x );

	/* get the filename */
	do {
	rl_completion_entry_function = rl_filename_completion_function;

	prompt_again = FALSE;

	filename = pal_rl_get_line_default("Filename: ", y, 0, g_strdup("~/.pal/"));


	/* clear any filename completions */
	clrtobot();

	/* if first character is ~, replace it with the home directory */
	if(*filename == '~') {
		char* orig_filename = filename;
		filename = g_strconcat(g_get_home_dir(), filename+1, NULL);
		free(orig_filename);
	}

	/* check if file exists */
	if(g_file_test(filename, G_FILE_TEST_EXISTS)) {
		/* make sure it aint a directory */
		if(g_file_test(filename, G_FILE_TEST_IS_DIR))
		{
		g_print("\n");
		pal_output_error("ERROR: %s is a directory.\n", filename);
		prompt_again = TRUE;
		}
	} else {
		int y,x;
		getyx(stdscr, y,x);

			/* ask to create the file if it doesn't exist */
		g_print("\n");
		pal_output_error("WARNING: %s does not exist.\n", filename);

		if(!pal_rl_get_y_n("Create? [y/n]: ")) {
		move(y,x);
		clrtobot();
		prompt_again = TRUE;
		} else {
		/* create the file */
		FILE*  file = fopen(filename, "w");
		if(file == NULL) {
			move(y+1,0);
			pal_output_error("ERROR: Can't create %s.\n", filename);
			clrtobot();

			prompt_again = TRUE;
		} else {
			char *markers = NULL;
			char *event_type = NULL;
			char *top_line = NULL;

			g_print("\n");
			pal_output_fg(BRIGHT, GREEN, "> ");
			g_print("Information for %s:\n", filename);
			getyx( stdscr, y, x );
			do
			markers = pal_rl_get_line("2 character marker for calendar: ", y, 0);
			while(g_utf8_strlen(markers, -1) != 2 || markers[0] == '#');

			g_print("\n");
			getyx( stdscr, y, x);
			event_type = pal_rl_get_line("Calendar title: ", y, 0);

			top_line = g_strconcat(markers, " ", event_type, "\n", NULL);

			g_print("\n");
			pal_output_fg(BRIGHT, RED, "> ");
			g_print("%s\n", "If you want events in this new calendar file to appear when you run pal,\n  you need to manually update ~/.pal/pal.conf");
			g_print("%s\n", "Press any key to continue.");
			getch();

			fputs(top_line, file);

			free(top_line);
			free(event_type);
			free(markers);

			fclose(file);
		}
		}
	}

	if(prompt_again)
		free(filename);

	}while (prompt_again);

	/* no more completion necessary */
	rl_completion_entry_function = (rl_compentry_func_t*) pal_rl_no_match;

	return filename;
}


void
pal_add_write_file(char* filename, char* key, char* desc)
{
	FILE *file = NULL;
	char* write_line = NULL;
	gboolean no_newline = FALSE;

	/* check for newline at end of file */
	do {
		file = fopen(filename, "r");
		if(file == NULL) {
			pal_output_error("ERROR: Can't read from file %s.\n", filename);
			if(!pal_rl_get_y_n("Try again? [y/n]: "))
			return;
		}
	} while(file == NULL);

	fseek(file, -1, SEEK_END);
	if(fgetc(file)!= '\n')
	no_newline = TRUE;
	fclose(file);

	/* write the new event out to that file */
	do {
		file = fopen(filename, "a");
		if(file == NULL) {
			pal_output_error("ERROR: Can't write to file %s.\n", filename);
			if(!pal_rl_get_y_n("Try again? [y/n]: "))
			return;
		}
	} while(file == NULL);

	/* put a newline at end of file if one isn't there already */
	if(no_newline)
	fputc('\n', file);

	write_line = g_strconcat(key, " ", desc, "\n", NULL);
	fputs(write_line, file);
	g_print("\n");
	pal_output_fg(BRIGHT, GREEN, ">>> ");
	g_print("Wrote new event \"%s %s\" to %s.\n", key, desc, filename);
	free(write_line);
	fclose(file);

}

void
pal_add_event( struct tm *selected_date )
{
	char* filename = NULL;
	char* description = NULL;
	char* key = NULL;
	PalEvent *event = NULL;
	char buf[128] ="";

	clear();
	pal_output_fg(BRIGHT, GREEN, "* * * ");
	pal_output_attr(BRIGHT, "Add an event");
	pal_output_fg(BRIGHT, GREEN, " * * *\n");

	pal_output_fg(BRIGHT, GREEN, "Selected date: ");
	strftime( buf, 128, settings->date_fmt, selected_date );
	pal_output_attr(BRIGHT, buf);
	pal_output_fg(BRIGHT, GREEN, "\n");

	filename = pal_add_get_file();
	g_print("\n");

	key = pal_add_get_recur(selected_date);

	g_print("\n");
	pal_output_fg(BRIGHT, GREEN, "Date (in pal's format):");
	g_print(" %s\n\n", key);
	description = pal_add_get_desc();

	/* Check the event is parsable */
	event = pal_event_init();
	if(!parse_event(event, key)) {
		pal_output_error("INTERNAL ERROR: Please report this error message along with the input that generated it.\n");
		pal_output_error("INVALID KEY: %s\n", key);
		getch();
	} else {
		pal_add_write_file(filename, key, description);
	}

	pal_event_free(event);

	free(filename);
	free(description);
	free(key);

	pal_main_reload();
}

