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


#include <sys/ioctl.h> /* get # columns for terminal */
#include <time.h>
#include <string.h>
#include <locale.h>
#include <sys/types.h> /* FreeBSD, regex.h needs this */
#include <regex.h>	   /* regular expressions */
#include <glib.h>
#include <assert.h>

#include <ncurses.h>

#include <getopt.h>

#include "output.h"
#include "main.h"
#include "input.h"
#include "event.h"

#include "rl.h"
#include "html.h"
#include "latex.h"
#include "search.h"

#include "manage.h"

Settings* settings;
GHashTable* ht;		/* ht holds the loaded events */

/* Returns 1 is the date is valid, 0 otherwise
*/
static int
valid_date(int day, int month, int year)
{
	if(day < 1 || day > 31 || month < 1 || month > 12 || year < 1)
	    return 0;

	/* FIXME: Don't allow one-time events on leap day in years
	 * that leap day doesn't exist */
	if(month == 2 && day > 29)
	    return 0;

	/* 30 days in april, june, sept, nov */
	if((month == 4 || month == 6 || month == 9 || month == 11) &&
	   day > 30)
	    return 0;

    return 1;
}


/* prints the events on the dates from the starting_date to
 * starting_date+window */
static void
view_range(struct tm* starting_date, int window)
{
	if(settings->reverse_order) {
		starting_date->tm_mday += window-1;
		mktime(starting_date);
	}

	for(int i = 0; i < window; i++) {
		pal_output_date(starting_date,0,-1);

		if(settings->reverse_order) {
			starting_date->tm_mday -= 1;
		} else {
			starting_date->tm_mday += 1;
		}
		mktime(starting_date);
	}
}



/* Returns a struct tm object for the given key (in_string) */
/* This function only checks the one-time date events (not todo, or
 * recurring events).  It returns NULL on a failure and can optionally
 * print an error message on failure. */
struct tm*
get_query_date(char* in_string, gboolean show_error)
{
	struct tm* to_show = NULL;
	char* date_string = g_ascii_strdown(in_string, -1);

	if(date_string == NULL)
		return NULL;

	g_strstrip(date_string);

	time_t currenttime = time(NULL);
	to_show = localtime(&currenttime);

	/* these could be better... */
	if(strncmp(date_string, "tomorrow", strlen("tomorrow")) == 0) {
		to_show->tm_mday += 1;
		free(date_string);
		return to_show;
	}

	if(strncmp(date_string, "yesterday", strlen("yesterday")) == 0) {
		to_show->tm_mday -= 1;
		mktime(to_show);
		free(date_string);
		return to_show;
	}

	if(strncmp(date_string, "today", strlen("today")) == 0) {
		free(date_string);
		return to_show;
	}

	if(strncmp(date_string, "mo", strlen("mo")) == 0 ||
	   strncmp(date_string, "next mo", strlen("next mo")) == 0) {
		if(to_show->tm_wday <= 1)
			to_show->tm_mday += 1 - to_show->tm_wday;
		else
			to_show->tm_mday += 7 - to_show->tm_wday;
		mktime(to_show);
		free(date_string);
		return to_show;
	}

	if(strncmp(date_string, "tu", strlen("tu")) == 0 ||
	   strncmp(date_string, "next tu", strlen("next tu")) == 0) {
		if(to_show->tm_wday <= 2)
			to_show->tm_mday += 2 - to_show->tm_wday;
		else
			to_show->tm_mday += 8 - to_show->tm_wday;
		mktime(to_show);
		free(date_string);
		return to_show;
	}

	if(strncmp(date_string, "we", strlen("we")) == 0 ||
	   strncmp(date_string, "next we", strlen("next we")) == 0) {
		if(to_show->tm_wday <= 3)
			to_show->tm_mday += 3 - to_show->tm_wday;
		else
			to_show->tm_mday += 9 - to_show->tm_wday;
		mktime(to_show);
		free(date_string);
		return to_show;
	}

	if(strncmp(date_string, "th", strlen("th")) == 0 ||
	   strncmp(date_string, "next th", strlen("next th")) == 0) {
		if(to_show->tm_wday <= 4)
			to_show->tm_mday += 4 - to_show->tm_wday;
		else
			to_show->tm_mday += 10 - to_show->tm_wday;
		mktime(to_show);
		free(date_string);
		return to_show;
	}

	if(strncmp(date_string, "fr", strlen("fr")) == 0 ||
	   strncmp(date_string, "next fr", strlen("next fr")) == 0) {
		if(to_show->tm_wday <= 5)
			to_show->tm_mday += 5 - to_show->tm_wday;
		else
			to_show->tm_mday += 11 - to_show->tm_wday;
		mktime(to_show);
		free(date_string);
		return to_show;
	}

	if(strncmp(date_string, "sa", strlen("sa")) == 0 ||
	   strncmp(date_string, "next sa", strlen("next sa")) == 0) {
		to_show->tm_mday += 6 - to_show->tm_wday;
		mktime(to_show);
		free(date_string);
		return to_show;
	}

	if(strncmp(date_string, "su", strlen("su")) == 0 ||
	   strncmp(date_string, "next su", strlen("next su")) == 0) {
		to_show->tm_mday += 7 - to_show->tm_wday;
		mktime(to_show);
		free(date_string);
		return to_show;
	}

	if(strncmp(date_string, "last mo", strlen("last mo")) == 0) {
		do to_show->tm_mday -= 1; while(to_show->tm_wday != 1);
		free(date_string);
		return to_show;
	}
	if(strncmp(date_string, "last tu", strlen("last tu")) == 0) {
		do to_show->tm_mday -= 1; while(to_show->tm_wday != 2);
		free(date_string);
		return to_show;
	}
	if(strncmp(date_string, "last we", strlen("last we")) == 0) {
		do to_show->tm_mday -= 1; while(to_show->tm_wday != 3);
		free(date_string);
		return to_show;
	}
	if(strncmp(date_string, "last th", strlen("last th")) == 0) {
		do to_show->tm_mday -= 1; while(to_show->tm_wday != 4);
		free(date_string);
		return to_show;
	}
	if(strncmp(date_string, "last fr", strlen("last fr")) == 0) {
		do to_show->tm_mday -= 1; while(to_show->tm_wday != 5);
		free(date_string);
		return to_show;
	}
	if(strncmp(date_string, "last sa", strlen("last sa")) == 0) {
		do to_show->tm_mday -= 1; while(to_show->tm_wday != 6);
		free(date_string);
		return to_show;
	}
	if(strncmp(date_string, "last su", strlen("last su")) == 0) {
		do to_show->tm_mday -= 1; while(to_show->tm_wday != 7);
		free(date_string);
		return to_show;
	}

	/* use regexs here for easier localization */
	regex_t preg;

	regcomp(&preg, "^[0-9]+ days away$", REG_ICASE|REG_NOSUB|REG_EXTENDED);

	if(regexec(&preg, date_string, 0, NULL, 0)==0) {
		char* ptr = date_string;
		int date_offset = 0;
		while(!g_ascii_isdigit(*ptr) && ptr != NULL)
		ptr = g_utf8_find_next_char(ptr, NULL);

		sscanf(ptr, "%d", &date_offset);

		to_show->tm_mday += date_offset;
		free(date_string);
		regfree(&preg);
		return to_show;
	}

	regfree(&preg);
	regcomp(&preg, "^[0-9]+ days ago$", REG_ICASE|REG_NOSUB|REG_EXTENDED);

	if(regexec(&preg, date_string, 0, NULL, 0)==0) {
		char* ptr = date_string;
		int date_offset = 0;
		while(!g_ascii_isdigit(*ptr) && ptr != NULL)
		ptr = g_utf8_find_next_char(ptr, NULL);

		sscanf(ptr, "%d", &date_offset);

		to_show->tm_mday -= date_offset;
		free(date_string);
		regfree(&preg);
		return to_show;
	}
	regfree(&preg);


	if(g_ascii_isdigit(*(date_string))) { /* if it begins with a digit ... */
		char* ptr = date_string;

		while(g_ascii_isdigit(*ptr))
			ptr++;

		/* ... and if the string is entirely made up of digits */
		if(*ptr == '\0') {
			int query_date_int = -1;

			sscanf(date_string, "%d", &query_date_int);


			/* check for 0, negative number, or excessive preceeding 0's */
			if(!(query_date_int <= 0 ||
			(*date_string == '0' && *(date_string+1) == '0' &&
			 *(date_string+2) == '0' && *(date_string+3) == '0') ||
			(*(date_string+4) == '0' && *(date_string+5) == '0') ||
			(*(date_string+6) == '0' && *(date_string+7) == '0'))) {

				if(query_date_int < 32) {
					if(query_date_int < to_show->tm_mday)
						to_show->tm_mon += 1;

					if(valid_date(query_date_int, to_show->tm_mon, to_show->tm_year)) {
						to_show->tm_mday = query_date_int;
						mktime(to_show);
						free(date_string);
						return to_show;
					}
				} else if(query_date_int < 1232) {
					int day, month;

					day = query_date_int % 100;
					month = query_date_int / 100;

					if(day > 0 && day < 32 && month > 0 && month < 13) {
						if(month < to_show->tm_mon)
							to_show->tm_year += 1;
						if(day	 < to_show->tm_mday && month == to_show->tm_mon)
							to_show->tm_year += 1;

						if(valid_date(day, month, to_show->tm_year)) {
							to_show->tm_mday =  day;
							to_show->tm_mon =  month;
							to_show->tm_year =  month;
							mktime(to_show);

							free(date_string);
							return to_show;
						}
					}
				} else if(query_date_int > 10000) {
					int day, month, year;

					day = query_date_int % 100;
					month = (query_date_int % 10000 - day) / 100;
					year = query_date_int / 10000;

					if(day > 0 && day < 32 && month > 0 && month < 13 && year > 0) {
						if(valid_date( day, month, year)) {
							to_show->tm_mday = day;
							to_show->tm_mon = month;
							to_show->tm_year = year;
							free(date_string);
							return to_show;
						}
					}
				}
			}
		}
	}

	/* glib is last resort, but don't let a few things get to it... */
	if(!((*date_string == '0' && *(date_string+1) == '0' &&
	  *(date_string+2) == '0' && *(date_string+3) == '0') ||
	 (*(date_string+4) == '0' && *(date_string+5) == '0') ||
	 (*(date_string+6) == '0' && *(date_string+7) == '0'))) {
		g_date_set_parse(to_show, date_string);

		if(valid_date(to_show->tm_mday,to_show->tm_mon,to_show->tm_year)) {
			free(date_string);
			return to_show;
		}
	}

	if(show_error) {
		/* if we got here, there was an error */
		pal_output_error("ERROR: The following date is not valid: %s\n", date_string);
		pal_output_error(  "	   %s\n", "Valid date formats include:");
		pal_output_error(  "	   %s '%s', '%s', '%s',\n", "dd, mmdd, yyyymmdd,", "yesterday", "today", "tomorrow");
		pal_output_error(  "	   %s\n", "'n days away', 'n days ago',");
		pal_output_error(  "	   %s\n", "first two letters of weekday,");
		pal_output_error(  "	   %s\n", "'next ' followed by first two letters of weekday,");
		pal_output_error(  "	   %s\n", "'last ' followed by first two letters of weekday,");
		pal_output_error(  "	   %s\n", "'1 Jan 2000', 'Jan 1 2000', etc.");
	}
	free(to_show);
	free(date_string);
	return NULL;
}


/* determines what should be dispalyed if -r, -s, -d are used */
static void
view_details(void)
{
	struct tm* to_show = settings->query_date;


	if(settings->search_string != NULL &&
	   settings->range_days == 0 &&
	   settings->range_neg_days == 0) {
		g_print("\n");
		pal_output_fg(BRIGHT, RED, "> ");
		pal_output_wrap("NOTE: You can use -r to specify the range of days to search.  By default, pal searches days within one year of today.",2,2);
		settings->range_days = 365;
	}

	/* if -r and -s isn't used */
	if(settings->range_days == 0 &&
	   settings->range_neg_days == 0 &&
	   settings->search_string == NULL) {
		/* if -d is used, show that day.  Otherwise, show nothing */
		if(to_show != NULL)
			pal_output_date(to_show, TRUE, -1);
		/* if -r or -s is used, show range of dates relative to -d */
	} else if(settings->range_days	   != 0 ||
			settings->range_neg_days != 0) {
		struct tm* starting_date = NULL;

	/* if -d isn't used, start from current date */
	if(to_show == NULL) {
		time_t currenttime = time(NULL);
		starting_date = localtime(&currenttime);
	} else { /* otherwise, start from date specified */
		starting_date = to_show;
	}

	starting_date->tm_mday -= settings->range_neg_days;

	if(settings->search_string == NULL)
		view_range(starting_date, settings->range_neg_days + settings->range_days);
	else
		pal_search_view(settings->search_string, starting_date, settings->range_neg_days + settings->range_days, FALSE);

	}
}



static int
parse_args(int argc, char** argv)
{
	char *token, *str, *tofree;
	int opt;
	while (1) {

		static struct option long_options[] = {
		  {"mail",     no_argument,   0, 'a'},
		  {"color",    no_argument,   0, 'b'},
		  {"version",  no_argument,   0, 'e'},
		  {"nocolor",  no_argument,   0, 'g'},
		  {"html",     no_argument,   0, 't'},
		  {"latex",    no_argument,   0, 'l'},
		  {"help",     no_argument,   0, 'h'},
		  {0, 0, 0, 0}
		};

		int option_index = 0;

		opt = getopt_long(argc, argv, "c:d:f:lmp:r:s:u:vh", long_options, &option_index);

 		/* Detect the end of the options. */
		if (opt == -1)
			break;

		switch (opt) {
			case 'a': //--mail
				set_colorize(-2); /* overrides a later --color argument */
				settings->mail = TRUE;
				break;
			case 'b': //--color
				set_colorize(1);
				break;
			case 'c':
				settings->cal_lines = strtoull(optarg, NULL, 10);
				break;
			case 'd':
				settings->query_date = get_query_date(optarg, TRUE);
				if(settings->query_date == NULL)
					pal_output_error("NOTE: Use quotes around the date if it has spaces.\n");
				break;
			case 'e': //--version
				printf("pal %s\n", PAL_VERSION);
				printf("Compiled with prefix: %s\n", PREFIX);
				exit(0);
				break;
			case 'f':
				free(settings->conf_file);
				settings->conf_file = strdup(optarg);
				settings->specified_conf_file = TRUE;
				break;
			case 'g': //--nocolor
				set_colorize(0);
				break;
			case 'l': //--latex
				settings->latex_out = TRUE;
				break;
			case 'm':
				settings->manage_events = TRUE;
				break;
			case 'p':
				settings->pal_file = strdup(optarg);
				break;
			case 'r':
				tofree = str = strdup(optarg);
				if((token = strsep(&str, "-"))) {
					settings->range_days = strtoull(token, NULL, 10);
					if (str) {
						settings->range_neg_days = strtoull(token, NULL, 10);
						settings->range_days = strtoull(str, NULL, 10);
					}
					free(tofree);
				}
				settings->range_arg = TRUE;
				break;
			case 's':
				settings->search_string = g_strdup(optarg);
				break;
			case 't': //--html
				settings->html_out = TRUE;
				break;
			case 'u':
				free(settings->conf_file);
				settings->conf_file = g_strconcat("/home/", optarg, "/.pal/pal.conf", NULL);
				settings->specified_conf_file = TRUE;
				break;
			case 'v':
				settings->verbose = TRUE;
				break;
			case 'x':
				settings->expunge = strtoull(optarg, NULL, 10);
				break;
			case 'h': //fallthrough
			default:
				g_print("%s %s - %s\n", "pal", PAL_VERSION, "Copyright (C) 2006, Scott Kuhl");

				g_print("  ");
				pal_output_wrap("pal is licensed under the GNU General Public License and has NO WARRANTY.", 2, 2);
				g_print("\n");

				pal_output_wrap(" -d date		 Show events on the given date.  Valid formats for date include: dd, mmdd, yyyymmdd, 'Jan 1 2000'.	Run 'man pal' for a list of all valid formats.", 0, 16);
				pal_output_wrap(" -r n		 Display events within n days after today or a date used with -d. (default: n=0, show nothing)", 0, 16);
				pal_output_wrap(" -r p-n		 Display events within p days before and n days after today or a date used with -d.", 0, 16);
				pal_output_wrap(" -s regex	 Search for events matching the regular expression. Use -r to select range of days to search.", 0, 16);
				pal_output_wrap(" -x n		 Expunge events that are n or more days old.", 0, 16);

				pal_output_wrap(" -c n		 Display calendar with n lines. (default: 5)", 0, 16);
				pal_output_wrap(" -f file		 Load 'file' instead of ~/.pal/pal.conf", 0, 16);
				pal_output_wrap(" -u username  Load /home/username/.pal/pal.conf", 0, 16);
				pal_output_wrap(" -p palfile	 Load *.pal file only (overrides files loaded from pal.conf)", 0, 16);
				pal_output_wrap(" -m			 Add/Modify/Delete events interactively.", 0, 16);
				pal_output_wrap(" --color		 Force colors, regardless of terminal type.", 0, 16);
				pal_output_wrap(" --nocolor	 Force no colors, regardless of terminal type.", 0, 16);
				pal_output_wrap(" --mail		 Generate output readable by sendmail.", 0, 16);
				pal_output_wrap(" --html		 Generate HTML calendar.  Set size of calendar with -c.", 0, 16);
				pal_output_wrap(" --latex		 Generate LaTeX calendar.  Set size of calendar with -c.", 0, 16);
				pal_output_wrap(" -v			 Verbose output.", 0, 16);
				pal_output_wrap(" --version	 Display version information.", 0, 16);
				pal_output_wrap(" -h, --help	 Display this help message.", 0, 16);

				g_print("\n");
				pal_output_wrap("Type \"man pal\" for more information.", 0, 16);
				exit(0);
		}
	}
return 0;
}



/* used when the pal is finished running and the hashtable entries
 * need to be properly freed */
static void
hash_table_free_item(gpointer key, gpointer value, gpointer user_data)
{
	GList* list = (GList*) value;
	GList* prev = list;

	free(key);

	/* free the list for this hashtable key */
	while(g_list_length(list) != 0) {
		pal_event_free((PalEvent*) list->data);
		list = g_list_next(list);
	}

	g_list_free(prev);
}


/* free the hashtable */
static void
pal_main_ht_free(void)
{
	if(ht != NULL) {
		g_hash_table_foreach(ht, (GHFunc) hash_table_free_item, NULL);
		g_hash_table_destroy(ht);
		ht = NULL;
	}
}

/* free, and reload the hashtable and settings from pal.conf and
 * calendar files */
void
pal_main_reload(void)
{
	if(settings->verbose)
		g_printerr("Reloading events and settings.\n");

	pal_main_ht_free();
	ht = load_files();
}

int
main(int argc, char** argv)
{
	const char *charset = NULL;
	time_t currenttime = time(NULL);
	struct tm *today = localtime(&currenttime);

    settings = malloc(sizeof(Settings));
	settings->cal_lines             = 5;
	settings->range_days            = 0;
	settings->range_neg_days        = 0;
	settings->range_arg             = 0;
	settings->search_string         = NULL;
	settings->verbose               = 0;
	settings->mail                  = 0;
	settings->query_date            = NULL;
	settings->expunge               = -1;
	settings->date_fmt              = g_strdup("%a %e %b %Y");
	settings->week_start_monday     = 0;
	settings->reverse_order         = 0;
	settings->cal_on_bottom         = 0;
	settings->specified_conf_file   = 0;
	settings->no_columns            = 0;
	settings->hide_event_type       = 0;
	settings->manage_events         = 0;
	settings->curses                = 0;
	settings->event_color           = BLUE;
	settings->pal_file              = NULL;
	settings->html_out              = 0;
	settings->latex_out             = 0;
	settings->compact_list          = 0;
	settings->term_cols             = 80;
	settings->term_rows             = 24;
	settings->compact_date_fmt      = g_strdup("%m/%d/%Y");
	settings->conf_file             = g_strconcat(g_get_home_dir(), "/.pal/pal.conf", NULL);
	settings->show_weeknum          = 0;

	//g_set_print_handler( pal_output_handler );
	//g_set_printerr_handler( pal_output_handler );

	//textdomain("pal");
	//bind_textdomain_codeset("pal", "utf-8");
	//if(setlocale(LC_MESSAGES, "") == NULL ||
	//   setlocale(LC_TIME, "") == NULL ||
	//   setlocale(LC_ALL, "") == NULL ||
	//   setlocale(LC_CTYPE, "") == NULL)
	//	pal_output_error("WARNING: Localization failed.\n");


#ifndef __CYGWIN__
	/* figure out the terminal width if possible */
	struct winsize wsz;
	if(ioctl(0, TIOCGWINSZ, &wsz) != -1) {
		settings->term_cols = wsz.ws_col;
		settings->term_rows = wsz.ws_row;
	}
#endif

	/* parse all the arguments */
	parse_args(argc, argv);

	//g_get_charset(&charset);
	//if(settings->verbose)
	//	g_printerr("Character set: %s\n", charset);


	ht = load_files();


	/* adjust settings if --mail is used */
	if(settings->mail) {
		char *pretty_date = asctime(today);

		printf("From: \"pal\" <pal>\n");
		printf("Content-Type: text/plain; charset=%s\n", charset);
		printf("Subject: [pal] %s\n\n", pretty_date);


		/* let the mail reader handle the line wrapping */
		settings->term_cols = -1;
	}

	if(settings->manage_events)
		pal_manage();


	if(!settings->html_out && !settings->latex_out) {

		if(!settings->cal_on_bottom) {
			pal_output_cal(settings->cal_lines, today);
			/* print a newline under calendar if we're printing other stuff */
			if(settings->cal_lines > 0 && (settings->range_days>0 ||
						   settings->range_neg_days>0 ||
						   settings->query_date != NULL))

				g_print("\n");

		}

		view_details(); /* prints results of -d,-r,-s */

		if(settings->cal_on_bottom) {
			/* print a new line over calendar if we've printed other stuff */
			if(settings->cal_lines > 0 && (settings->range_days>0 ||
						   settings->range_neg_days>0 ||
						   settings->query_date != NULL))
				g_print("\n");

			pal_output_cal(settings->cal_lines,today);
		}


 		/* end if not html_out and not latex_out */
		} else if(settings->html_out && settings->latex_out) {
			pal_output_error("ERROR: Can't use both --html and --latex.\n");
			return 1;
		} else if(settings->html_out)
			pal_html_out();
		else if(settings->latex_out)
			pal_latex_out();


	pal_main_ht_free();

	/* free settings */
	free(settings->date_fmt);
	free(settings->conf_file);
	free(settings->search_string);
	free(settings->compact_date_fmt);
	free(settings->pal_file);

	free(settings);

	return 0;
}
