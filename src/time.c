#ifndef lint
static char *RCSid() { return RCSid("$Id: time.c,v 1.10.2.1 2000/05/03 21:26:12 joze Exp $"); }
#endif

/* GNUPLOT - time.c */

/*[
 * Copyright 1986 - 1993, 1998   Thomas Williams, Colin Kelley
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


/* some systems may not implement time very well ; in particular,
 * things might break as the year 2000 approaches.
 * This module either adds a routine gstrptime() to read a formatted time,
 * augmenting the standard suite of time routines provided by ansi,
 * or it completely replaces the whole lot with a new set of routines,
 * which count time relative to the year 2000. Default is to use the
 * new routines. define USE_SYSTEM_TIME to use the system routines, at your
 * own risk. One problem in particular is that not all systems allow
 * the time with integer value 0 to be represented symbolically, which
 * prevents use of relative times.
 */


#include "gp_time.h"

#include "util.h"

/* build as a standalone test */

#ifdef TEST_TIME

# ifdef HAVE_SYS_TIMEB_H
#  include <sys/timeb.h>
# else
/* declare struct timeb */
extern int ftime(struct timeb *);
# endif				/* !HAVE_SYS_TIMEB_H */

# define int_error(x,y) fprintf(stderr, "Error: " y "\n")
# define int_warn(x,y) fprintf(stderr, "Warn: " y "\n")

/* need (only) these from plot.h */
# define ZERO_YEAR	2000
/* 1st jan, 2000 is a Saturday (cal 1 2000 on unix) */
# define JAN_FIRST_WDAY 6

/*  zero gnuplot (2000) - zero system (1970) */
# define SEC_OFFS_SYS	946684800.0

/* avg, incl. leap year */
# define YEAR_SEC	31557600.0

/* YEAR_SEC / 12 */
# define MON_SEC		2629800.0

# define WEEK_SEC	604800.0
# define DAY_SEC		86400.0

/* HBB 990826: moved definitions up here, to avoid 'extern' where
 * it is neither wanted nor needed */
char *abbrev_month_names[] =
{ "jan", "feb", "mar", "apr", "may", "jun", "jul",
  "aug", "sep", "oct", "nov", "dec"
};

char *full_month_names[] =
{ "January", "February", "March", "April", "May",
  "June", "July", "August", "September", "October",
  "November", "December"
};

char *abbrev_day_names[] =
{ "sun", "mon", "tue", "wed", "thu", "fri", "sat"};

char *full_day_names[] =
{ "Sunday", "Monday", "Tuesday", "Wednesday",
  "Thursday", "Friday", "Saturday"
};

#else /* TEST_TIME */

# include "setshow.h"		/* for month names etc */

#endif /* TEST_TIME */

static char *read_int __PROTO((char *s, int nr, int *d));


static char *
read_int(s, nr, d)
char *s;
int nr, *d;
{
    int result = 0;

    while (--nr >= 0 && *s >= '0' && *s <= '9')
	result = result * 10 + (*s++ - '0');

    *d = result;
    return (s);
}



#ifndef USE_SYSTEM_TIME

/* a new set of routines to completely replace the ansi ones
 * Use at your own risk
 */

static int gdysize __PROTO((int yr));

static int mndday[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static int xstrftime __PROTO((char *buf, size_t bufsz, const char *fmt, struct tm * tm));

/* days in year */
static int
gdysize(yr)
int yr;
{

    if (!(yr % 4)) {
	if ((!(yr % 100)) && yr % 400)
	    return (365);
	return (366);
    }
    return (365);
}


/* new strptime() and gmtime() to allow time to be read as 24 hour,
 * and spaces in the format string. time is converted to seconds from
 * year 2000.... */

char *
gstrptime(s, fmt, tm)
char *s;
char *fmt;
struct tm *tm;
{
    int yday, date;

    date = yday = 0;
    tm->tm_mday = 1;
    tm->tm_mon = tm->tm_hour = tm->tm_min = tm->tm_sec = 0;
    /* make relative times work (user-defined tic step) */
    tm->tm_year = ZERO_YEAR;

    /* we do not yet calculate wday or yday, so make them illegal
     * [but yday will be read by %j]
     */

    tm->tm_yday = tm->tm_wday = -1;

    while (*fmt) {
	if (*fmt != '%') {
	    if (*fmt == ' ') {
		/* space in format means zero or more spaces in input */
		while (*s == ' ')
		    ++s;
		++fmt;
		continue;
	    } else if (*fmt == *s) {
		++s;
		++fmt;
		continue;
	    } else
		break;		/* literal match has failed */
	}
	/* we are processing a percent escape */

	switch (*++fmt) {
	case 'b':		/* abbreviated month name */
	    {
		int m;
		for (m = 0; m < 12; ++m)
		    if (strncasecmp(s, abbrev_month_names[m], strlen(abbrev_month_names[m])) == 0) {
			s += strlen(abbrev_month_names[m]);
			goto found_abbrev_mon;
		    }
		/* get here => not found */
		int_warn(NO_CARET, "Bad abbreviated month name");
		m = 0;
	      found_abbrev_mon:
		tm->tm_mon = m;
		break;
	    }

	case 'B':		/* full month name */
	    {
		int m;
		for (m = 0; m < 12; ++m)
		    if (strncasecmp(s, full_month_names[m], strlen(full_month_names[m])) == 0) {
			s += strlen(full_month_names[m]);
			goto found_full_mon;
		    }
		/* get here => not found */
		int_warn(NO_CARET, "Bad full month name");
		m = 0;
	      found_full_mon:
		tm->tm_mon = m;
		break;
	    }

	case 'd':		/* read a day of month */
	    s = read_int(s, 2, &tm->tm_mday);
	    date++;
	    break;

	case 'm':		/* month number */
	    s = read_int(s, 2, &tm->tm_mon);
	    date++;
	    --tm->tm_mon;
	    break;

	case 'y':		/* year number */
	    s = read_int(s, 2, &tm->tm_year);
	    /* In line with the current UNIX98 specification by
	     * The Open Group and major Unix vendors,
	     * two-digit years 69-99 refer to the 20th century, and
	     * values in the range 00-68 refer to the 21st century.
	     */
	    if (tm->tm_year <= 68)
		tm->tm_year += 100;
	    date++;
	    tm->tm_year += 1900;
	    break;

	case 'Y':
	    s = read_int(s, 4, &tm->tm_year);
	    date++;
	    break;

	case 'j':
	    s = read_int(s, 3, &tm->tm_yday);
	    tm->tm_yday--;
	    date++;
	    yday++;
	    break;

	case 'H':
	    s = read_int(s, 2, &tm->tm_hour);
	    break;

	case 'M':
	    s = read_int(s, 2, &tm->tm_min);
	    break;

	case 'S':
	    s = read_int(s, 2, &tm->tm_sec);
	    break;

	/* read EPOCH data
	 * EPOCH is the std. unixtimeformat seconds since 01.01.1970 UTC
	 * actualy i would need a read_long (or what time_t is else)
	 *  aktualy this is not my idea       i got if from
	 * AlunDa Penguin Jones (21.11.97)
	 * but changed %t to %s because of strftime
	 * and fixed the localtime() into gmtime()
	 * maybe we should use ggmtime() ? but who choose double ????
	 * Walter Harms <WHarms@bfs.de> 26 Mar 2000
	 */

	case 's':
	    {
		/* time_t when; */
		int when;
		struct tm *tmwhen;
		s = read_int(s, 10, &when);
		tmwhen = gmtime((time_t*)&when);
		tmwhen->tm_year += 1900;
		*tm = *tmwhen;
		break;
	    }

	default:
	    int_warn(NO_CARET, "Bad time format in string");
	}
	fmt++;
    }

    FPRINTF((stderr, "read date-time : %02d/%02d/%d:%02d:%02d:%02d\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year, tm->tm_hour, tm->tm_min, tm->tm_sec));

    /* now check the date/time entered, normalising if necessary
     * read_int cannot read a -ve number, but can read %m=0 then decrement
     * it to -1
     */

#define S (tm->tm_sec)
#define M (tm->tm_min)
#define H (tm->tm_hour)

    if (S >= 60) {
	M += S / 60;
	S %= 60;
    }
    if (M >= 60) {
	H += M / 60;
	M %= 60;
    }
    if (H >= 24) {
	if (yday)
	    tm->tm_yday += H / 24;
	tm->tm_mday += H / 24;
	H %= 24;
    }
#undef S
#undef M
#undef H

    FPRINTF((stderr, "normalised time : %02d/%02d/%d:%02d:%02d:%02d\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year, tm->tm_hour, tm->tm_min, tm->tm_sec));

    if (date) {
	if (yday) {

	    if (tm->tm_yday < 0)
		int_error(NO_CARET, "Illegal day of year");

	    /* we just set month to jan, day to yday, and let the
	     * normalising code do the work.
	     */

	    tm->tm_mon = 0;
	    /* yday is 0->365, day is 1->31 */
	    tm->tm_mday = tm->tm_yday + 1;
	}
	if (tm->tm_mon < 0) {
	    int_error(NO_CARET, "illegal month");
	    return (NULL);
	}
	if (tm->tm_mday < 1) {
	    int_error(NO_CARET, "illegal day of month");
	    return (NULL);
	}
	if (tm->tm_mon > 11) {
	    tm->tm_year += tm->tm_mon / 12;
	    tm->tm_mon %= 12;
	} {
	    int days_in_month;
	    while (tm->tm_mday > (days_in_month = (mndday[tm->tm_mon] + (tm->tm_mon == 1 && (gdysize(tm->tm_year) > 365))))) {
		if (++tm->tm_mon == 12) {
		    ++tm->tm_year;
		    tm->tm_mon = 0;
		}
		tm->tm_mday -= days_in_month;
	    }
	}
    }
    return (s);
}

size_t
gstrftime(s, bsz, fmt, l_clock)
char *s;
size_t bsz;
const char *fmt;
double l_clock;
{
    struct tm tm;

    ggmtime(&tm, l_clock);
#if 0
    if ((tm.tm_zone = (char *) malloc(strlen(xtm->tm_zone) + 1)))
	strcpy(tm.tm_zone, xtm->tm_zone);
    /* printf("zone: %s - %s\n",tm.tm_zone,xtm->tm_zone); */
#endif

    return (xstrftime(s, bsz, fmt, &tm));
}


/* some shorthands : check that there is space in the output string
 * Odd-looking defn is dangling-else-safe
 */
#define CHECK_SPACE(n) if ( (l+(n)) <= bsz) ; else return 0

/* copy a fixed string, checking that there's room */
#define COPY_STRING(z) CHECK_SPACE(strlen(z)) ; strcpy(s, z)

/* format a string, using default spec if none given
 * w and z are width and zero-flag
 * dw and dz are the defaults for these
 * In fact, CHECK_SPACE(w) is not a sufficient test, since
 * sprintf("%2d", 365) outputs three characters
 */

#define FORMAT_STRING(dz, dw, x)           \
  if (w==0) { w=(dw); if (!z) z=(dz); }    \
  CHECK_SPACE(w);                          \
  sprintf(s, z ? "%0*d" : "%*d", w, (x) )

static int
xstrftime(str, bsz, fmt, tm)
char *str;			/* output buffer */
size_t bsz;			/* space available */
const char *fmt;
struct tm *tm;
{
    size_t l = 0;			/* chars written so far */

    char *s = str;

    memset(s, '\0', bsz);

    while (*fmt != '\0') {
	if (*fmt != '%') {
	    if (l >= bsz)
		return (0);
	    *s++ = *fmt++;
	    l++;
	} else {

	    /* set up format modifiers */
	    int w = 0;
	    int z = 0;
	    if (*++fmt == '0') {
		z = 1;
		++fmt;
	    }
	    while (*fmt >= '0' && *fmt <= '9') {
		w = w * 10 + (*fmt - '0');
		++fmt;
	    }

	    switch (*fmt++) {
	    case '%':
		CHECK_SPACE(1);
		*s = '%';
		break;

	    case 'a':
		COPY_STRING(abbrev_day_names[tm->tm_wday]);
		break;

	    case 'A':
		COPY_STRING(full_day_names[tm->tm_wday]);
		break;

	    case 'b':
	    case 'h':
		COPY_STRING(abbrev_month_names[tm->tm_mon]);
		break;

	    case 'B':
		COPY_STRING(full_month_names[tm->tm_mon]);
		break;


#if 0
		/* %x not currently supported, so neither is c */
	    case 'c':
		if (!xstrftime(s, bsz - l, "%x %X", tm))
		    return (0);
		break;
#endif

	    case 'd':
		FORMAT_STRING(1, 2, tm->tm_mday);	/* %02d */
		break;

	    case 'D':
		if (!xstrftime(s, bsz - l, "%m/%d/%y", tm))
		    return (0);
		break;

	    case 'H':
		FORMAT_STRING(1, 2, tm->tm_hour);	/* %02d */
		break;

	    case 'I':
		FORMAT_STRING(1, 2, tm->tm_hour % 12);	/* %02d */
		break;

	    case 'j':
		FORMAT_STRING(1, 3, tm->tm_yday + 1);	/* %03d */
		break;

		/* not in linux strftime man page. Not really needed now */
	    case 'k':
		FORMAT_STRING(0, 2, tm->tm_hour);	/* %2d */
		break;

	    case 'l':
		FORMAT_STRING(0, 2, tm->tm_hour % 12);	/* %2d */
		break;

	    case 'm':
		FORMAT_STRING(1, 2, tm->tm_mon + 1);	/* %02d */
		break;

	    case 'M':
		FORMAT_STRING(1, 2, tm->tm_min);	/* %02d */
		break;

	    case 'p':
		CHECK_SPACE(2);
		strcpy(s, (tm->tm_hour < 12) ? "am" : "pm");
		break;

	    case 'r':
		if (!xstrftime(s, bsz - l, "%I:%M:%S %p", tm))
		    return (0);
		break;

	    case 'R':
		if (!xstrftime(s, bsz - l, "%H:%M", tm))
		    return (0);
		break;

	    case 'S':
		FORMAT_STRING(1, 2, tm->tm_sec);	/* %02d */
		break;

	    case 'T':
		if (!xstrftime(s, bsz - l, "%H:%M:%S", tm))
		    return (0);
		break;

	    case 'W':		/* mon 1 day of week */
		{
		    int week;
		    if (tm->tm_yday <= tm->tm_wday) {
			week = 1;

			if ((tm->tm_mday - tm->tm_yday) > 4) {
			    week = 52;
			}
			if (tm->tm_yday == tm->tm_wday && tm->tm_wday == 0)
			    week = 52;

		    } else {

			/* sun prev week */
			int bw = tm->tm_yday - tm->tm_wday;

			if (tm->tm_wday > 0)
			    bw += 7;	/* sun end of week */

			week = (int) bw / 7;

			if ((bw % 7) > 2)	/* jan 1 is before friday */
			    week++;
		    }
		    FORMAT_STRING(1, 2, week);	/* %02d */
		    break;
		}

	    case 'U':		/* sun 1 day of week */
		{
		    int week, bw;

		    if (tm->tm_yday <= tm->tm_wday) {
			week = 1;
			if ((tm->tm_mday - tm->tm_yday) > 4) {
			    week = 52;
			}
		    } else {
			/* sat prev week */
			bw = tm->tm_yday - tm->tm_wday - 1;
			if (tm->tm_wday >= 0)
			    bw += 7;	/* sat end of week */
			week = (int) bw / 7;
			if ((bw % 7) > 1) {	/* jan 1 is before friday */
			    week++;
			}
		    }
		    FORMAT_STRING(1, 2, week);	/* %02d */
		    break;
		}

	    case 'w':		/* day of week, sun=0 */
		FORMAT_STRING(1, 2, tm->tm_wday);	/* %02d */
		break;

	    case 'y':
		FORMAT_STRING(1, 2, tm->tm_year % 100);		/* %02d */
		break;

	    case 'Y':
		FORMAT_STRING(1, 4, tm->tm_year);	/* %04d */
		break;

#if 0
	    case 'Z':
		COPY_STRING(tm->tm_zone);
		break;
#endif
	    }			/* switch */

	    while (*s != '\0') {
		s++;
		l++;
	    }
	}
    }
    return (l);
}



/* time_t  */
double
gtimegm(tm)
struct tm *tm;
{
    register int i;
    /* returns sec from year ZERO_YEAR, defined in plot.h */
    double dsec = 0.;

    if (tm->tm_year < ZERO_YEAR) {
	for (i = tm->tm_year; i < ZERO_YEAR; i++) {
	    dsec -= (double) gdysize(i);
	}
    } else {
	for (i = ZERO_YEAR; i < tm->tm_year; i++) {
	    dsec += (double) gdysize(i);
	}
    }
    if (tm->tm_mday > 0) {
	for (i = 0; i < tm->tm_mon; i++) {
	    dsec += (double) mndday[i] + (i == 1 && (gdysize(tm->tm_year) > 365));
	}
	dsec += (double) tm->tm_mday - 1;
    } else {
	dsec += (double) tm->tm_yday;
    }
    dsec *= (double) 24;

    dsec += tm->tm_hour;
    dsec *= 60.0;
    dsec += tm->tm_min;
    dsec *= 60.0;
    dsec += tm->tm_sec;

    FPRINTF((stderr, "broken-down time : %02d/%02d/%d:%02d:%02d:%02d = %g seconds\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year, tm->tm_hour,
	     tm->tm_min, tm->tm_sec, dsec));

    return (dsec);
}

int
ggmtime(tm, l_clock)
struct tm *tm;
/* time_t l_clock; */
double l_clock;
{
    /* l_clock is relative to ZERO_YEAR, jan 1, 00:00:00,defined in plot.h */
    int i, days;

    /* dodgy way of doing wday - i hope it works ! */

    int wday = JAN_FIRST_WDAY;	/* eg 6 for 2000 */

    FPRINTF((stderr, "%g seconds = ", l_clock));

    tm->tm_year = ZERO_YEAR;
    tm->tm_mday = tm->tm_yday = tm->tm_mon = tm->tm_hour = tm->tm_min = tm->tm_sec = 0;
    if (l_clock < 0) {
	while (l_clock < 0) {
	    int days_in_year = gdysize(--tm->tm_year);
	    l_clock += days_in_year * DAY_SEC;	/* 24*3600 */
	    /* adding 371 is noop in modulo 7 arithmetic, but keeps wday +ve */
	    wday += 371 - days_in_year;
	}
    } else {
	for (;;) {
	    int days_in_year = gdysize(tm->tm_year);
	    if (l_clock < days_in_year * DAY_SEC)
		break;
	    l_clock -= days_in_year * DAY_SEC;
	    tm->tm_year++;
	    /* only interested in result modulo 7, but %7 is expensive */
	    wday += (days_in_year - 364);
	}
    }
    tm->tm_yday = (int) (l_clock / DAY_SEC);
    l_clock -= tm->tm_yday * DAY_SEC;
    tm->tm_hour = (int) l_clock / 3600;
    l_clock -= tm->tm_hour * 3600;
    tm->tm_min = (int) l_clock / 60;
    l_clock -= tm->tm_min * 60;
    tm->tm_sec = (int) l_clock;

    days = tm->tm_yday;

    /* wday%7 should be day of week of first day of year */
    tm->tm_wday = (wday + days) % 7;

    while (days >= (i = mndday[tm->tm_mon] + (tm->tm_mon == 1 && (gdysize(tm->tm_year) > 365)))) {
	days -= i;
	tm->tm_mon++;
    }
    tm->tm_mday = days + 1;

    FPRINTF((stderr, "broken-down time : %02d/%02d/%d:%02d:%02d:%02d\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year, tm->tm_hour, tm->tm_min, tm->tm_sec));

    return (0);
}




#else /* USE_SYSTEM_TIME */

/* define gnu time routines in terms of system time routines */

size_t
gstrftime(buf, bufsz, fmt, l_clock)
char *buf;
size_t bufsz;
const char *fmt;
double l_clock;
{
    time_t t = (time_t) l_clock;
    return strftime(buf, bufsz, fmt, gmtime(&t));
}

double
gtimegm(tm)
struct tm *tm;
{
    return (double) mktime(tm);
}

int
ggmtime(tm, l_clock)
struct tm *tm;
double l_clock;
{
    time_t t = (time_t) l_clock;
    struct tm *m = gmtime(&t);
    *tm = *m;			/* can any non-ansi compilers not do this ? */
    return 0;
}

#define NOTHING
#define LETTER(L, width, field, extra) \
  case L: s=read_int(s,width,&tm->field); extra; continue

/* supplemental routine gstrptime() to read a formatted time */

char *
gstrptime(s, fmt, tm)
char *s;
char *fmt;
struct tm *tm;
{
    FPRINTF((stderr, "gstrptime(\"%s\", \"%s\")\n", s, fmt));

    /* linux does not appear to like years before 1902
     * NT complains if its before 1970
     * initialise fields to midnight, 1st Jan, 1970 (for relative times)
     */
    tm->tm_sec = tm->tm_min = tm->tm_hour = 0;
    tm->tm_mday = 1;
    tm->tm_mon = 0;
    tm->tm_year = 70;
    /* oops - it goes wrong without this */
    tm->tm_isdst = 0;

    for (; *fmt && *s; ++fmt) {
	if (*fmt != '%') {
	    if (*s != *fmt)
		return s;
	    ++s;
	    continue;
	}
	assert(*fmt == '%');

	switch (*++fmt) {
	case 0:
	    /* uh oh - % is last character in format */
	    return s;
	case '%':
	    /* literal % */
	    if (*s++ != '%')
		return s - 1;
	    continue;

	    LETTER('d', 2, tm_mday, NOTHING);
	    LETTER('m', 2, tm_mon, NOTHING);
	    LETTER('y', 2, tm_year, NOTHING);
	    LETTER('Y', 4, tm_year, tm->tm_year -= 1900);
	    LETTER('H', 2, tm_hour, NOTHING);
	    LETTER('M', 2, tm_min, NOTHING);
	    LETTER('S', 2, tm_sec, NOTHING);

	default:
	    int_error(NO_CARET, "incorrect time format character");
	}
    }

    FPRINTF((stderr, "Before mktime : %d/%d/%d:%d:%d:%d\n", tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_hour, tm->tm_min, tm->tm_sec));
    /* mktime range-checks the time */

    if (mktime(tm) == -1) {
	FPRINTF((stderr, "mktime() was not happy\n"));
	int_error(NO_CARET, "Invalid date/time [mktime() did not like it]");
    }
    FPRINTF((stderr, "After mktime : %d/%d/%d:%d:%d:%d\n", tm->tm_mday, tm->tm_mon, tm->tm_year, tm->tm_hour, tm->tm_min, tm->tm_sec));

    return s;
}


#endif /* USE_SYSTEM_TIME */


#ifdef TEST_TIME

/* either print current time using supplied format, or read
 * supplied time using supplied format
 */


int
main(argc, argv)
int argc;
char *argv[];
{
    char output[80];

    if (argc < 2) {
	fputs("usage : test 'format' ['time']\n", stderr);
	exit(EXIT_FAILURE);
    }
    if (argc == 2) {
	struct timeb now;
	struct tm *tm;
	ftime(&now);
	tm = gmtime(&now.time);
	xstrftime(output, 80, argv[1], tm);
	puts(output);
    } else {
	struct tm tm;
	gstrptime(argv[2], argv[1], &tm);
	puts(asctime(&tm));
    }
    exit(EXIT_SUCCESS);
}

#endif /* TEST_TIME */
