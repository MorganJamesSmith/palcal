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

#include <ncurses.h>

#include "main.h"
#include "output.h"
#include "event.h"
#include "rl.h"
#include "add.h"
#include "del.h"
#include "input.h"


#define NUM_FIELDS 12

typedef struct _PalViewEvent {
    char *prompt;
    gboolean editable;
    gboolean changed;
} PalViewEvent;


PalViewEvent *fieldlist;
static int selectedField;
static char align[] ="%15s ";

char* pal_edit_get_field_val(int i, PalEvent *event, struct tm *d)
{
    char *buf = NULL;

    switch(i) {
		case 0:
		    return g_strdup(event->text);
		case 1:
		    return event->eventtype->get_descr( d );
		case 2:
		    buf = g_malloc(sizeof(char)*128);
		    snprintf(buf, 128, "%d", event->period_count);
		    return buf;
		case 3:
		    if(event->start_date == NULL)
				return g_strdup("None");

		    buf = g_malloc(sizeof(char)*128);
		    strftime(buf, 128, settings->date_fmt, event->start_date);
		    return buf;

		case 4:
		    if(event->end_date == NULL)
				return g_strdup("None");

		    buf = g_malloc(sizeof(char)*128);
		    strftime(buf, 128, settings->date_fmt, event->end_date);
		    return buf;
		case 5:
		    if(event->start_time == NULL)
				return g_strdup("None");

		    buf = g_malloc(sizeof(char)*128);
		    snprintf(buf, 128, "%02d:%02d", event->start_time->hour, event->start_time->min);
		    return buf;
		case 6:
		    if(event->end_time == NULL)
				return g_strdup("None");

		    buf = g_malloc(sizeof(char)*128);
		    snprintf(buf, 128, "%02d:%02d", event->end_time->hour, event->end_time->min);
		    return buf;
		case 7:
		    return g_strdup(event->key);
		case 8:
		    return g_strdup(event->date_string);
		case 9:
		    return g_strdup(event->file_name);
		case 10:
		    buf = g_malloc(sizeof(char)*128);
		    snprintf(buf, 128, "%c%c", event->start, event->end);
		    return buf;
		case 11:
		    if(event->color == -1)
				return string_color_of(settings->event_color);
		    return string_color_of(settings->event_color);
		default:
		    return g_strdup("NOT IMPLEMENTED");
    }
}


void pal_edit_init(void)
{
    PalViewEvent initfieldlist[NUM_FIELDS] = {
		{ "Event Description",         TRUE,  FALSE },
		{ "Event Type",                TRUE,  FALSE },
		{ "Skip Count",                TRUE,  FALSE },
		{ "Start Date",                TRUE,  FALSE },
		{ "End Date",                  TRUE,  FALSE },
		{ "Start Time",                TRUE,  FALSE },
		{ "End Time",                  TRUE,  FALSE },
		{ "Hashtable Key",             FALSE, FALSE },
		{ "Date in File",              FALSE, FALSE },
		{ "Event File",                FALSE, FALSE },
		{ "Event Marked on Calendar?", FALSE, FALSE },
		{ "File Color",                FALSE, FALSE }
	};


    selectedField = 0;

    fieldlist = g_malloc(sizeof(PalViewEvent)*NUM_FIELDS);
    memcpy(fieldlist, initfieldlist, sizeof(PalViewEvent)*NUM_FIELDS);
}


void pal_edit_refresh(PalEvent* event, struct tm *d)
{
    int i;
    char *fieldval = NULL;


    move(0,0);

    for(i=0; i<NUM_FIELDS; i++) {
		char *prompt = NULL;
		prompt = g_strconcat(fieldlist[i].prompt, ": ", NULL);
		if(selectedField == i)
		    pal_output_fg(BRIGHT, GREEN, align, prompt);
		else
		    pal_output_fg(BRIGHT, BLACK, align, prompt);
		free(prompt);
		fieldval = pal_edit_get_field_val(i, event, d);
		g_print("%s\n", fieldval);
		free(fieldval);
    }
    clrtobot();
}


void pal_edit_event(PalEvent* event, struct tm *d)
{
    pal_edit_init();
    while(1) {
		int c;

		pal_edit_refresh(event, d);
		while((c = getch()) == ERR);

		switch(c) {
		    case KEY_RESIZE:
				pal_edit_refresh(event, d);
				break;
		    case KEY_DOWN:
				selectedField = (selectedField + 1) % NUM_FIELDS;
				while(!fieldlist[selectedField].editable && selectedField < NUM_FIELDS)
					selectedField = (selectedField + 1) % NUM_FIELDS;
				break;
		    case KEY_UP:
				selectedField = (selectedField - 1 + NUM_FIELDS) % NUM_FIELDS;
				while(!fieldlist[selectedField].editable && selectedField < NUM_FIELDS)
				    selectedField = (selectedField - 1 + NUM_FIELDS) % NUM_FIELDS;
				break;
		    case KEY_ENTER:
		    case '\r':
		    	{
				char *fieldval = pal_edit_get_field_val(selectedField, event, d);
				char *prompt = g_strconcat(fieldlist[selectedField].prompt, ": ", NULL);
				char align_prompt[128];

				snprintf(align_prompt, 128, align, prompt);

				pal_rl_get_line_default(align_prompt, selectedField, 0, fieldval);
				free(fieldval);
				free(prompt);
				break;
		    	}
		    case 'q':
		    case 'Q':
			return;
		}
    }
}

