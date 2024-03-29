#ifndef PAL_INPUT_H
#define PAL_INPUT_H

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


GHashTable* load_files(void);
void pal_input_skip_comments(FILE* file, FILE* out_file);
PalEvent* pal_input_read_head(FILE* file, FILE* out_file, char* filename);
PalEvent* pal_input_read_event(FILE* file, FILE* out_file, char* filename, PalEvent* event_head, PalEvent* del_event);
bool pal_input_eof(FILE* file);
#endif
