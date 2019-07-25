/* pal
 *
 * Copyright (C) 2004, Scott Kuhl
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

#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <curses.h>
#include <assert.h>

/* for vsnprintf */
#include <stdarg.h>

#include "main.h"
#include "colorize.h"
#include "event.h"


/* Interface between g_print and ncurses.  This is also the print
 * handler that gets used if we aren't using curses.  The default
 * handler apparently calls fflush() all of the time.  This slows down
 * some of our code that calls fflush() often (such as the code we use
 * to print out the calendar).  */
//void
//pal_output_handler( const char *instr )
//{
//    gsize strsize;
//    char *outstr = g_locale_from_utf8(instr, -1, NULL, &strsize, NULL);
//
//    if(settings->curses)
//		waddnstr(pal_curwin, outstr, strsize);
//    else
//		fputs(outstr, stdout);
//
//    free(outstr);
//}


/* set attribute w/o changing color */
void
pal_output_attr(int attr, char *formatString, ...)
{
    char buf[2048] = "";
    va_list argptr;

    va_start(argptr, formatString);
    if( attr == BRIGHT )
      colorize_bright();

    /* glib 2.2 provides g_vfprintf */
    vsnprintf(buf, 2048, formatString, argptr);
    g_print("%s", buf);

    colorize_reset();
    va_end(argptr);
}

/* set foreground color and attribute */
void
pal_output_fg( int attr, int color, char *formatString, ...)
{
    char buf[2048] = "";
    va_list argptr;

    va_start(argptr, formatString);
    colorize_fg(attr, color);

    /* glib 2.2 provides g_vfprintf */
    vsnprintf(buf, 2048, formatString, argptr);
    g_print("%s", buf);

    colorize_reset();
    va_end(argptr);
}


void pal_output_error(char *formatString, ... )
{
    char buf[2048] = "";
    va_list argptr;

    va_start( argptr, formatString );
    colorize_error();

    /* glib 2.2 provides g_vfprintf */
    vsnprintf(buf, 2048, formatString, argptr);
    g_printerr("%s", buf); /* use g_print to convert from UTF-8 */

    colorize_reset();
    va_end(argptr);
}


/* finishes with date on the sunday of the next week */
static void
pal_output_text_week(struct tm* date, int force_month_label, const struct tm* today)
{

    if(settings->week_start_monday) {
		/* go to last day in week (sun == 0) */
		mktime(date); //update date object
		date->tm_mday += (7 - date->tm_wday) % 7;
		mktime(date); //update date object
		assert(date->tm_wday == 0);
	} else {
		/* go to last day in week (sat == 6) */
		mktime(date); //update date object
		date->tm_mday += 6 - date->tm_wday;
		mktime(date); //update date object
		assert(date->tm_wday == 6);
	}


    for(int i = 0; i<7; i++) {
		if(date->tm_mday == 1)
	    	force_month_label = 1;

		date->tm_mday -= 1;
		mktime(date);
    }

    date->tm_mday += 1;
	mktime(date);
	assert(settings->week_start_monday ? date->tm_wday == 1 : date->tm_wday == 0);
    /* date is now at beginning of week */

    if(force_month_label) {
		char buf[1024];

		date->tm_mday += 6;
		mktime(date);
		strftime(buf, 128, "%b", date);
		date->tm_mday -= 6;
		mktime(date);

		/* make sure we're only showing 3 characters */
		if(g_utf8_strlen(buf, -1) != 3) {
		    /* append whitespace in case "buf" is too short */
		    char* s = g_strconcat(buf, "        ", NULL);

		    /* just show first 3 characters of month */
		    g_utf8_strncpy(buf, s, 3);
		    free(s);
		}

		pal_output_fg(BRIGHT, GREEN, "%s ", buf);

    } else if( settings->show_weeknum ) {
		char weeknum[3];
		settings->week_start_monday ? strftime(weeknum, sizeof(char) * 3, "%W", date)
			: strftime(weeknum, sizeof(char) * 3, "%U", date);
		pal_output_fg(BRIGHT, GREEN, " %s ", weeknum);
    } else {
		g_print("    ");
	}


    for(int i=0; i<7; i++) {
		GList* events = NULL;
		char start=' ', end=' ';
		char utf8_buf[8];
		int color = settings->event_color;
		events = get_events(date);

		if(difftime(mktime(date),mktime(today))/(24*3600) == 0) {
		    start = end = '@';
		} else if(events != NULL) {
		    GList* item  = g_list_first(events);
		    PalEvent *event = (PalEvent*) item->data;

		    int same_char  = 1;
		    int same_color = 1;

		    /* skip to a event that isn't hidden or to the end of the list */
		    while(g_list_length(item) > 1 && event->hide) {
				item = g_list_next(item);
				event = (PalEvent*) item->data;
		    }

		    /* save the markers for the event */
		    if(event->hide) {
				start = ' ', end = ' ';
			} else {
				start = event->start;
				end   = event->end;
				color = event->color;
		    }

		    /* if multiple events left */
		    while(g_list_length(item) > 1) {
				item = g_list_next(item);
				event = (PalEvent*) item->data;

				/* find next non-hidden event */
				while(g_list_length(item) > 1 && event->hide) {
				    item = g_list_next(item);
				    event = (PalEvent*) item->data;
				}

				/* if this event is hidden, there aren't any more non-hidden events left */
				/* if this event isn't hidden, then determine if it has different markers */
				if( ! event->hide ) {
				    gunichar new_start = event->start;
				    gunichar new_end   = event->end;
				    int     new_color = event->color;

				    if(new_start != start || new_end != end)
					same_char = 0;
				    if(new_color != color)
					same_color = 0;
				}
				if(same_char == 0)
				    start = '*', end = '*';
				if(same_color == 0)
				    color = -1;
		    }
		}

		utf8_buf[g_unichar_to_utf8(start, utf8_buf)] = '\0';

		/* print color for marker if needed */
		if(start != ' ' && end != ' ') {
		    if(color == -1)
				pal_output_fg(BRIGHT, settings->event_color, utf8_buf);
		    else
				pal_output_fg(BRIGHT, color, utf8_buf);
		} else {
		    g_print(utf8_buf);
		}


		if(difftime(mktime(date),mktime(today))/(3600*24) == 0) { /* make today bright */
		    pal_output_attr(BRIGHT, "%02d", date->tm_mday);
		} else {
		    printf("%02d", date->tm_mday);
		}


		utf8_buf[g_unichar_to_utf8(end, utf8_buf)] = '\0';

		/* print color for marker if needed */
		if(start != ' ' && end != ' ') {
		    if(color == -1)
				pal_output_fg(BRIGHT, settings->event_color, utf8_buf);
		    else
				pal_output_fg(BRIGHT, color, utf8_buf);

		} else {
		    g_print(utf8_buf);
		}


		/* print extra space between days */
		if(i != 6)
		    g_print(" ");

		date->tm_mday += 1;
		mktime(date);
		g_list_free(events);
    }
	assert(settings->week_start_monday ? date->tm_wday == 1 : date->tm_wday == 0);

}



static void
pal_output_week(struct tm* date, int force_month_label, const struct tm* today)
{

    pal_output_text_week(date, force_month_label, today);

    if(!settings->no_columns && settings->term_cols >= 77) {
		pal_output_fg(DIM,YELLOW,"%s","|");

    	   /* skip ahead to next column */
		date->tm_mday += settings->cal_lines*7;
		pal_output_text_week(date, force_month_label, today);

		/* skip back to where we were */
		date->tm_mday -= settings->cal_lines*7 + 7;

    }

    if(settings->term_cols != 77)
		g_print("\n");

}



void
pal_output_cal(int num_lines, const struct tm* today)
{
    int on_week = 0;
    char* week_hdr;
    struct tm date = *today;

    if(num_lines <= 0)
		return;

    if(settings->week_start_monday)
		week_hdr = strdup("Mo   Tu   We   Th   Fr   Sa   Su");
    else
		week_hdr = strdup("Su   Mo   Tu   We   Th   Fr   Sa");


    /* if showing enough lines, show previous week. */
    if(num_lines > 3) {
		date.tm_mday -= 7;
		mktime(&date); //normalize date
	}

    if(settings->no_columns || settings->term_cols < 77)
		pal_output_fg(BRIGHT,GREEN, "     %s\n", week_hdr);
    else {
		pal_output_fg(BRIGHT,GREEN,"     %s ", week_hdr);
		pal_output_fg(DIM,YELLOW,"%s","|");
		pal_output_fg(BRIGHT,GREEN,"     %s\n", week_hdr);
    }

    free(week_hdr);

    while(on_week < num_lines) {
		if(on_week == 0)
	    	pal_output_week(&date, 1, today);
		else
	    	pal_output_week(&date, 0, today);
		on_week++;
    }
}

/* replaces tabs with spaces */
static void pal_output_strip_tabs(char* string)
{
    char *ptr = string;
    while(*ptr != '\0')
    {
	if(*ptr == '\t')
	    *ptr = ' ';
	ptr++;
    }
}


/* returns the length of the next word */
static int pal_output_wordlen(char* string)
{
    char* p = string;
    int i=0;
    while(*p != ' ' && *p != '\0')
    {
	p = g_utf8_next_char(p);
	i++;
    }

    return i;
}


/* This function does not yet handle tabs and color codes.  Tabs
 * should be stripped from 'string' before this is called.
 * "chars_used" indicates the number of characters already used on the
 * line that "string" will be printed out on.
 * Returns the number of lines printed.
 */
int pal_output_wrap(char* string, int chars_used, int indent)
{
    int numlines = 0;
    char* s = string;
    int width = settings->term_cols - 1;  /* -1 to avoid unexpected wrap */
    if( width <= 0 )
        width = 10000;

    while(*s != '\0')
    {
	/* print out any leading whitespace on this line */
	while(*s == ' ' && chars_used < width)
	{
	    g_print(" ");
	    chars_used++;
	    s = g_utf8_next_char(s);
	}

	/* if word doesn't fit on line, split it */
	if(pal_output_wordlen(s)+chars_used > width)
	{
	    char line[2048];
	    g_utf8_strncpy(line, s, width - chars_used);
	    g_print("%s\n", line); /* print as much as we can */
	    numlines++;
	    s = g_utf8_offset_to_pointer(s, width - chars_used);
	    chars_used = 0;
	}
	else /* if next word fits on line */
	{

	    while(*s != '\0' &&
		  pal_output_wordlen(s)+chars_used <= width)
	    {

		/* if the next word is not a blank, copy the word */
		if(*s != ' ')
		{
		    int word_len = pal_output_wordlen(s);
		    char word[2048];
		    g_utf8_strncpy(word, s, word_len);
		    g_print("%s", word);
		    s = g_utf8_offset_to_pointer(s, word_len);
		    chars_used += word_len;
		}

		/* print out any spaces that follow the word */
		while(*s == ' ' && chars_used < width)
		{
		    g_print(" ");
		    chars_used++;
		    s = g_utf8_next_char(s);
		}


		/* if we filled line up perfectly, and there is a
		 * space next in the string, ignore it---the newline
		 * will act as the space */
		if(chars_used == width && *s == ' ')
		{
		    s = g_utf8_next_char(s);

		    /* if the next line is a space too, break out of
		     * this loop.  If we don't break, whitespace might
		     * not be preserved properly. */
		    if(*s == ' ')
			break;
		}
	    }

	    numlines++;
	    g_print("\n");

	    chars_used = width;
	}

	/* if not done, print indents for next line */
	if(*s != '\0')
	{
	    int i;

	    /* now, chars_used == width, onto the next line! */
	    chars_used = indent;

	    for(i=0; i<indent; i++)
		g_print(" ");
	}

    }

    return numlines;

}




/* If event_number is -1, don't number the events.
   Returns the number of lines printed.
*/
int
pal_output_event(const PalEvent* event, const struct tm* date, const int selected)
{
    int numlines = 0;
    const int indent = 2;
    char* event_text = NULL;

    if(selected)
		pal_output_fg(BRIGHT, GREEN, "%s ", ">");
    else if(event->color == -1)
		pal_output_fg(BRIGHT, settings->event_color, "%s ", "*");
    else
		pal_output_fg(BRIGHT, event->color, "%s ", "*");

    pal_output_strip_tabs(event->text);
    pal_output_strip_tabs(event->type);

    event_text = pal_event_escape(event, date);

    if(settings->compact_list) {
		char* s = NULL;
		char *date_text = asctime(date);
		pal_output_attr(BRIGHT, "%s ", date_text);

		if(settings->hide_event_type)
		    s = g_strconcat(event_text, NULL);
		else
		    s = g_strconcat(event->type, ": ", event_text, NULL);

		numlines += pal_output_wrap(s, indent+g_utf8_strlen(date_text,-1)+1, indent);
		free(s);
    } else {
		if(settings->hide_event_type)
	    	numlines += pal_output_wrap(event_text, indent, indent);
		else {
	    	char* s = g_strconcat(event->type, ": ", event_text, NULL);
	    	numlines += pal_output_wrap(s, indent, indent);
	    	free(s);
		}
    }
    free(event_text);

    return numlines;
}


void pal_output_date_line(const struct tm* date)
{
    int diff = 0;

	time_t currenttime = time(NULL); //TODO breaks if day changes from last time we got today
	struct tm *today = localtime(&currenttime);

	char *pretty_date = asctime(date);

    pal_output_attr(BRIGHT, "%s", pretty_date);
    g_print(" - ");

    diff = difftime(mktime(date),mktime(today))/(24*3600);
    if(diff == 0)
		pal_output_fg(BRIGHT, RED, "%s", "Today");
    else if(diff == 1)
		pal_output_fg(BRIGHT, YELLOW, "%s", "Tomorrow");
    else if(diff == -1)
		g_print("%s", "Yesterday");
    else if(diff > 1)
		g_print("%d days away", diff);
    else if(diff < -1)
		g_print("%d days ago", -1*diff);

    g_print("\n");
}


/* outputs the events in the order of PalEvent->file_num.
   Returns the number of lines printed. */
int
pal_output_date(struct tm* date, int show_empty_days, int selected_event)
{
    int numlines = 0;
    GList* events = get_events(date);
    int num_events = g_list_length(events);

    if(events != NULL || show_empty_days) {
		GList* item = NULL;
		int i;

		if(!settings->compact_list) {
		    pal_output_date_line(date);
		    numlines++;
		}

		item = g_list_first(events);

		for(i=0; i<num_events; i++) {
		    numlines += pal_output_event((PalEvent*) (item->data),
						 date, i==selected_event);

		    item = g_list_next(item);
		}


		if(num_events == 0) {
		    if(settings->compact_list) {
					char *pretty_date = asctime(date);
					pal_output_attr(BRIGHT, "  %s ", pretty_date);
					g_print("%s\n", "No events.");
					numlines++;
		    	} else {
					g_print("%s\n", "No events.");
					numlines++;
		    }
		}

		if(!settings->compact_list) {
		    g_print("\n");
		    numlines++;
		}
    }
    return numlines;
}


/* returns the PalEvent for the given event_number */
PalEvent* pal_output_event_num(const struct tm* date, int event_number)
{
    GList* events = get_events(date);
    int num_events = g_list_length(events);

    if(events == NULL || event_number < 1 || event_number > num_events)
	return NULL;

    return (PalEvent*) g_list_nth_data(events, event_number-1);
}

