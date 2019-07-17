#ifndef PAL_EVENT_H
#define PAL_EVENT_H

/* pal
 *
 * Copyright (C) 2003, Scott Kuhl
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

/* returns a list of events on the givent date */
GList* get_events(const struct tm* date);
/* Return just the count */
int pal_get_event_count( struct tm *date );


PalEvent* pal_event_init(void);
PalEvent* pal_event_copy(PalEvent* orig);

void pal_event_free(PalEvent* event);
void pal_event_fill_dates(PalEvent* pal_event, const char* date_string);

gboolean is_valid_todo(const char* date_string);
gboolean get_key_todo(const struct tm* date, char *buffer);
char *get_descr_todo(const struct tm *date);

gboolean is_valid_daily(const char* date_string);
gboolean get_key_daily(const struct tm* date, char *buffer);
char *get_descr_daily(const struct tm *date);

gboolean is_valid_yyyymmdd(const char* date_string);
gboolean is_valid_0000mmdd(const char* date_string);

gboolean get_key_yyyymmdd(const struct tm* date, char *buffer);
char *get_descr_yyyymmdd(const struct tm* date);

gboolean is_valid_weekly(const char* date_string);
gboolean get_key_weekly(const struct tm* date, char* buffer);
char *get_descr_weekly(const struct tm* date);

gboolean is_valid_000000dd(const char* date_string);
gboolean get_key_000000dd(const struct tm* date, char* buffer);
char *get_descr_000000dd(const struct tm* date);

gboolean is_valid_0000mmdd(const char* date_string);
gboolean get_key_0000mmdd(const struct tm* date, char* buffer);
char *get_descr_0000mmdd(const struct tm* date);

gboolean is_valid_star_00nd(const char* date_string);
gboolean get_key_star_00nd(const struct tm* date, char* buffer);
char *get_descr_star_00nd(const struct tm* date);
gboolean is_valid_star_mmnd(const char* date_string);
gboolean get_key_star_mmnd(const struct tm* date, char* buffer);
char *get_descr_star_mmnd(const struct tm* date);
gboolean is_valid_star_00Ld(const char* date_string);
gboolean get_key_star_00Ld(const struct tm* date, char* buffer);
char *get_descr_star_00Ld(const struct tm* date);
gboolean is_valid_star_mmLd(const char* date_string);
gboolean get_key_star_mmLd(const struct tm* date, char* buffer);
char *get_descr_star_mmLd(const struct tm* date);
gboolean is_valid_EASTER(const char* date_string);
struct tm* find_easter(int year);
gboolean get_key_EASTER(const struct tm* date, char *buffer);
char *get_descr_EASTER(const struct tm* date);
gboolean last_weekday_of_month(const struct tm* date);
int get_nth_day(const struct tm* date);
GList* inspect_range(GList* list, const struct tm* date);
int pal_event_sort_fn(gconstpointer x, gconstpointer y);
GList* pal_event_sort_events(GList* events);

gboolean parse_event(PalEvent *event, const char* date_string);
char* get_key(const struct tm* date);
struct tm* get_date(const char* key);
char* pal_event_date_string_to_key(const char* date_string);
char* pal_event_escape(const PalEvent* event, const struct tm* today);


#endif
