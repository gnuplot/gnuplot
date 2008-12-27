#ifndef lint
static char *RCSid() { return RCSid("$Id: util.c,v 1.83 2008/12/27 20:09:12 sfeam Exp $"); }
#endif

/* GNUPLOT - util.c */

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

#include "util.h"

#include "alloc.h"
#include "command.h"
#include "datafile.h"		/* for df_showdata and df_reset_after_error */
#include "misc.h"
#include "plot.h"
#include "term_api.h"		/* for term_end_plot() used by graph_error() */
#include "variable.h" /* For locale handling */

#if defined(HAVE_DIRENT_H)
# include <sys/types.h>
# include <dirent.h>
#elif defined(_Windows)
# include <windows.h>
#endif

#if defined(HAVE_PWD_H)
# include <sys/types.h>
# include <pwd.h>
#elif defined(_Windows)
# include <windows.h>
# if !defined(INFO_BUFFER_SIZE)
#  define INFO_BUFFER_SIZE 32767
# endif
#endif

/* Exported (set-table) variables */

/* decimal sign */
char *decimalsign = NULL;

/* Holds the name of the current LC_NUMERIC as set by "set decimal locale" */
char *numeric_locale = NULL;

/* Holds the name of the current LC_TIME as set by "set locale" */
char *current_locale = NULL;

const char *current_prompt = NULL; /* to be set by read_line() */

/* internal prototypes */

static void mant_exp __PROTO((double, double, TBOOLEAN, double *, int *, const char *));
static void parse_sq __PROTO((char *));

#if 0 /* UNUSED */
/*
 * chr_in_str() compares the characters in the string of token number t_num
 * with c, and returns TRUE if a match was found.
 */
int
chr_in_str(int t_num, int c)
{
    int i;

    if (!token[t_num].is_token)
	return (FALSE);		/* must be a value--can't be equal */
    for (i = 0; i < token[t_num].length; i++) {
	if (input_line[token[t_num].start_index + i] == c)
	    return (TRUE);
    }
    return FALSE;
}
#endif

/*
 * equals() compares string value of token number t_num with str[], and
 *   returns TRUE if they are identical.
 */
int
equals(int t_num, const char *str)
{
    int i;

    if (!token[t_num].is_token)
	return (FALSE);		/* must be a value--can't be equal */
    for (i = 0; i < token[t_num].length; i++) {
	if (gp_input_line[token[t_num].start_index + i] != str[i])
	    return (FALSE);
    }
    /* now return TRUE if at end of str[], FALSE if not */
    return (str[i] == NUL);
}



/*
 * almost_equals() compares string value of token number t_num with str[], and
 *   returns TRUE if they are identical up to the first $ in str[].
 */
int
almost_equals(int t_num, const char *str)
{
    int i;
    int after = 0;
    int start = token[t_num].start_index;
    int length = token[t_num].length;

    if (!str)
	return FALSE;
    if (!token[t_num].is_token)
	return FALSE;		/* must be a value--can't be equal */
    for (i = 0; i < length + after; i++) {
	if (str[i] != gp_input_line[start + i]) {
	    if (str[i] != '$')
		return (FALSE);
	    else {
		after = 1;
		start--;	/* back up token ptr */
	    }
	}
    }

    /* i now beyond end of token string */

    return (after || str[i] == '$' || str[i] == NUL);
}



int
isstring(int t_num)
{

    return (token[t_num].is_token &&
	    (gp_input_line[token[t_num].start_index] == '\'' ||
	     gp_input_line[token[t_num].start_index] == '"'));
}

int
type_udv(int t_num)
{
    struct udvt_entry **udv_ptr = &first_udv;

    while (*udv_ptr) {
       if (equals(t_num, (*udv_ptr)->udv_name))
	   return (*udv_ptr)->udv_value.type;
       udv_ptr = &((*udv_ptr)->next_udv);
    }
    return 0;
}

int
isanumber(int t_num)
{
    return (!token[t_num].is_token);
}


int
isletter(int t_num)
{
    return (token[t_num].is_token &&
	    ((isalpha((unsigned char) gp_input_line[token[t_num].start_index])) ||
	     (gp_input_line[token[t_num].start_index] == '_')));
}


/*
 * is_definition() returns TRUE if the next tokens are of the form
 *   identifier =
 *              -or-
 *   identifier ( identifer {,identifier} ) =
 */
int
is_definition(int t_num)
{
    /* variable? */
    if (isletter(t_num) && equals(t_num + 1, "="))
	return 1;

    /* function? */
    /* look for dummy variables */
    if (isletter(t_num) && equals(t_num + 1, "(") && isletter(t_num + 2)) {
	t_num += 3;		/* point past first dummy */
	while (equals(t_num, ",")) {
	    if (!isletter(++t_num))
		return 0;
	    t_num += 1;
	}
	return (equals(t_num, ")") && equals(t_num + 1, "="));
    }
    /* neither */
    return 0;
}



/*
 * copy_str() copies the string in token number t_num into str, appending
 *   a null.  No more than max chars are copied (including \0).
 */
void
copy_str(char *str, int t_num, int max)
{
    int i = 0;
    int start = token[t_num].start_index;
    int count = token[t_num].length;

    if (count >= max) {
	count = max - 1;
	FPRINTF((stderr, "str buffer overflow in copy_str"));
    }

    do {
	str[i++] = gp_input_line[start++];
    } while (i != count);
    str[i] = NUL;

}

/* length of token string */
size_t
token_len(int t_num)
{
    return (size_t)(token[t_num].length);
}

/*
 * quote_str() does the same thing as copy_str, except it ignores the
 *   quotes at both ends.  This seems redundant, but is done for
 *   efficency.
 */
void
quote_str(char *str, int t_num, int max)
{
    int i = 0;
    int start = token[t_num].start_index + 1;
    int count;

    if ((count = token[t_num].length - 2) >= max) {
	count = max - 1;
	FPRINTF((stderr, "str buffer overflow in quote_str"));
    }
    if (count > 0) {
	do {
	    str[i++] = gp_input_line[start++];
	} while (i != count);
    }
    str[i] = NUL;
    /* convert \t and \nnn (octal) to char if in double quotes */
    if (gp_input_line[token[t_num].start_index] == '"')
	parse_esc(str);
    else
        parse_sq(str);
}


/*
 * capture() copies into str[] the part of gp_input_line[] which lies between
 * the begining of token[start] and end of token[end].
 */
void
capture(char *str, int start, int end, int max)
{
    int i, e;

    e = token[end].start_index + token[end].length;
    if (e - token[start].start_index >= max) {
	e = token[start].start_index + max - 1;
	FPRINTF((stderr, "str buffer overflow in capture"));
    }
    for (i = token[start].start_index; i < e && gp_input_line[i] != NUL; i++)
	*str++ = gp_input_line[i];
    *str = NUL;
}


/*
 * m_capture() is similar to capture(), but it mallocs storage for the
 * string.
 */
void
m_capture(char **str, int start, int end)
{
    int i, e;
    char *s;

    e = token[end].start_index + token[end].length;
    *str = gp_realloc(*str, (e - token[start].start_index + 1), "string");
    s = *str;
    for (i = token[start].start_index; i < e && gp_input_line[i] != NUL; i++)
	*s++ = gp_input_line[i];
    *s = NUL;
}


/*
 * m_quote_capture() is similar to m_capture(), but it removes
 * quotes from either end of the string.
 */
void
m_quote_capture(char **str, int start, int end)
{
    int i, e;
    char *s;

    e = token[end].start_index + token[end].length - 1;
    *str = gp_realloc(*str, (e - token[start].start_index + 1), "string");
    s = *str;
    for (i = token[start].start_index + 1; i < e && gp_input_line[i] != NUL; i++)
	*s++ = gp_input_line[i];
    *s = NUL;

    if (gp_input_line[token[start].start_index] == '"')
	parse_esc(*str);
    else
        parse_sq(*str);

}

/*
 * Wrapper for isstring + m_quote_capture that can be used with
 * or without GP_STRING_VARS enabled.
 * EAM Aug 2004
 */
char *
try_to_get_string()
{
    char *newstring = NULL;
    struct value a;
    int save_token = c_token;

    if (END_OF_COMMAND)
	return NULL;
    const_string_express(&a);
    if (a.type == STRING)
	newstring = a.v.string_val;
    else
	c_token = save_token;

    return newstring;
}


/* Our own version of strdup()
 * Make copy of string into gp_alloc'd memory
 * As with all conforming str*() functions,
 * it is the caller's responsibility to pass
 * valid parameters!
 */
char *
gp_strdup(const char *s)
{
    char *d;

    if (!s)
	return NULL;

#ifndef HAVE_STRDUP
    d = gp_alloc(strlen(s) + 1, "gp_strdup");
    if (d)
	memcpy (d, s, strlen(s) + 1);
#else
    d = strdup(s);
#endif
    return d;
}

/*
 * Allocate a new string and initialize it by concatenating two
 * existing strings.
 */
char *
gp_stradd(const char *a, const char *b)
{
    char *new = gp_alloc(strlen(a)+strlen(b)+1,"gp_stradd");
    strcpy(new,a);
    strcat(new,b);
    return new;
}

/* HBB 20020405: moved these functions here from axis.c, where they no
 * longer truly belong. */
/*{{{  mant_exp - split into mantissa and/or exponent */
/* HBB 20010121: added code that attempts to fix rounding-induced
 * off-by-one errors in 10^%T and similar output formats */
static void
mant_exp(
    double log10_base,
    double x,
    TBOOLEAN scientific,	/* round to power of 3 */
    double *m,			/* results */
    int *p,
    const char *format)		/* format string for fixup */
{
    int sign = 1;
    double l10;
    int power;
    double mantissa;

    /*{{{  check 0 */
    if (x == 0) {
	if (m)
	    *m = 0;
	if (p)
	    *p = 0;
	return;
    }
    /*}}} */
    /*{{{  check -ve */
    if (x < 0) {
	sign = (-1);
	x = (-x);
    }
    /*}}} */

    l10 = log10(x) / log10_base;
    power = floor(l10);
    mantissa = pow(10.0, log10_base * (l10 - power));

    /* round power to an integer multiple of 3, to get what's
     * sometimes called 'scientific' or 'engineering' notation. Also
     * useful for handling metric unit prefixes like 'kilo' or 'micro'
     * */
    if (scientific) {
	/* Scientific mode makes no sense whatsoever if the base of
	 * the logarithmic axis is anything but 10.0 */
	assert(log10_base == 1.0);

	/* HBB FIXED 20040701: negative modulo positive may yield
	 * negative result.  But we always want an effectively
	 * positive modulus --> adjust input by one step */
	switch (power % 3) {
	case -1:
	    power -= 3;
	case 2:
	    mantissa *= 100;
	    break;
	case -2:
	    power -= 3;
	case 1:
	    mantissa *= 10;
	    break;
	case 0:
	    break;
	default:
	    int_error (NO_CARET, "Internal error in scientific number formatting");
	}
	power -= (power % 3);
    }

    /* HBB 20010121: new code for decimal mantissa fixups.  Looks at
     * format string to see how many decimals will be put there.  Iff
     * the number is so close to an exact power of 10 that it will be
     * rounded up to 10.0e??? by an sprintf() with that many digits of
     * precision, increase the power by 1 to get a mantissa in the
     * region of 1.0.  If this handling is not wanted, pass NULL as
     * the format string */
    /* HBB 20040521: extended to also work for bases other than 10.0 */
    if (format) {
	double actual_base = (scientific ? 1000 : pow(10.0, log10_base));
	int precision = 0;
	double tolerance;

	format = strchr (format, '.');
	if (format != NULL)
	    /* a decimal point was found in the format, so use that
	     * precision. */
	    precision = strtol(format + 1, NULL, 10);

	/* See if mantissa would be right on the border.  The
	 * condition to watch out for is that the mantissa is within
	 * one printing precision of the next power of the logarithm
	 * base.  So add the 0.5*10^-precision to the mantissa, and
	 * see if it's now larger than the base of the scale */
	tolerance = pow(10.0, -precision) / 2;
	if (mantissa + tolerance >= actual_base) {
	    mantissa /= actual_base;
	    power += (scientific ? 3 : 1);
	}
    }
    if (m)
	*m = sign * mantissa;
    if (p)
	*p = power;
}

/*}}} */

/*
 * Kludge alert!!
 * Workaround until we have a better solution ...
 * Note: this assumes that all calls to sprintf in gprintf have
 * exactly three args. Lars
 */
#ifdef HAVE_SNPRINTF
# define sprintf(str,fmt,arg) \
    if (snprintf((str),count,(fmt),(arg)) > count) \
      fprintf (stderr,"%s:%d: Warning: too many digits for format\n",__FILE__,__LINE__)
#endif

/*{{{  gprintf */
/* extended s(n)printf */
/* HBB 20010121: added code to maintain consistency between mantissa
 * and exponent across sprintf() calls.  The problem: format string
 * '%t*10^%T' will display 9.99 as '10.0*10^0', but 10.01 as
 * '1.0*10^1'.  This causes problems for people using the %T part,
 * only, with logscaled axes, in combination with the occasional
 * round-off error. */
void
gprintf(
    char *dest,
    size_t count,
    char *format,
    double log10_base,
    double x)
{
    char temp[MAX_LINE_LEN + 1];
    char *t;
    TBOOLEAN seen_mantissa = FALSE; /* memorize if mantissa has been
                                       output, already */
    int stored_power = 0;	/* power that matches the mantissa
                                   output earlier */
    TBOOLEAN got_hash = FALSE;				   

    set_numeric_locale();

    for (;;) {
	/*{{{  copy to dest until % */
	while (*format != '%')
	    if (!(*dest++ = *format++)) {
		reset_numeric_locale();
		return;		/* end of format */
	    }
	/*}}} */

	/*{{{  check for %% */
	if (format[1] == '%') {
	    *dest++ = '%';
	    format += 2;
	    continue;
	}
	/*}}} */

	/*{{{  copy format part to temp, excluding conversion character */
	t = temp;
	*t++ = '%';
	if (format[1] == '#') {
	    *t++ = '#';
	    format++;
	    got_hash = TRUE;
	}
	/* dont put isdigit first since sideeffect in macro is bad */
	while (*++format == '.' || isdigit((unsigned char) *format)
	       || *format == '-' || *format == '+' || *format == ' '
	       || *format == '\'')
	    *t++ = *format;
	/*}}} */

	/*{{{  convert conversion character */
	switch (*format) {
	    /*{{{  x and o */
	case 'x':
	case 'X':
	case 'o':
	case 'O':
	    t[0] = *format;
	    t[1] = 0;
	    sprintf(dest, temp, (int) x);
	    break;
	    /*}}} */
	    /*{{{  e, f and g */
	case 'e':
	case 'E':
	case 'f':
	case 'F':
	case 'g':
	case 'G':
	    t[0] = *format;
	    t[1] = 0;
	    sprintf(dest, temp, x);
	    break;
	    /*}}} */
	    /*{{{  l --- mantissa to current log base */
	case 'l':
	    {
		double mantissa;

		t[0] = 'f';
		t[1] = 0;
		mant_exp(log10_base, x, FALSE, &mantissa, &stored_power, temp);
		seen_mantissa = TRUE;
		sprintf(dest, temp, mantissa);
		break;
	    }
	    /*}}} */
	    /*{{{  t --- base-10 mantissa */
	case 't':
	    {
		double mantissa;

		t[0] = 'f';
		t[1] = 0;
		mant_exp(1.0, x, FALSE, &mantissa, &stored_power, temp);
		seen_mantissa = TRUE;
		sprintf(dest, temp, mantissa);
		break;
	    }
	    /*}}} */
	    /*{{{  s --- base-1000 / 'scientific' mantissa */
	case 's':
	    {
		double mantissa;

		t[0] = 'f';
		t[1] = 0;
		mant_exp(1.0, x, TRUE, &mantissa, &stored_power, temp);
		seen_mantissa = TRUE;
		sprintf(dest, temp, mantissa);
		break;
	    }
	    /*}}} */
	    /*{{{  L --- power to current log base */
	case 'L':
	    {
		int power;

		t[0] = 'd';
		t[1] = 0;
		if (seen_mantissa)
		    power = stored_power;
		else
		    mant_exp(log10_base, x, FALSE, NULL, &power, "%.0f");
		sprintf(dest, temp, power);
		break;
	    }
	    /*}}} */
	    /*{{{  T --- power of ten */
	case 'T':
	    {
		int power;

		t[0] = 'd';
		t[1] = 0;
		if (seen_mantissa)
		    power = stored_power;
		else
		    mant_exp(1.0, x, FALSE, NULL, &power, "%.0f");
		sprintf(dest, temp, power);
		break;
	    }
	    /*}}} */
	    /*{{{  S --- power of 1000 / 'scientific' */
	case 'S':
	    {
		int power;

		t[0] = 'd';
		t[1] = 0;
		if (seen_mantissa)
		    power = stored_power;
		else
		    mant_exp(1.0, x, TRUE, NULL, &power, "%.0f");
		sprintf(dest, temp, power);
		break;
	    }
	    /*}}} */
	    /*{{{  c --- ISO decimal unit prefix letters */
	case 'c':
	    {
		int power;

		t[0] = 'c';
		t[1] = 0;
		if (seen_mantissa)
		    power = stored_power;
		else
		    mant_exp(1.0, x, TRUE, NULL, &power, "%.0f");

		if (power >= -18 && power <= 18) {
		    /* -18 -> 0, 0 -> 6, +18 -> 12, ... */
		    /* HBB 20010121: avoid division of -ve ints! */
		    power = (power + 18) / 3;
		    sprintf(dest, temp, "afpnum kMGTPE"[power]);
		} else {
		    /* please extend the range ! */
		    /* name  power   name  power
		       -------------------------
		       atto   -18    Exa    18
		       femto  -15    Peta   15
		       pico   -12    Tera   12
		       nano    -9    Giga    9
		       micro   -6    Mega    6
		       milli   -3    kilo    3   */

		    /* for the moment, print e+21 for example */
		    sprintf(dest, "e%+02d", (power - 6) * 3);
		}

		break;
	    }
	    /*}}} */
	    /*{{{  P --- multiple of pi */
	case 'P':
	    {
		t[0] = 'f';
		t[1] = 0;
		sprintf(dest, temp, x / M_PI);
		break;
	    }
	    /*}}} */
	default:
	   reset_numeric_locale();
	   int_error(NO_CARET, "Bad format character");
	} /* switch */
	/*}}} */
	
	if (got_hash && (format != strpbrk(format,"oeEfFgG"))) {
	   reset_numeric_locale();
	   int_error(NO_CARET, "Bad format character");
	}

    /* change decimal `.' to the actual entry in decimalsign */
	if (decimalsign != NULL) {
	    char *dotpos1 = dest, *dotpos2;
	    size_t newlength = strlen(decimalsign);
	    int dot;

	    /* dot is the default decimalsign we will be replacing */
	    dot = *get_decimal_locale();

	    /* replace every dot by the contents of decimalsign */
	    while ((dotpos2 = strchr(dotpos1,dot)) != NULL) {
		size_t taillength = strlen(dotpos2);

		dotpos1 = dotpos2 + newlength;
		/* test if the new value for dest would be too long */
		if (dotpos1 - dest + taillength > count)
		    int_error(NO_CARET,
			      "format too long due to long decimalsign string");
		/* move tail end of string out of the way */
		memmove(dotpos1, dotpos2 + 1, taillength);
		/* insert decimalsign */
		memcpy(dotpos2, decimalsign, newlength);
	    }
	    /* clear temporary variables for safety */
	    dotpos1=NULL;
	    dotpos2=NULL;
	}

	/* this was at the end of every single case, before: */
	dest += strlen(dest);
	++format;
    } /* for ever */

    reset_numeric_locale();
}

/*}}} */
#ifdef HAVE_SNPRINTF
# undef sprintf
#endif

/* some macros for the error and warning functions below
 * may turn this into a utility function later
 */
#define PRINT_MESSAGE_TO_STDERR				\
do {							\
    fprintf(stderr, "\n%s%s\n",				\
	    current_prompt ? current_prompt : "",	\
	    gp_input_line);				\
} while (0)
    
#define PRINT_SPACES_UNDER_PROMPT		\
do {						\
    const char *p;				\
						\
    if (!current_prompt)			\
	break;					\
    for (p = current_prompt; *p != '\0'; p++)	\
	(void) fputc(' ', stderr);		\
} while (0)

#define PRINT_SPACES_UPTO_TOKEN						\
do {									\
    int i;								\
									\
    for (i = 0; i < token[t_num].start_index; i++)			\
	(void) fputc((gp_input_line[i] == '\t') ? '\t' : ' ', stderr);	\
} while(0)

#define PRINT_CARET fputs("^\n",stderr);

#define PRINT_FILE_AND_LINE						\
if (!interactive) {							\
    if (lf_head && lf_head->name)                                       \
	fprintf(stderr, "\"%s\", line %d: ", lf_head->name, inline_num);\
    else fprintf(stderr, "line %d: ", inline_num);			\
}

/* TRUE if command just typed; becomes FALSE whenever we
 * send some other output to screen.  If FALSE, the command line
 * will be echoed to the screen before the ^ error message.
 */
TBOOLEAN screen_ok;

#if defined(VA_START) && defined(STDC_HEADERS)
void
os_error(int t_num, const char *str,...)
#else
void
os_error(int t_num, const char *str, va_dcl)
#endif
{
#ifdef VA_START
    va_list args;
#endif
#ifdef VMS
    static status[2] = { 1, 0 };		/* 1 is count of error msgs */
#endif /* VMS */

    /* reprint line if screen has been written to */

    if (t_num == DATAFILE) {
	df_showdata();
    } else if (t_num != NO_CARET) {	/* put caret under error */
	if (!screen_ok)
	    PRINT_MESSAGE_TO_STDERR;

	PRINT_SPACES_UNDER_PROMPT;
	PRINT_SPACES_UPTO_TOKEN;
	PRINT_CARET;
    }
    PRINT_SPACES_UNDER_PROMPT;

#ifdef VA_START
    VA_START(args, str);
# if defined(HAVE_VFPRINTF) || _LIBC
    vfprintf(stderr, str, args);
# else
    _doprnt(str, args, stderr);
# endif
    va_end(args);
#else
    fprintf(stderr, str, a1, a2, a3, a4, a5, a6, a7, a8);
#endif
    putc('\n', stderr);

    PRINT_SPACES_UNDER_PROMPT;
    PRINT_FILE_AND_LINE;

#ifdef VMS
    status[1] = vaxc$errno;
    sys$putmsg(status);
    (void) putc('\n', stderr);
#else /* VMS */
    perror("util.c");
    putc('\n', stderr);
#endif /* VMS */

    bail_to_command_line();
}


#if defined(VA_START) && defined(STDC_HEADERS)
void
int_error(int t_num, const char *str,...)
#else
void
int_error(int t_num, const char str[], va_dcl)
#endif
{
#ifdef VA_START
    va_list args;
#endif

    char error_message[128] = {'\0'};

    /* reprint line if screen has been written to */

    if (t_num == DATAFILE) {
        df_showdata();
    } else if (t_num != NO_CARET) { /* put caret under error */
	if (!screen_ok)
	    PRINT_MESSAGE_TO_STDERR;

	PRINT_SPACES_UNDER_PROMPT;
	PRINT_SPACES_UPTO_TOKEN;
	PRINT_CARET;
    }
    PRINT_SPACES_UNDER_PROMPT;
    PRINT_FILE_AND_LINE;

#ifdef VA_START
    VA_START(args, str);
# if defined(HAVE_VFPRINTF) || _LIBC
    vsnprintf(error_message, sizeof(error_message), str, args);
    fprintf(stderr,"%.120s",error_message);
# else
    _doprnt(str, args, stderr);
# endif
    va_end(args);
#else
    fprintf(stderr, str, a1, a2, a3, a4, a5, a6, a7, a8);
#ifdef HAVE_SNPRINTF
    snprintf(error_message, sizeof(error_message), str, a1, a2, a3, a4, a5, a6, a7, a8);
#else
    sprintf(error_message, str, a1, a2, a3, a4, a5, a6, a7, a8);
#endif
#endif

    fputs("\n\n", stderr);

    /* We are bailing out of nested context without ever reaching */
    /* the normal cleanup code. Reset any flags before bailing.   */
    df_reset_after_error();

    /* Load error state variables */
    update_gpval_variables(2);
    fill_gpval_string("GPVAL_ERRMSG", error_message);

    bail_to_command_line();
}

/* Warn without bailing out to command line. Not a user error */
#if defined(VA_START) && defined(STDC_HEADERS)
void
int_warn(int t_num, const char *str,...)
#else
void
int_warn(int t_num, const char str[], va_dcl)
#endif
{
#ifdef VA_START
    va_list args;
#endif

    /* reprint line if screen has been written to */

    if (t_num == DATAFILE) {
        df_showdata();
    } else if (t_num != NO_CARET) { /* put caret under error */
	if (!screen_ok)
	    PRINT_MESSAGE_TO_STDERR;

	PRINT_SPACES_UNDER_PROMPT;
	PRINT_SPACES_UPTO_TOKEN;
	PRINT_CARET;
    }
    PRINT_SPACES_UNDER_PROMPT;
    PRINT_FILE_AND_LINE;

    fputs("warning: ", stderr);
#ifdef VA_START
    VA_START(args, str);
# if defined(HAVE_VFPRINTF) || _LIBC
    vfprintf(stderr, str, args);
# else
    _doprnt(str, args, stderr);
# endif
    va_end(args);
#else  /* VA_START */
    fprintf(stderr, str, a1, a2, a3, a4, a5, a6, a7, a8);
#endif /* VA_START */
    putc('\n', stderr);
}

/*{{{  graph_error() */
/* handle errors during graph-plot in a consistent way */
/* HBB 20000430: move here, from graphics.c */
#if defined(VA_START) && defined(STDC_HEADERS)
void
graph_error(const char *fmt, ...)
#else
void
graph_error(const char *fmt, va_dcl)
#endif
{
#ifdef VA_START
    va_list args;
#endif

    multiplot = FALSE;
    term_end_plot();

#ifdef VA_START
    VA_START(args, fmt);
#if 0
    /* HBB 20001120: this seems not to work at all. Probably because a
     * va_list argument, is, after all, something else than a varargs
     * list (i.e. a '...') */
    int_error(NO_CARET, fmt, args);
#else
    /* HBB 20001120: instead, copy the core code from int_error() to
     * here: */
    PRINT_SPACES_UNDER_PROMPT;
    PRINT_FILE_AND_LINE;

# if defined(HAVE_VFPRINTF) || _LIBC
    vfprintf(stderr, fmt, args);
# else
    _doprnt(fmt, args, stderr);
# endif
    va_end(args);
    fputs("\n\n", stderr);

    bail_to_command_line();
#endif /* 1/0 */
    va_end(args);
#else
    int_error(NO_CARET, fmt, a1, a2, a3, a4, a5, a6, a7, a8);
#endif

}

/*}}} */


/* Lower-case the given string (DFK) */
/* Done in place. */
void
lower_case(char *s)
{
    char *p = s;

    while (*p) {
	if (isupper((unsigned char)*p))
	    *p = tolower((unsigned char)*p);
	p++;
    }
}

/* Squash spaces in the given string (DFK) */
/* That is, reduce all multiple white-space chars to single spaces */
/* Done in place. */
void
squash_spaces(char *s)
{
    char *r = s;	/* reading point */
    char *w = s;	/* writing point */
    TBOOLEAN space = FALSE;	/* TRUE if we've already copied a space */

    for (w = r = s; *r != NUL; r++) {
	if (isspace((unsigned char) *r)) {
	    /* white space; only copy if we haven't just copied a space */
	    if (!space) {
		space = TRUE;
		*w++ = ' ';
	    }			/* else ignore multiple spaces */
	} else {
	    /* non-space character; copy it and clear flag */
	    *w++ = *r;
	    space = FALSE;
	}
    }
    *w = NUL;			/* null terminate string */
}


/* postprocess single quoted strings: replace "''" by "'"
*/
void
parse_sq(char *instr)
{
    char *s = instr, *t = instr;

    /* the string will always get shorter, so we can do the
     * conversion in situ
     */

    while (*s != NUL) {
        if (*s == '\'' && *(s+1) == '\'')
            s++;
        *t++ = *s++;
    }
    *t = NUL;
}


void
parse_esc(char *instr)
{
    char *s = instr, *t = instr;

    /* the string will always get shorter, so we can do the
     * conversion in situ
     */

    while (*s != NUL) {
	if (*s == '\\') {
	    s++;
	    if (*s == '\\') {
		*t++ = '\\';
		s++;
	    } else if (*s == 'n') {
		*t++ = '\n';
		s++;
	    } else if (*s == 'r') {
		*t++ = '\r';
		s++;
	    } else if (*s == 't') {
		*t++ = '\t';
		s++;
	    } else if (*s == '\"') {
		*t++ = '\"';
		s++;
	    } else if (*s >= '0' && *s <= '7') {
		int i, n;
		char *octal = (*s == '0' ? "%4o%n" : "%3o%n");
		if (sscanf(s, octal, &i, &n) > 0) {
		    *t++ = i;
		    s += n;
		} else {
		    /* int_error("illegal octal number ", c_token); */
		    *t++ = '\\';
		    *t++ = *s++;
		}
	    }
	} else if (df_separator && *s == '\"' && *(s+1) == '\"') {
	/* EAM Mar 2003 - For parsing CSV strings with quoted quotes */
	    *t++ = *s++; s++;
	} else {
	    *t++ = *s++;
	}
    }
    *t = NUL;
}


/* FIXME HH 20020915: This function does nothing if dirent.h and windows.h
 * not available. */
TBOOLEAN
existdir (const char *name)
{
#ifdef HAVE_DIRENT_H
    DIR *dp;
    if (! (dp = opendir(name) ) )
	return FALSE;

    closedir(dp);
    return TRUE;
#elif defined(_Windows)
    HANDLE FileHandle;
    WIN32_FIND_DATA finddata;

    FileHandle = FindFirstFile(name, &finddata);
    if (FileHandle != INVALID_HANDLE_VALUE) {
	if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	    return TRUE;
    }
    return FALSE;
#elif defined(VMS)
    return FALSE;
#else
    int_warn(NO_CARET,
	     "Test on directory existence not supported\n\t('%s!')",
	     name);
    return FALSE;
#endif
}

char *
getusername ()
{
    char *username = NULL;
    char *fullname = NULL;

    username=getenv("USER");
    if (!username)
	username=getenv("USERNAME");

#ifdef HAVE_PWD_H
    if (username) {
	struct passwd *pwentry = NULL;
	pwentry=getpwnam(username);
	if (pwentry && strlen(pwentry->pw_gecos)) {
	    fullname = gp_alloc(strlen(pwentry->pw_gecos)+1,"getusername");
	    strcpy(fullname, pwentry->pw_gecos);
	} else {
	    fullname = gp_alloc(strlen(username)+1,"getusername");
	    strcpy(fullname, username);
	}
    }
#elif defined(_Windows)
    if (username) {
	DWORD bufCharCount = INFO_BUFFER_SIZE;
	fullname = gp_alloc(INFO_BUFFER_SIZE + 1,"getusername");
	if (!GetUserName(fullname,&bufCharCount)) {
	    free(fullname);
	    fullname = NULL;
	}
    }
#else
    fullname = gp_alloc(strlen(username)+1,"getusername");
    strcpy(fullname, username);
#endif /* HAVE_PWD_H */

    return fullname;
}

TBOOLEAN contains8bit(const char *s)
{
    while (*s) {
	if ((*s++ & 0x80))
	    return TRUE;
    }
    return FALSE;
}

#define INVALID_UTF8 0xfffful

/* Read from second byte to end of UTF-8 sequence.
   used by utftoulong() */
TBOOLEAN
utf8_getmore (unsigned long * wch, const char **str, int nbytes)
{
  int i;
  unsigned char c;
  unsigned long minvalue[] = {0x80, 0x800, 0x10000, 0x200000, 0x4000000};

  for (i = 0; i < nbytes; i++) {
    c = (unsigned char) **str;
  
    if ((c & 0xc0) != 0x80) {
      *wch = INVALID_UTF8;
      return FALSE;
    }
    *wch = (*wch << 6) | (c & 0x3f);
    (*str)++;
  }

  /* check for overlong UTF-8 sequences */
  if (*wch < minvalue[nbytes-1]) {
    *wch = INVALID_UTF8;
    return FALSE;
  }
  return TRUE;
}

/* Convert UTF-8 multibyte sequence from string to unsigned long character.
   Returns TRUE on success.
*/
TBOOLEAN
utf8toulong (unsigned long * wch, const char ** str)
{
  unsigned char c;

  c =  (unsigned char) *(*str)++;
  if ((c & 0x80) == 0) {
    *wch = (unsigned long) c;
    return TRUE;
  }

  if ((c & 0xe0) == 0xc0) {
    *wch = c & 0x1f;
    return utf8_getmore(wch, str, 1);
  }

  if ((c & 0xf0) == 0xe0) {
    *wch = c & 0x0f;
    return utf8_getmore(wch, str, 2);
  }

  if ((c & 0xf8) == 0xf0) {
    *wch = c & 0x07;
    return utf8_getmore(wch, str, 3);
  }

  if ((c & 0xfc) == 0xf8) {
    *wch = c & 0x03;
    return utf8_getmore(wch, str, 4);
  }

  if ((c & 0xfe) == 0xfc) {
    *wch = c & 0x01;
    return utf8_getmore(wch, str, 5);
  }

  *wch = INVALID_UTF8;
  return FALSE;
}
