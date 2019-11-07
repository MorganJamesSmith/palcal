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
int pal_get_event_count(GDate *date);


PalEvent* pal_event_init(void);
void pal_event_free(PalEvent* event);
void pal_event_fill_dates(PalEvent* pal_event, const char* date_string);
bool parse_event(PalEvent *event, const char* date_string);
char* get_key(const GDate* date);
GDate* get_date(const char* key);
char* pal_event_date_string_to_key(const char* date_string);
PalEvent* pal_event_copy(PalEvent* orig);
char* pal_event_escape(const PalEvent* event, const GDate* today);

bool is_valid_yyyymmdd(const char* date_string);
bool is_valid_0000mmdd(const char* date_string);

#endif
