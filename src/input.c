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

#include <time.h>
#include <string.h>

/* mkdir */
#include <sys/stat.h>
#include <sys/types.h>

#include "main.h"
#include "output.h"
#include "event.h"
#include "input.h"

static gboolean pal_input_file_is_global(const char* filename);

/* checks if events in the format yyyymmdd can be expunged */
static int should_be_expunged(const PalEvent* pal_event)
{
    struct tm today;
    struct tm *event_day;

    if(settings->expunge < 1)
	return FALSE;

    event_day = get_date(pal_event->date_string);
	time_t currenttime = time(NULL);
	today = *localtime(&currenttime);

	//TODO reimplement this bit
    /* if not a yyyymmdd (ie, not recurring) */
    if(!event_day) {
	/* recurring event with end_date */
		if(pal_event->end_date != NULL &&
			difftime(mktime(&today),mktime(pal_event->end_date))/(24*3600) <= -1*settings->expunge) {
		    return 1;
		}

	return 0;
    }

    /* if it is a yyyymmdd event (ie one-time event) */
    if(difftime(mktime(&today),mktime(pal_event->end_date))/(24*3600) <= -1*settings->expunge) {
		return 1;
    }

    return 0;

}


/* Returns the n'th time of the format h:mm or hh:mm that occurs in
 * the string s.  Returns NULL if no time exists in the string */
static PalTime* pal_input_get_time(char* s, int n)
{
    char* s_start = s;
    char *h1, *h2, *m1, *m2;

    if(n < 1 || s == NULL)
		return NULL;

    while(1) {
		s = g_utf8_find_next_char(s, NULL);

		if(*s == '\0')
		    return NULL;

		if(*s == ':') {
    	        /* get the digits in the hour */
		    h1 = g_utf8_find_prev_char(s_start, s);
		    h2 = g_utf8_find_prev_char(s_start, h1);

		    /* get the minutes digits */
		    m2 = g_utf8_find_next_char(s, NULL);
		    if(*m2 == '\0')
				return NULL;  /* hit end of line, done */
		    m1 = g_utf8_find_next_char(m2, NULL);

		    /* check for digits surrounding the : */
		    if(g_ascii_isdigit(*h1) &&
		       g_ascii_isdigit(*m1) && g_ascii_isdigit(*m2))
		    {
				int hour = 0;
				int min = 0;

				/* use 10s digit place in hours if it is a digit */
				if(h2 != NULL && *h2 != '\0' && g_ascii_isdigit(*h2))
				    hour = 10*g_ascii_digit_value(*h2);

				hour += g_ascii_digit_value(*h1);

				min = 10*g_ascii_digit_value(*m2);
				min += g_ascii_digit_value(*m1);

				if(min >= 0 && min < 60 &&
				   hour >= 0 && hour < 24)
				{
				    /* we just found a VALID date, if it is the nth
				     * one, return it */
				    if(n == 1) {
						PalTime* time = g_malloc(sizeof(PalTime));
						time->hour = hour;
						time->min = min;
						return time;
				    }
				    else
					n--;
				}
		    }
		}
    }
}



/* reads file stream until a non-commented, non-emtpy line is
 * encountered */
void pal_input_skip_comments(FILE* file, FILE* out_file)
{
    long start_of_line;
    char s[2048];

    do {
		char* orig_string = NULL;

		start_of_line = ftell(file);
		if(fgets(s, 2048, file) == NULL)
		    return;

		orig_string = g_strdup(s);
		g_strstrip(s);

		/* write the line out only if we aren't going to seek back later */
		if(out_file != NULL && (*s == '#' || *s=='\0'))
		    fputs(orig_string, out_file);

		free(orig_string);

    } while(*s == '#' || *s == '\0');

    /* now we are on the next non-comment, non-emtpy line. */
    /* jump back in stream so the next line read is not a comment */
    fseek(file, start_of_line, SEEK_SET);
}



/* reads in the 'first' line of the .pal file (two marker characters
 * and calendar title) */
PalEvent* pal_input_read_head(FILE* file, FILE* out_file, char* filename)
{
    char s[2048];
    char c;
    PalEvent* event_head = NULL;

    if(fgets(s, 2048, file) == NULL) {
		pal_output_error("WARNING: File is missing 2 character marker and event type: %s\n", filename);
		return NULL;
    }


    if(out_file != NULL)
		fputs(s, out_file);

    event_head = pal_event_init();

    g_strstrip(s);
    event_head->start = g_utf8_get_char(s);
    event_head->end   = g_utf8_get_char(g_utf8_offset_to_pointer(s,1));
    c = g_utf8_get_char(g_utf8_offset_to_pointer(s,2));
    event_head->type  = g_strdup(g_utf8_offset_to_pointer(s,3));
    event_head->file_name = g_strdup(filename);
    event_head->global = pal_input_file_is_global(filename);

    if(c != ' ' && c != '\t') { /* there should be white space here */
		char* file = g_path_get_basename(filename);
		pal_output_error("ERROR: First line is improperly formatted.\n");
		pal_output_error(  "       %s: %s\n", "FILE", file);
		free(file);
		pal_output_error(  "       %s: %s\n", "LINE", s);
		pal_event_free(event_head);
		return NULL;
    }

    /* check if text if UTF-8 */
    if(!g_utf8_validate(event_head->type, -1, NULL))
	pal_output_error("ERROR: First line is not ASCII or UTF-8 in %s.\n",
			 filename);

    g_strstrip(event_head->type);

    return event_head;
}

gboolean pal_input_eof(FILE* file)
{
    if(feof(file) != 0)
		return TRUE;
    return FALSE;
}

/* Returns:    The PalEvent for the next event in the file (or del_event if del_event was deleted).
 * file:       File stream to read from.
 * out_file:   Print the expunged output to out_file if it isn't NULL
 * filename:   Name of the file corresponding to the "file" stream
 * event_head: Default to these values for the returned PalEvent.
 * del_event:  If this event is encountered in the file, do not print it to out_file */
PalEvent* pal_input_read_event(FILE* file, FILE* out_file, char* filename, PalEvent* event_head,
			       PalEvent* del_event)
{
    char s[2048];

    char date_string[128];
    char* text_string = NULL;
    char* tmp = NULL;
    PalEvent* pal_event = NULL;

    if(fgets(s, 2048, file) == NULL)
		return NULL;

    /* first word is the date string */
    sscanf(s, "%s", date_string);

    /* the rest is the descriptive text */
    text_string = g_strdup(s+strlen(date_string));

    /* a roundabout way of making date_string uppercase.  The keys
       in hashtable must be uppercase, but pal's .pal files are
       case insensitive. */
    g_strstrip(date_string);
    tmp = g_ascii_strup(date_string, -1);
    sscanf(tmp, "%s", date_string);
    free(tmp);

    g_strstrip(text_string);

    pal_event = pal_event_copy(event_head);
    /* check for a valid date_string */
    if(!parse_event( pal_event, date_string )) {
		char* file = g_path_get_basename(filename);
		pal_output_error("ERROR: Invalid date string.\n");
		pal_output_error(  "       %s: %s\n", "FILE", file);
		pal_output_error(  "       %s: %s\n", "LINE", s);
		free(file);

		/* copy bad line */
		if(out_file != NULL)
		    fputs(s, out_file);

		free(text_string);
		pal_event_free( pal_event );
		return NULL;
    }

    /* require a description for a event */
    if(strlen(text_string) == 0) {
		char* file = g_path_get_basename(filename);
		pal_output_error("ERROR: Event description missing.\n");
		pal_output_error(  "       %s: %s\n", "FILE", file);
		pal_output_error(  "       %s: %s\n", "LINE", s);
		free(file);

		/* copy bad line */
		if(out_file != NULL)
		    fputs(s, out_file);

		free(text_string);
		pal_event_free( pal_event );
		return NULL;
    }

    /* check if text if UTF-8 */
    if(!g_utf8_validate(text_string, -1, NULL))
	pal_output_error("ERROR: Event text '%s' is not ASCII or UTF-8 in file %s.\n",
			 text_string, filename);

    /* Sanity checks */
    if( pal_event->period_count != 1 && !pal_event->start_date ) {
		char* file = g_path_get_basename(filename);
		time_t currenttime = time(NULL);
        pal_event->start_date = localtime(&currenttime);
        pal_event->end_date = g_date_new_dmy(1,1,3000);

        pal_output_error("ERROR: Event with count has no start date\n");
		pal_output_error(  "       %s: %s\n", "FILE", file);
		pal_output_error(  "       %s: %s\n", "LINE", s);
    }
    pal_event->start_time = pal_input_get_time(text_string, 1);
    pal_event->end_time   = pal_input_get_time(text_string, 2);
    tmp = g_strstr_len(text_string, -1, " %%");

    if (tmp == NULL) {
		tmp = g_strstr_len(text_string, -1, "%%");
    }

    if (tmp != NULL) {
		*tmp = '\0';
    }

    pal_event->text = g_strdup(text_string);
    pal_event->date_string = g_strdup(date_string);


    if(out_file != NULL) {
	/* don't print to out_file if event should be expunged */
	if(should_be_expunged(pal_event)) {
	    if(settings->verbose)
			g_printerr("%s: %s", "Expunged", s);

	    pal_event_free(pal_event);
	    free(text_string);
	    return NULL;
	}

	/* don't print to out_file if event should be deleted */
	else if(del_event != NULL &&
		strcmp(pal_event->date_string, del_event->date_string) == 0 &&
		strcmp(pal_event->text,        del_event->text)        == 0)
	{
	    pal_event_free(pal_event);
	    free(text_string);
	    return del_event;
	}
	else /* otherwise, print to out_file */
	    fputs(s, out_file);
    }


    free(text_string);
    return pal_event;
}



/* checks if file is a global */
static gboolean pal_input_file_is_global(const char* filename)
{
    if(strncmp(filename, PREFIX "/share/pal", strlen(PREFIX "/share/pal")) == 0)
	return TRUE;
    return FALSE;
}



/* loads a pal calendar file, returns the number of events loaded into hashtable */
static int load_file(char* filename, FILE* file, int filecount, gboolean hide, int color)
{
    int eventcount = 0;
    PalEvent* event_head;
    FILE *out_file = NULL;
    char* out_filename = NULL;


    g_strstrip(filename);
    out_filename = g_strconcat(filename, ".paltmp", NULL);

    /* if -x is used and the file isn't a global calendar, expunge */
    if(settings->expunge > 0 && !pal_input_file_is_global(out_filename))
    {
	out_file = fopen(out_filename, "w");
	if(out_file == NULL)
	{
	    pal_output_error("ERROR: Can't write file: %s\n", out_filename);
	    pal_output_error(  "       %s\n", "File will not be expunged: %s", filename);
	}
    }

    pal_input_skip_comments(file, out_file);
    event_head = pal_input_read_head(file, out_file, filename);

    if(event_head != NULL)
    {
	event_head->color = color;
	event_head->file_num = filecount;
	event_head->hide = hide;

	while(1)
	{
	    GList* days_events = NULL;
	    PalEvent* pal_event = NULL;

	    pal_input_skip_comments(file, out_file);
	    pal_event = pal_input_read_event(file, out_file, filename, event_head, NULL);

	    if(pal_event == NULL && pal_input_eof(file))
		break;

	    if(pal_event != NULL)
	    {
		char* key = pal_event->key;
		eventcount++;

		/* if no list exists for that key, make new list */
		if(g_hash_table_lookup(ht, key) == NULL)
		{
		    days_events = NULL;
		    days_events = g_list_append(days_events, pal_event);
		    g_hash_table_insert(ht,g_strdup(key),days_events);
		}
		else /* else, append to current list */
		{
		    days_events = g_hash_table_lookup(ht,key);
		    days_events = g_list_append(days_events,pal_event);
		}
	    }

	}
    }

    if(out_file != NULL)
    {
	fclose(out_file);
	if(rename(out_filename, filename) != 0)
	    pal_output_error("ERROR: Can't rename %s to %s\n", out_filename, filename);
    }

    free( out_filename );
    pal_event_free(event_head);
    return eventcount;
}



/* gets the pal file to load.  From "file", it looks for a pal
 * calendar file locally or globally to load and puts the path of that
 * file in pal_file.  It returns true if successful. */
static gboolean get_file_to_load(char* file, char* pal_file, gboolean show_error)
{
    if(g_path_is_absolute(file))
    {
	if(! g_file_test(file, G_FILE_TEST_EXISTS) || g_file_test(file, G_FILE_TEST_IS_DIR))
	{
	    if(show_error)
		pal_output_error("ERROR: File doesn't exist: %s\n", file);
	    return FALSE;
	}
	else
	    sprintf(pal_file, file);
    }

    else
    {
	/* if a relative path, try looking in the path found in settings->conf_file */
	char* dirname = g_path_get_dirname(settings->conf_file);
	sprintf(pal_file, "%s/%s", dirname, file);

	/* if that doesn't work, try looking in PREFIX/share/pal */
	if(! g_file_test(pal_file, G_FILE_TEST_EXISTS) || g_file_test(pal_file, G_FILE_TEST_IS_DIR))
	    sprintf(pal_file, PREFIX "/share/pal/%s", file);

	/* if that doesn't work, print message */
	if(! g_file_test(pal_file, G_FILE_TEST_EXISTS) || g_file_test(pal_file, G_FILE_TEST_IS_DIR))
	{
	    char other_file[2048];
	    sprintf(other_file, "%s/%s", dirname, file);
	    if(show_error)
	    {
		pal_output_error("ERROR: Can't find file.  I tried %s and %s.\n",
				 other_file, pal_file);
		exit(1); /* if we don't exit, this error gets buried
			  * when -m is used. */
	    }

	    free(dirname);
	    return FALSE;
	}

	free(dirname);
    }

    return TRUE;
}


/* opens a file for reading.  Prints error if can't read file.
 * Returns NULL if can't read, returns a FILE pointer if reading file.
 * fclose() should be called on the returned pointer when done reading
 * the file. */
static FILE* get_file_handle(char* filename, gboolean show_error)
{
    FILE* file = fopen(filename, "r");

    if(settings->verbose)
	g_printerr("Reading: %s\n", filename);

    if(file == NULL && show_error)
	pal_output_error("ERROR: Can't read file: %s\n", filename);

    return file;
}


/* loads calendar files and settings from a pal.conf file */
GHashTable*
load_files()
{
    char s[2048];
    char text[2048];
    FILE* file = NULL;
    unsigned int filecount=0, eventcount=0;

    ht = g_hash_table_new(g_str_hash,g_str_equal);

    if(settings->verbose) {
		if(settings->expunge >= 0)
	    	g_printerr("Looking for data to expunge.\n");
    }

    file = get_file_handle(settings->conf_file, FALSE);

    if(file == NULL) {
		if(settings->specified_conf_file) {
		    pal_output_error("ERROR: Can't open file: %s\n", settings->conf_file);
		    return ht;
		} else { /* didn't specify conf file, and couldn't find conf file: create a default one */
		    FILE* out_file;
		    char* out_dirname;
		    char* out_path;
		    int c;

		    out_dirname = g_strconcat(g_get_home_dir(), "/.pal", NULL);
		    out_path    = g_strconcat(out_dirname, "/pal.conf", NULL);

		    pal_output_error("NOTE: Creating %s\n", out_path);
		    pal_output_error("NOTE: Edit ~/.pal/pal.conf to change how and if certain events are displayed.\n",
				     out_path);

		    /* create directory if it doesn't exist */
		    if(!g_file_test(out_dirname, G_FILE_TEST_IS_DIR)) {
				if(mkdir(out_dirname, 0755) != 0) {
			    	pal_output_error("ERROR: Can't create directory: %s\n", out_dirname);
			    	return ht;
				}
		    }

		    /* attempt to copy /etc/pal.conf to ~/.pal/pal.conf */
		    file = fopen("/etc/pal.conf", "r");

		    /* if not found, try PREFIX/share/pal/pal.conf instead */
		    /* NOTE: This is will be removed in the future */
		    if(file == NULL)
				file = fopen(PREFIX "/share/pal/pal.conf", "r");


		    if(file == NULL) {
				pal_output_error("ERROR: Can't open file: /etc/pal.conf\n");
				pal_output_error("ERROR: Can't open file: " PREFIX "/share/pal/pal.conf\n");
				pal_output_error("ERROR: This indicates an improper installation.\n");
				return ht;
		    }

		    out_file = fopen(out_path, "w");
		    if(out_file == NULL) {
				pal_output_error("ERROR: Can't create/write file: %s\n", out_file);
				return ht;
		    }

		    /* do copy */
		    c = fgetc(file);
		    while(c != EOF) {
				fputc(c,out_file);
				c=fgetc(file);
		    }

		    fclose(out_file);
		    fclose(file);

		    free(out_dirname);
		    free(out_path);

		    /* open file to read it as usual */
		    file = fopen(settings->conf_file, "r");
		}

    } /* done opening/creating file */

    /* if using -p, load that .pal file now. */
    if(settings->pal_file != NULL) {
		char pal_file[16384];
		FILE* pal_file_handle = NULL;

		if(!get_file_to_load(settings->pal_file, pal_file, FALSE))
		    sprintf(pal_file, settings->pal_file);

		pal_file_handle = get_file_handle(pal_file, TRUE);
		if(pal_file_handle != NULL) {
		    eventcount += load_file(pal_file, pal_file_handle, filecount, FALSE, -1);
		    fclose(pal_file_handle);
		    filecount++;
		}
    }


    while(fgets(s, 2048, file) != NULL) {
		char pal_file[16384];
		char color[16384];
		int int_color = -1;
		int i,j;
		color[0] = '\0';
		g_strstrip(s);

		if(sscanf(s, "file %s (%[a-z])\n", text, color) == 2 ||
		    sscanf(s, "file %s\n", text) == 1) {
			    FILE* pal_file_handle = NULL;
			    gboolean hide = FALSE;

			    /* skip this line if we're using -p */
			    if(settings->pal_file != NULL)
				continue;

			    if(sscanf(s, "file_hide %s (%[a-z])\n", text, color) == 2)
				hide = TRUE;
			    else if(sscanf(s, "file_hide %s\n", text) == 1)
				hide = TRUE;

			    if(color[0] != '\0') {
					if(int_color_of(color) != -1)
					    int_color = int_color_of(color);

					if(int_color == -1) {
					    pal_output_error("ERROR: Invalid color '%s' in file %s.",
							     color, settings->conf_file);
					    pal_output_error("\n       %s %s\n", "Valid colors:", "black, red, green, yellow, blue, magenta, cyan, white");
					}
			    }

			    if(get_file_to_load(text, pal_file, TRUE)) {
					pal_file_handle = get_file_handle(pal_file, TRUE);
					if(pal_file_handle != NULL) {
					    /* assign events that are the "default" color to
					     * have a color of -1 the output code will apply
					     * the default color to events (since we might not
					     * have read in what the default color is yet. */
					    eventcount += load_file(pal_file, pal_file_handle, filecount, hide, int_color);
					    fclose(pal_file_handle);
					    filecount++;
				}
			}
		} else if(sscanf(s, "date_fmt %s",text) == 1) {
		    free(settings->date_fmt);
		    settings->date_fmt = g_strdup(g_strstrip(&s[9]));
		} else if(strcmp(s, "week_start_monday") == 0)
		    settings->week_start_monday = TRUE;
		else if(strcmp(s, "show_weeknum") == 0)
		    settings->show_weeknum = TRUE;
		else if(strcmp(s, "reverse_order") == 0)
		    settings->reverse_order = TRUE;
		else if(strcmp(s, "cal_on_bottom") == 0)
		    settings->cal_on_bottom = TRUE;
		else if(strcmp(s, "no_columns") == 0)
		    settings->no_columns = TRUE;
		else if(strcmp(s, "hide_event_type") == 0)
		    settings->hide_event_type = TRUE;
		else if(strcmp(s, "compact_list") == 0)
		    settings->compact_list = TRUE;
		else if(sscanf(s, "compact_date_fmt %s",text) == 1) {
		    free(settings->compact_date_fmt);
		    settings->compact_date_fmt = g_strdup(g_strstrip(&s[17]));
		} else if(sscanf(s, "event_color %s",text) == 1) {
		    int ec = int_color_of(text);
		    if(ec == -1) {
			    pal_output_error("%s\n", "ERROR: Invalid color '%s' in file %s.",
					     color, settings->conf_file);
			    pal_output_error("       %s %s\n", "Valid colors:", "black, red, green, yellow, blue, magenta, cyan, white");
		    } else
				settings->event_color = ec;
		}
		else if(sscanf(s, "default_range %d-%d", &i, &j) == 2) {
		    /* check if -r was used */
		    if(!settings->range_arg) {
			settings->range_neg_days = i;
			settings->range_days = j;
		    }
		} else if(sscanf(s, "default_range %d", &i) == 1) {
		    if(!settings->range_arg)
				settings->range_days = i;
		} else if(*s != '#' && *s != '\0') {
	        /* ignore empty lines or comments */
		    pal_output_error("ERROR: Invalid line (File: %s, Line text: %s)\n", settings->conf_file, s);
		}

    }
    fclose(file);
    if(settings->verbose)
		g_printerr("Done reading data (%d events, %d files).\n\n", eventcount, filecount);
    return ht;
}
