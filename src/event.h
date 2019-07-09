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
GList* get_events(const GDate* date);
/* Return just the count */
gint pal_get_event_count( GDate *date );


PalEvent* pal_event_init(void);
PalEvent* pal_event_copy(PalEvent* orig);

void pal_event_free(PalEvent* event);
void pal_event_fill_dates(PalEvent* pal_event, const gchar* date_string);

gboolean is_valid_todo(const gchar* date_string);
gboolean get_key_todo(const GDate* date, gchar *buffer);
gchar *get_descr_todo(const GDate *date);

gboolean is_valid_daily(const gchar* date_string);
gboolean get_key_daily(const GDate* date, gchar *buffer);
gchar *get_descr_daily(const GDate *date);

gboolean is_valid_yyyymmdd(const gchar* date_string);
gboolean is_valid_0000mmdd(const gchar* date_string);

gboolean get_key_yyyymmdd(const GDate* date, gchar *buffer);
gchar *get_descr_yyyymmdd(const GDate* date);

gboolean is_valid_weekly(const gchar* date_string);
gboolean get_key_weekly(const GDate* date, gchar* buffer);
gchar *get_descr_weekly(const GDate* date);

gboolean is_valid_000000dd(const gchar* date_string);
gboolean get_key_000000dd(const GDate* date, gchar* buffer);
gchar *get_descr_000000dd(const GDate* date);

gboolean is_valid_0000mmdd(const gchar* date_string);
gboolean get_key_0000mmdd(const GDate* date, gchar* buffer);
gchar *get_descr_0000mmdd(const GDate* date);

gboolean is_valid_star_00nd(const gchar* date_string);
gboolean get_key_star_00nd(const GDate* date, gchar* buffer);
gchar *get_descr_star_00nd(const GDate* date);
gboolean is_valid_star_mmnd(const gchar* date_string);
gboolean get_key_star_mmnd(const GDate* date, gchar* buffer);
gchar *get_descr_star_mmnd(const GDate* date);
gboolean is_valid_star_00Ld(const gchar* date_string);
gboolean get_key_star_00Ld(const GDate* date, gchar* buffer);
gchar *get_descr_star_00Ld(const GDate* date);
gboolean is_valid_star_mmLd(const gchar* date_string);
gboolean get_key_star_mmLd(const GDate* date, gchar* buffer);
gchar *get_descr_star_mmLd(const GDate* date);
gboolean is_valid_EASTER(const gchar* date_string);
GDate* find_easter(gint year);
gboolean get_key_EASTER(const GDate* date, gchar *buffer);
gchar *get_descr_EASTER(const GDate* date);
gboolean last_weekday_of_month(const GDate* date);
gint get_nth_day(const GDate* date);
GList* inspect_range(GList* list, const GDate* date);
gint pal_event_sort_fn(gconstpointer x, gconstpointer y);
GList* pal_event_sort_events(GList* events);

gboolean parse_event(PalEvent *event, const gchar* date_string);
gchar* get_key(const GDate* date);
GDate* get_date(const gchar* key);
gchar* pal_event_date_string_to_key(const gchar* date_string);
gchar* pal_event_escape(const PalEvent* event, const GDate* today);


#endif
