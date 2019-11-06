#ifndef PAL_OUTPUT_H
#define PAL_OUTPUT_H

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

#include "main.h"

#include "colorize.h"

void pal_output_handler( const char *instr );

void pal_output_attr(int attr, char *formatString, ...);
void pal_output_fg(int attr, int color, char *formatString,...);
void pal_output_error(char *formatString, ... );

void pal_output_cal(int num_lines, const GDate* today);
int pal_output_date(GDate* date, gboolean show_empty_days, int select_event);
void pal_output_date_line(const GDate* date);
int pal_output_event(const PalEvent* event, const GDate* date, const gboolean selected);
int pal_output_wrap(char* string, int chars_used, int indent);
PalEvent* pal_output_event_num(const GDate* date, int event_number);
#endif
