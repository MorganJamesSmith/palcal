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


struct tm* get_query_date(char* date_string, gboolean show_error);
void pal_main_reload(void);

/* a structure that contains all of the runtime settings */
typedef struct _Settings {
    int cal_lines;           /* number of lines for calendar */
    int range_days;          /* print events in the next 'range_days' days */
    int range_neg_days;      /* print events within 'range_neg_days' days old */
    gboolean range_arg;       /* user started pal with -r */
    char* search_string;     /* regex to search for */
    gboolean verbose;         /* verbose output */
    struct tm* query_date;        /* from argument used after -d */
    int expunge;             /* expunge events older than 'expunge' days */
    gboolean mail;            /* --mail */
    char* conf_file;         /* .conf file to use */
    gboolean specified_conf_file;  /* user specified a .conf file */
    char* date_fmt;          /* format of date to use with -r */
    gboolean week_start_monday; /* weeks should start on monday */
    gboolean reverse_order;   /* show things in reverse order */
    gboolean cal_on_bottom;   /* show calendar last */
    gboolean no_columns;      /* don't use columns */
    gboolean hide_event_type; /* hide the type of listed events */
    int term_cols;           /* number of columns in terminal */
    int term_rows;           /* number of rows in terminal */
    gboolean manage_events;   /* add an event to pal, interactive */
    gboolean curses;          /* use curses output functions instead of glib */
    int event_color;         /* default event color */
    gboolean html_out;        /* html output */
    gboolean latex_out;       /* LaTeX output */
    gboolean compact_list;    /* show a compact list */
    gboolean show_weeknum;    /* Show weeknum in output */
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
    gboolean (*valid_string)(const char *);     /* Returns true if this string is valid for this event type */
    gboolean (*get_key)(const struct tm *, char *); /* For the given date, return the key for this event type */
    char *(*get_descr)(const struct tm *);          /* For the given date, return a textual representation */
} PalEventType;


/* See event.c for definition of PalEventTypes */
extern PalEventType PalEventTypes[];
extern const int PAL_NUM_EVENTTYPES;  /* number of items in PalEventTypes */

typedef struct _PalEvent {
    char* text;       /* description of event */
    gunichar start;    /* character used before the day in calendar */
    gunichar end;      /* character used after the day in calendar */
    gboolean hide;     /* should the event be hidden on the calendar? */
    char* type;       /* type of event it is (from top of calendar file) */
    int file_num;     /* this event was in the file_num-th file loaded */
    char* file_name;    /* name of the file containing this event */
    int color;          /* color to be used with this event */
    struct tm* start_date;   /* for recurring events, date event starts on */
    struct tm* end_date;     /* for recurring events, date event ends on */
    PalTime* start_time; /* 1st time listed in event description */
    PalTime* end_time;   /* 2nd time listed in event description */
    char* date_string;  /* date string used in the file for this event */
    gboolean global;     /* TRUE if event is in a global file */
    int period_count;   /* How often repeat (default=1) */
    char* key;          /* Key in hash table */
    PalEventType *eventtype;  /* Pointer to eventtype struct */
} PalEvent;

extern Settings* settings;
extern GHashTable* ht;     /* ht holds the loaded events */

#endif
