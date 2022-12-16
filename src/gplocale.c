/* GNUPLOT - gplocale.c */

/*[
 * Copyright 1999, 2004   Lars Hecking
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
]*/

#include <string.h>

#include "gplocale.h"
#include "stdfn.h"
#include "command.h"
#include "term_api.h"
#include "util.h"

/* Day and month names controlled by 'set locale'.
 * These used to be defined in national.h but internationalization via locale
 * is now a bit more common than it was last century.
 */
char full_month_names[12][32] = { 
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};
char abbrev_month_names[12][8] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
char full_day_names[7][32] = {
    "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"
};
char abbrev_day_names[7][8] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

char *
locale_handler(int action, char *newlocale)
{
    struct tm tm;
    int i;

    switch(action) {
    case ACTION_CLEAR:
    case ACTION_INIT:
	free(time_locale);
#ifdef HAVE_LOCALE_H
	setlocale(LC_TIME, "");
	setlocale(LC_CTYPE, "");
	time_locale = gp_strdup(setlocale(LC_TIME,NULL));
#else
	time_locale = gp_strdup(INITIAL_LOCALE);
#endif
	break;

    case ACTION_SET:
#ifdef HAVE_LOCALE_H
	if (setlocale(LC_TIME, newlocale)) {
	    free(time_locale);
	    time_locale = gp_strdup(setlocale(LC_TIME,NULL));
	} else {
	    int_error(c_token, "Locale not available");
	}

	/* we can do a *lot* better than this ; eg use system functions
	 * where available; create values on first use, etc
	 */
	memset(&tm, 0, sizeof(struct tm));
	for (i = 0; i < 7; ++i) {
	    tm.tm_wday = i;		/* hope this enough */
	    strftime(full_day_names[i], sizeof(full_day_names[i]), "%A", &tm);
	    strftime(abbrev_day_names[i], sizeof(abbrev_day_names[i]), "%a", &tm);
	}
	for (i = 0; i < 12; ++i) {
	    tm.tm_mon = i;		/* hope this enough */
	    strftime(full_month_names[i], sizeof(full_month_names[i]), "%B", &tm);
	    strftime(abbrev_month_names[i], sizeof(abbrev_month_names[i]), "%b", &tm);
	}
#else
	time_locale = gp_realloc(time_locale, strlen(newlocale) + 1, "locale");
	strcpy(time_locale, newlocale);
#endif /* HAVE_LOCALE_H */
	break;

    case ACTION_SHOW:
#ifdef HAVE_LOCALE_H
	fprintf(stderr, "\tgnuplot LC_CTYPE   %s\n", setlocale(LC_CTYPE,NULL));
	fprintf(stderr, "\tgnuplot encoding   %s\n", encoding_names[encoding]);
	fprintf(stderr, "\tgnuplot LC_TIME    %s\n", setlocale(LC_TIME,NULL));
	fprintf(stderr, "\tgnuplot LC_NUMERIC %s\n", numeric_locale ? numeric_locale : "C");
#else
	fprintf(stderr, "\tlocale is \"%s\"\n", time_locale);
#endif
	break;

    case ACTION_GET:
    default:
	break;
    }

    return time_locale;
}
