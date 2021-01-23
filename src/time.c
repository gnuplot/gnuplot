/* GNUPLOT - time.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
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


/* This module provides a set of routines to read or write times using
 * human-friendly units (hours, days, years, etc).  Unlike similar
 * ansi routines, these ones support sub-second precision and relative
 * times.  E.g.   -1.23 seconds is a valid time.
 */


#include "gp_time.h"

#include "util.h"
#include "variable.h"

static char *read_int(char *s, int nr, int *d);

static char *
read_int(char *s, int nr, int *d)
{
    int result = 0;

    while (--nr >= 0 && *s >= '0' && *s <= '9')
	result = result * 10 + (*s++ - '0');

    *d = result;
    return (s);
}


static int gdysize(int yr);

static int mndday[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static size_t xstrftime(char *buf, int bsz, const char *fmt, struct tm * tm, double usec, double fulltime);

/* days in year */
static int
gdysize(int yr)
{

    if (!(yr % 4)) {
	if ((!(yr % 100)) && yr % 400)
	    return (365);
	return (366);
    }
    return (365);
}


/* gstrptime() interprets a time_spec format string
 * and fills in a time structure that can be passed to gmtime() to
 * recover number of seconds from the EPOCH date.
 * Return value:
 * DT_TIMEDATE	indicates "normal" format elements corresponding to
 *		a date that is returned in tm, with fractional seconds
 *		returned in usec
 * DT_DMS	indicates relative time format elements were encountered
 *		(tD tH tM tS).  The relative time in seconds is returned
 *		in reltime.
 * DT_BAD	time format could not be interpreted
 *
 * parameters and return values revised for gnuplot version 5.3
 */

td_type
gstrptime(char *s, char *fmt, struct tm *tm, double *usec, double *reltime)
{
    int yday = 0;
    TBOOLEAN sanity_check_date = FALSE;
    TBOOLEAN reltime_formats = FALSE;
    TBOOLEAN explicit_pm = FALSE;
    TBOOLEAN explicit_am = FALSE;
    TBOOLEAN leading_minus_sign = FALSE;

    tm->tm_mday = 1;
    tm->tm_mon = tm->tm_hour = tm->tm_min = tm->tm_sec = 0;
    /* make relative times work (user-defined tic step) */
    tm->tm_year = ZERO_YEAR;

    init_timezone(tm);

    /* Fractional seconds will be returned separately, since
     * there is no slot for the fraction in struct tm.
     */
    *usec = 0.0;

    /* we do not yet calculate wday or yday, so make them illegal
     * [but yday will be read by %j]
     */
    tm->tm_yday = tm->tm_wday = -1;

    /* If the format requests explicit day, month, or year, then we will
     * do sanity checking to make sure the input makes sense.
     * For backward compatibility with gnuplot versions through 4.6.6
     * hour, minute, seconds default to zero with no error return
     * if the corresponding field cannot be found or interpreted.
     */ 
    if (strstr(fmt,"%d")) {
	tm->tm_mday = -1;
	sanity_check_date = TRUE;
    }
    if (strstr(fmt,"%y") || strstr(fmt,"%Y")) {
	tm->tm_year = -1;
	sanity_check_date = TRUE;
    }
    if (strstr(fmt,"%m") || strstr(fmt,"%B") || strstr(fmt,"%b")) {
	tm->tm_mon = -1;
	sanity_check_date = TRUE;
    }
    /* Relative time formats tD tH tM tS cannot be mixed with date formats */
    if (strstr(fmt,"%t")) {
	reltime_formats = TRUE;
	*reltime = 0.0;
	sanity_check_date = FALSE;
    }


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
		    if (strncasecmp(s, abbrev_month_names[m],
				    strlen(abbrev_month_names[m])) == 0) {
			s += strlen(abbrev_month_names[m]);
			goto found_abbrev_mon;
		    }
		/* get here => not found */
		int_warn(DATAFILE, "Bad abbreviated month name");
		m = 0;
	      found_abbrev_mon:
		tm->tm_mon = m;
		break;
	    }

	case 'B':		/* full month name */
	    {
		int m;

		for (m = 0; m < 12; ++m)
		    if (strncasecmp(s, full_month_names[m],
				    strlen(full_month_names[m])) == 0) {
			s += strlen(full_month_names[m]);
			goto found_full_mon;
		    }
		/* get here => not found */
		int_warn(DATAFILE, "Bad full month name");
		m = 0;
	      found_full_mon:
		tm->tm_mon = m;
		break;
	    }

	case 'd':		/* read a day of month */
	    s = read_int(s, 2, &tm->tm_mday);
	    break;

	case 'm':		/* month number */
	    s = read_int(s, 2, &tm->tm_mon);
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
	    tm->tm_year += 1900;
	    break;

	case 'Y':
	    s = read_int(s, 4, &tm->tm_year);
	    break;

	case 'j':
	    s = read_int(s, 3, &tm->tm_yday);
	    tm->tm_yday--;
	    sanity_check_date = TRUE;
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
	    if (*s == '.' || (decimalsign && *s == *decimalsign))
		*usec = atof(s);
	    break;


	case 's':
	    /* read EPOCH data
	     * EPOCH is the std. unix timeformat seconds since 01.01.1970 UTC
	     */
	    {
		char  *fraction = strchr(s, decimalsign ? *decimalsign : '.');
		double ufraction = 0;
		double when = strtod (s, &s) - SEC_OFFS_SYS;
		ggmtime(tm, when);
		if (fraction && fraction < s)
		    ufraction = atof(fraction);
		if (ufraction < 1.)	/* Filter out e.g. 123.456e7 */
		    *usec = ufraction;
		*reltime = when;	/* not used unless we return DT_DMS ... */
		if (when < 0)		/* ... which we force for negative times */
		    reltime_formats = TRUE;
		break;
	    }

	case 't':
	    /* Relative time formats tD tH tM tS */
	    {
		double cont = 0;

		/* Special case of negative time with first field 0,
		 * e.g.  -00:12:34
		 */
		if (*reltime == 0) {
		    while (isspace((unsigned char) *s)) s++;
		    if (*s == '-')
			leading_minus_sign = TRUE;
		}

		fmt++;
		if (*fmt == 'D') {
		    cont = 86400. * strtod(s, &s);
		} else if (*fmt == 'H') {
		    cont = 3600. * strtod(s, &s);
		} else if (*fmt == 'M') {
		    cont = 60. * strtod(s, &s);
		} else if (*fmt == 'S') {
		    cont = strtod(s, &s);
		} else {
		    return DT_BAD;
		}
		if (*reltime < 0)
		    *reltime -= fabs(cont);
		else if (*reltime > 0)
		    *reltime += fabs(cont);
		else if (leading_minus_sign)
		    *reltime -= fabs(cont);
		else
		    *reltime = cont;
		/* FIXME:  leading precision field should be accepted but ignored */
		break;
	    }

	case 'a':		/* weekday name (ignored) */
	case 'A':		/* weekday name (ignored) */
	    while (isalpha((unsigned char) *s))
		s++;
	    break;
	case 'w':		/* one or two digit weekday number (ignored) */
	case 'W':		/* one or two digit week number (ignored) */
	    if (isdigit((unsigned char) *s))
		s++;
	    if (isdigit((unsigned char) *s))
		s++;
	    break;

	case 'p':		/* am or pm */
	    if (!strncmp(s, "pm", 2) || !strncmp(s, "PM", 2))
		explicit_pm = TRUE;
	    if (!strncmp(s, "am", 2) || !strncmp(s, "AM", 2))
		explicit_am = TRUE;
	    s += 2;
	    break;

#ifdef HAVE_STRUCT_TM_TM_GMTOFF
	case 'z':		/* timezone offset  */
	    {
		int neg = (*s == '-') ? -1 : 1;
		int off_h, off_m;
		if (*s == '-' || *s == '+')
		    s++;
		s = read_int(s, 2, &off_h);
		if (*s == ':')
		    s++;
		s = read_int(s, 2, &off_m);
		tm->tm_gmtoff = 3600*off_h + 60*off_m;
		tm->tm_gmtoff *= neg;

	        break;
	    }
	case 'Z':		/* timezone name (ignored) */
	    while (*s && !isspace((unsigned char) *s))
		s++;
	    break;
#endif

	default:
	    int_warn(DATAFILE, "Bad time format in string");
	}
	fmt++;
    }

    /* Relative times are easy.  Just return the value in reltime */
    if (reltime_formats) {
	return DT_DMS;
    }

    FPRINTF((stderr, "read date-time : %02d/%02d/%d:%02d:%02d:%02d\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year, tm->tm_hour, tm->tm_min, tm->tm_sec));

    /* apply AM/PM correction */
    if ((tm->tm_hour < 12) && explicit_pm)
	tm->tm_hour += 12;
    if ((tm->tm_hour == 12) && explicit_am)
	tm->tm_hour = 0;

    /* now sanity check the date/time entered, normalising if necessary
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

    if (sanity_check_date) {
	if (yday) {

	    if (tm->tm_yday < 0) {
		return DT_BAD;
	    }

	    /* we just set month to jan, day to yday, and let the
	     * normalising code do the work.
	     */

	    tm->tm_mon = 0;
	    /* yday is 0->365, day is 1->31 */
	    tm->tm_mday = tm->tm_yday + 1;
	}
	if (tm->tm_mon < 0) {
	    return DT_BAD;
	}
	if (tm->tm_mday < 1) {
	    return DT_BAD;
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
    return DT_TIMEDATE;
}

size_t
gstrftime(char *s, size_t bsz, const char *fmt, double l_clock)
{
    struct tm tm;
    double usec;

    ggmtime(&tm, l_clock);

    usec = l_clock - (double)floor(l_clock);

    return xstrftime(s, bsz, fmt, &tm, usec, l_clock);
}


static size_t
xstrftime(
    char *str,			/* output buffer */
    int bsz,			/* remaining space available in buffer */
    const char *fmt,
    struct tm *tm,
    double usec,
    double fulltime)
{
    size_t l = 0;			/* chars written so far */
    int incr = 0;			/* chars just written */
    char *s = str;
    TBOOLEAN sign_printed = FALSE;

    if (bsz <= 0)
	return 0;

    memset(str, '\0', bsz);

    while (*fmt != '\0') {
	if (*fmt != '%') {
	    if (l >= bsz-1)
		return 0;
	    *s++ = *fmt++;
	    l++;
	} else {
	    /* set up format modifiers */
	    int w = 0;
	    int z = 0;
	    int p = 0;

	    if (*++fmt == '0') {
		z = 1;
		++fmt;
	    }
	    while (*fmt >= '0' && *fmt <= '9') {
		w = w * 10 + (*fmt - '0');
		++fmt;
	    }
	    if (*fmt == '.') {
		++fmt;
		while (*fmt >= '0' && *fmt <= '9') {
		    p = p * 10 + (*fmt - '0');
		    ++fmt;
		}
		if (p > 6) p = 6;
	    }

	    switch (*fmt++) {

		/* some shorthands : check that there is space in the
		 * output string. */
#define CHECK_SPACE(n) do {				\
		    if ((l+(n)) > bsz) return 0;	\
		} while (0)

		/* copy a fixed string, checking that there's room */
#define COPY_STRING(z) do {			\
		    CHECK_SPACE(strlen(z)) ;	\
		    strcpy(s, z);		\
		} while (0)

		/* format a string, using default spec if none given w
		 * and z are width and zero-flag dw and dz are the
		 * defaults for these In fact, CHECK_SPACE(w) is not a
		 * sufficient test, since sprintf("%2d", 365) outputs
		 * three characters
		 */
#define FORMAT_STRING(dz, dw, x) do {				\
		    if (w==0) {					\
			w=(dw);					\
			if (!z)					\
			    z=(dz);				\
		    }						\
		    incr = snprintf(s, bsz-l-1, z ? "%0*d" : "%*d", w, (x));	\
		    CHECK_SPACE(incr);				\
		} while(0)

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

	    case 'd':
		FORMAT_STRING(1, 2, tm->tm_mday);	/* %02d */
		break;

	    case 'D':
		if (!xstrftime(s, bsz - l, "%m/%d/%y", tm, 0., fulltime))
		    return 0;
		break;

	    case 'F':
		if (!xstrftime(s, bsz - l, "%Y-%m-%d", tm, 0., fulltime))
		    return 0;
		break;

	    case 'H':
		FORMAT_STRING(1, 2, tm->tm_hour);	/* %02d */
		break;

	    case 'I':
		FORMAT_STRING(1, 2, (tm->tm_hour + 11) % 12 + 1); /* %02d */
		break;

	    case 'j':
		FORMAT_STRING(1, 3, tm->tm_yday + 1);	/* %03d */
		break;

		/* not in linux strftime man page. Not really needed now */
	    case 'k':
		FORMAT_STRING(0, 2, tm->tm_hour);	/* %2d */
		break;

	    case 'l':
		FORMAT_STRING(0, 2, (tm->tm_hour + 11) % 12 + 1); /* %2d */
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
		if (!xstrftime(s, bsz - l, "%I:%M:%S %p", tm, 0., fulltime))
		    return 0;
		break;

	    case 'R':
		if (!xstrftime(s, bsz - l, "%H:%M", tm, 0., fulltime))
		    return 0;
		break;

	    case 's':
		CHECK_SPACE(12); /* large enough for year 9999 */
		sprintf(s, "%.0f", gtimegm(tm));
		break;

	    case 'S':
		FORMAT_STRING(1, 2, tm->tm_sec);	/* %02d */

		/* EAM FIXME - need to implement an actual format specifier */
		if (p > 0) {
		    double base = pow(10., (double)p);
		    int msec = floor(0.5 + base * usec);
		    char *f = &s[strlen(s)];
		    CHECK_SPACE(p+1);
		    sprintf(f, ".%0*d", p, msec<(int)base?msec:(int)base-1);
		}
		break;

	    case 'T':
		if (!xstrftime(s, bsz - l, "%H:%M:%S", tm, 0., fulltime))
		    return 0;
		break;

	    case 't':		/* Time (as opposed to Date) formats */
		{
		int tminute, tsecond;

		    switch (*fmt++) {
		    case 'D':
			/* +/- fractional days */
			if (p > 0) {
			    incr = snprintf(s, bsz-l-1, "%*.*f", w, p, fulltime/86400.);
			    CHECK_SPACE(incr);
			    break;
			}
			/* Set flag in case hours come next */
			if (fulltime < 0) {
			    CHECK_SPACE(1);	/* the minus sign */
			    *s++ = '-';
			    l++;
			}
			sign_printed = TRUE;
			/* +/- integral day truncated toward zero */
			sprintf(s, "%0*d", w, (int)floor(fabs(fulltime/86400.)));

			/* Subtract the day component from the total */
			fulltime -= sgn(fulltime) * 86400. * floor(fabs(fulltime/86400.));
			break;
		    case 'H':
			/* +/- fractional hours (not wrapped at 24h) */
			if (p > 0) {
			    incr = snprintf(s, bsz-l-1, "%*.*f", w, p,
				sign_printed ? fabs(fulltime)/3600. : fulltime/3600.);
			    CHECK_SPACE(incr);
			    break;
			}
			/* Set flag in case minutes come next */
			if (fulltime < 0 && !sign_printed) {
			    CHECK_SPACE(1);	/* the minus sign */
			    *s++ = '-';
			    l++;
			}
			sign_printed = TRUE;
			/* +/- integral hour truncated toward zero */
			sprintf(s, "%0*d", w, (int)floor(fabs(fulltime/3600.)));

			/* Subtract the hour component from the total */
			fulltime -= sgn(fulltime) * 3600. * floor(fabs(fulltime/3600.));
			break;
		    case 'M':
			/* +/- fractional minutes (not wrapped at 60m) */
			if (p > 0) {
			    incr = snprintf(s, bsz-l-1, "%*.*f", w, p,
				    sign_printed ? fabs(fulltime)/60. : fulltime/60.);
			    CHECK_SPACE(incr);
			    break;
			}
			/* +/- integral minute truncated toward zero */
			tminute = floor((fabs(fulltime/60.)));
			if (fulltime < 0 && !sign_printed) {
				*s++ = '-';
				l++;
			}
			sign_printed = TRUE;
			FORMAT_STRING(1, 2, tminute);	/* %02d */

			/* Subtract the minute component from the total */
			fulltime -= sgn(fulltime) * 60. * floor(fabs(fulltime/60.));
			break;
		    case 'S':
			/* +/- fractional seconds */
			tsecond = floor(fabs(fulltime));
			if (fulltime < 0) {
			    if (usec > 0)
				usec = 1.0 - usec;
			    if (!sign_printed) {
				*s++ = '-';
				l++;
			    }
			}
			FORMAT_STRING(1, 2, tsecond);	/* %02d */
			if (p > 0) {
			    double base = pow(10., (double)p);
			    int msec = floor(0.5 + base * usec);
			    char *f = &s[strlen(s)];
			    CHECK_SPACE(p+1);
			    sprintf(f, ".%0*d", p, msec<(int)base?msec:(int)base-1);
			}
			break;
		    default:
			break;
		    }
		    break;
		}

	    case 'W':		/* Mon..Sun is day 0..6 */
		/* CHANGE Jan 2021
		 * Follow ISO 8601 standard week date convention.
		 */
		{
		int week = tmweek(fulltime, 0);
		FORMAT_STRING(1, 2, week);	/* %02d */
		break;
		}

	    case 'U':		/* Sun..Sat is day 0..6 */
		/* CHANGE Jan 2021
		 * Follow CDC/MMWR "epi week" convention
		 */
		{
		int week = tmweek(fulltime, 1);
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

	    }			/* switch */

	    while (*s != '\0') {
		s++;
		l++;
	    }
#undef CHECK_SPACE
#undef COPY_STRING
#undef FORMAT_STRING
	} /* switch(fmt letter) */
    } /* if(fmt letter not '%') */
    return (l);
}



/* time_t  */
double
gtimegm(struct tm *tm)
{
    int i;
    /* returns sec from year ZERO_YEAR in UTC, defined in gp_time.h */
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

#ifdef HAVE_STRUCT_TM_TM_GMTOFF
    dsec -= tm->tm_gmtoff;

    FPRINTF((stderr, "broken-down time : %02d/%02d/%d:%02d:%02d:%02d:(%02d:%02d) = %g seconds\n",
	     tm->tm_mday, tm->tm_mon + 1, tm->tm_year, tm->tm_hour,
	     tm->tm_min, tm->tm_sec, tm->tm_gmtoff / 3600, (tm->tm_gmtoff % 3600) / 60, dsec));
#endif

    return (dsec);
}

int
ggmtime(struct tm *tm, double l_clock)
{
    /* l_clock is relative to ZERO_YEAR, jan 1, 00:00:00,defined in plot.h */
    int i, days;

    /* dodgy way of doing wday - i hope it works ! */
    int wday = JAN_FIRST_WDAY;	/* eg 6 for 2000 */

    FPRINTF((stderr, "%g seconds = ", l_clock));
    if (fabs(l_clock) > 1.e12) {  /* Some time in the year 33688 */
	int_warn(NO_CARET, "time value out of range");
	return(-1);
    }

    tm->tm_year = ZERO_YEAR;
    tm->tm_mday = tm->tm_yday = tm->tm_mon = tm->tm_hour = tm->tm_min = tm->tm_sec = 0;
    init_timezone(tm);

    if (l_clock >= 0) {
	for (;;) {
	    int days_in_year = gdysize(tm->tm_year);
	    if (l_clock < days_in_year * DAY_SEC)
		break;
	    l_clock -= days_in_year * DAY_SEC;
	    tm->tm_year++;
	    /* only interested in result modulo 7, but %7 is expensive */
	    wday += (days_in_year - 364);
	}
    } else {
	while (l_clock < 0) {
	    int days_in_year = gdysize(--tm->tm_year);
	    l_clock += days_in_year * DAY_SEC;	/* 24*3600 */
	    /* adding 371 is noop in modulo 7 arithmetic, but keeps wday +ve */
	    wday += 371 - days_in_year;
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
	/* This catches round-off error that initially assigned a date to year N-1 */
	/* but counting out seconds puts it in the first second of Jan year N.     */
	if (tm->tm_mon > 11) {
	    tm->tm_mon = 0;
	    tm->tm_year++;
	}
    }
    tm->tm_mday = days + 1;

    FPRINTF((stderr, "broken-down time : %02d/%02d/%d:%02d:%02d:%02d\n", tm->tm_mday, tm->tm_mon + 1, tm->tm_year, tm->tm_hour, tm->tm_min, tm->tm_sec));

    return (0);
}

/*
 * tmweek( time, standard )
 * input:  time in seconds since epoch date 1-Jan-1970
 * return: week of year in either
 *      standard = 0  to use ISO 8601 standard week (Mon-Sun)
 *      standard = 1  to use CDC/MMWR "epi week" (Sun-Sat)
 * Notes:
 *	The first week of the year is the week whose starts is closest
 *	    to 1 January, even if that is in the previous calendar year.
 *	Corollaries: Up to three days in Week 1 may be in previous year.
 *	The last week of a year may extend into the next calendar year.
 *	The highest week number in a year is either 52 or 53.
 */
int
tmweek( double time, int standard )
{
    struct tm tm;
    int wday, week;

    /* Fill time structure from time since epoch in seconds */
    ggmtime(&tm, time);

    if (standard == 0)
	/* ISO C tm->tm_wday uses 0 = Sunday but we want 0 = Monday */
	wday = (6 + tm.tm_wday) % 7;
    else
	wday = tm.tm_wday;
    week = (int)(10 + tm.tm_yday - wday ) / 7;

    /* Up to three days in December may belong in week 1 of the next year. */
    if (tm.tm_mon == 11) {
	if ( (tm.tm_mday == 31 && wday < 3)
	||   (tm.tm_mday == 30 && wday < 2)
	||   (tm.tm_mday == 29 && wday < 1))
	    week = 1;
    }

    /* Up to three days in January may be in the last week of the previous year.
     * That might be either week 52 or week 53 depending on the leap year cycle.
     */
    if (week == 0) {
	struct tm temp = tm;
	int Jan01, Dec31;

	/* Was either 1 Jan or 31 Dec of the previous year precisely midweek? */
	temp.tm_year -= 1; temp.tm_mon = 0; temp.tm_mday = 1;
	ggmtime( &temp, gtimegm(&temp) );
	Jan01 = temp.tm_wday;
	temp.tm_mon = 11; temp.tm_mday = 31;
	ggmtime( &temp, gtimegm(&temp) );
	Dec31 = temp.tm_wday;
	if (standard == 0)
	    week = (Jan01 == 4 || Dec31 == 4) ? 53 : 52;
	else
	    week = (Jan01 == 3 || Dec31 == 3) ? 53 : 52;
    }

    return week;
}


/*
 * Convert from week date notation to standard time in seconds since epoch.
 *   time = weekdate( year, week, standard )
 *   year, week as given in "week date" format; may not be calendar year
 *          e.g. ISO week date 2009-W01-2 corresponds to 30 Dec 2008
 *   standard 0 - ISO 8601 week date (week starts on Monday)
 *   standard 1 - CDC/MMRW epidemiological week (starts on Sunday)
 */
double
weekdate( int year, int week, int day, int standard )
{
    struct tm time_tm;
    double time;
    int wday;

    /* Sanity check input (but allow day = 0 to mean day 1) */
    /* FIXME:  Is there a minimum year for ISO dates? */
    if (week < 1 || week > 53 || day > 7 || day < 0)
	int_error(NO_CARET, "invalid week date");
    if (day == 0)
	day = 1;

    memset( &time_tm, 0, sizeof(struct tm) );

    /* Find standard time and week date for 1 Jan in nominal year.
     * This will let us determine the start date for the week date system.
     */
    time_tm.tm_year = year;
    time_tm.tm_mday = 1;
    time_tm.tm_mon = 0;
    time = gtimegm(&time_tm);

    /* Normalize (unlike mktime, gtimegm does not recalculation wday) */
    ggmtime(&time_tm, time);

    /* Add offset to nearest Sunday (ISO 8601) */
    if (standard == 1)
	wday = time_tm.tm_wday;
    else
	wday = (6 + time_tm.tm_wday) % 7;
    if (wday < 4)
	time -= wday * DAY_SEC;
    else
	time += (7-wday) * DAY_SEC;

    /* Now add offsets to the week number we were given */
    time += (week-1) * WEEK_SEC;
    time += (day-1) * DAY_SEC;

    return time;
}
