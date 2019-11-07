#ifndef PAL_MAIN_H
#define PAL_MAIN_H

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

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <libintl.h>
#define _(String) gettext (String)
#define gettext_noop(String) String
#define N_(String) gettext_noop (String)

GDate* get_query_date(char* date_string, bool show_error);
void pal_main_reload(void);

/* a structure that contains all of the runtime settings */
typedef struct _Settings {
    int cal_lines;           /* number of lines for calendar */
    int range_days;          /* print events in the next 'range_days' days */
    int range_neg_days;      /* print events within 'range_neg_days' days old */
    bool range_arg;          /* user started pal with -r */
    char* search_string;     /* regex to search for */
    bool verbose;            /* verbose output */
    GDate* query_date;       /* from argument used after -d */
    int expunge;             /* expunge events older than 'expunge' days */
    bool mail;               /* --mail */
    char* conf_file;         /* .conf file to use */
    bool specified_conf_file;/* user specified a .conf file */
    char* date_fmt;          /* format of date to use with -r */
    bool week_start_monday;  /* weeks should start on monday */
    bool reverse_order;      /* show things in reverse order */
    bool cal_on_bottom;      /* show calendar last */
    bool no_columns;         /* don't use columns */
    bool hide_event_type;    /* hide the type of listed events */
    int term_cols;           /* number of columns in terminal */
    int term_rows;           /* number of rows in terminal */
    bool manage_events;      /* add an event to pal, interactive */
    bool curses;             /* use curses output functions instead of glib */
    int event_color;         /* default event color */
    bool html_out;           /* html output */
    bool latex_out;          /* LaTeX output */
    bool compact_list;       /* show a compact list */
    bool show_weeknum;       /* Show weeknum in output */
    char* compact_date_fmt;  /* comapct list date format */
    char* pal_file;          /* specified one pal file to load instead
			       * of those in pal.conf */
} Settings;

typedef struct _PalTime {
    int hour;
    int min;
} PalTime;

typedef enum _PalPeriodic {
    PAL_ONCEONLY,
    PAL_DAILY,
    PAL_WEEKLY,
    PAL_MONTHLY,
    PAL_YEARLY
} PalPeriodic;

#define MAX_KEYLEN 16

/* Defines event types. Where a string is returned, caller must free */

typedef struct _PalEventType {
    PalPeriodic period;
    bool (*valid_string)(const char *);     /* Returns true if this string is valid for this event type */
    bool (*get_key)(const GDate *, char *); /* For the given date, return the key for this event type */
    char *(*get_descr)(const GDate *);      /* For the given date, return a textual representation */
} PalEventType;


/* See event.c for definition of PalEventTypes */
extern PalEventType PalEventTypes[];
extern const int PAL_NUM_EVENTTYPES;  /* number of items in PalEventTypes */

typedef struct _PalEvent {
    char* text;               /* description of event */
    gunichar start;           /* character used before the day in calendar */
    gunichar end;             /* character used after the day in calendar */
    bool hide;                /* should the event be hidden on the calendar? */
    char* type;               /* type of event it is (from top of calendar file) */
    int file_num;             /* this event was in the file_num-th file loaded */
    char* file_name;          /* name of the file containing this event */
    int color;                /* color to be used with this event */
    GDate* start_date;        /* for recurring events, date event starts on */
    GDate* end_date;          /* for recurring events, date event ends on */
    PalTime* start_time;      /* 1st time listed in event description */
    PalTime* end_time;        /* 2nd time listed in event description */
    char* date_string;        /* date string used in the file for this event */
    bool global;              /* true if event is in a global file */
    int period_count;         /* How often repeat (default=1) */
    char* key;                /* Key in hash table */
    PalEventType *eventtype;  /* Pointer to eventtype struct */
} PalEvent;

extern Settings* settings;
extern GHashTable* ht;        /* ht holds the loaded events */

#endif
