#ifndef lint
static char *RCSid() { return RCSid("$Id: datafile.c,v 1.16.2.3 2000/06/14 00:38:37 joze Exp $"); }
#endif

/* GNUPLOT - datafile.c */

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

/* AUTHOR : David Denholm */

/*
 * this file provides the functions to handle data-file reading..
 * takes care of all the pipe / stdin / index / using worries
 */

/*{{{  notes */
/* couldn't decide how to implement 'thru' only for 2d and 'index'
 * for only 3d, so I did them for both - I can see a use for
 * index in 2d, especially for fit.
 *
 * I keep thru for backwards compatibility, and extend it to allow
 * more natural plot 'data' thru f(y) - I (personally) prefer
 * my syntax, but then I'm biased...
 *
 * - because I needed it, I have added a range of indexes...
 * (s)plot 'data' [index i[:j]]
 *
 * also every a:b:c:d:e:f  - plot every a'th point from c to e,
 * in every b lines from d to f
 * ie for (line=d; line<=f; line+=b)
 *     for (point=c; point >=e; point+=a)
 *
 *
 * I dont like mixing this with the time series hack... I am
 * very into modular code, so I would prefer to not have to
 * have _anything_ to do with time series... for example,
 * we just look at columns in file, and that is independent
 * of 2d/3d. I really dont want to have to pass a flag to
 * this is plot or splot. 
 *
 * use a global array df_timecol[] - cleared by df_open, then
 * columns needing time set by client.
 *
 * Now that df_2dbinary() and df_3dbinary() are here, I am seriously
 * tempted to move get_data() and get_3ddata() in here too
 *
 * public variables declared in this file.
 *    int df_no_use_specs - number of columns specified with 'using'
 *    int df_line_number  - for error reporting
 *    int df_datum        - increases with each data point
 *    TBOOLEAN df_binary  - it's a binary file
 *        [ might change this to return value from df_open() ]
 *    int df_eof          - end of file
 *    int df_timecol[]    - client controls which cols read as time
 *
 * functions 
 *   int df_open(int max_using)
 *      parses thru / index / using on command line
 *      max_using is max no of 'using' columns allowed
 *      returns number of 'using' cols specified, or -1 on error (?)
 *
 *   int df_readline(double vector[], int max)
 *      reads a line, does all the 'index' and 'using' manipulation
 *      deposits values into vector[]
 *      returns
 *          number of columns parsed  [0=not blank line, but no valid data],
 *          DF_EOF for EOF
 *          DF_UNDEFINED - undefined result during eval of extended using spec
 *          DF_FIRST_BLANK for first consecutive blank line
 *          DF_SECOND_BLANK for second consecutive blank line
 *            will return FIRST before SECOND
 *
 * if a using spec was given, lines not fulfilling spec are ignored.
 * we will always return exactly the number of items specified
 *
 * if no spec given, we return number of consecutive columns we parsed.
 * 
 * if we are processing indexes, separated by 'n' blank lines,
 * we will return n-1 blank lines before noticing the index change
 *
 *   void df_close()
 *     closes a currently open file.
 *
 *    void f_dollars(x)
 *    void f_column()    actions for expressions using $i, column(j), etc
 *    void f_valid()     
 * 
 *
 * line parsing slightly differently from previous versions of gnuplot...
 * given a line containing fewer columns than asked for, gnuplot used to make
 * up values... I say that if I have explicitly said 'using 1:2:3', then if
 * column 3 doesn't exist, I dont want this point...
 *
 * a column number of 0 means generate a value... as before, this value
 * is useful in 2d as an x value, and is reset at blank lines.
 * a column number of -1 means the (data) line number (not the file line
 * number).  splot 'file' using 1  is equivalent to
 * splot 'file' using 0:-1:1
 * column number -2 is the index. It was put in to kludge multi-branch
 * fitting.
 *
 * 20/5/95 : accept 1.23d4 in place of e (but not in scanf string)
 *         : autoextend data line buffer and MAX_COLS
 *
 * 11/8/96 : add 'columns' -1 for suggested y value, and -2 for
 *           current index.
 *           using 1:-1:-2  and  column(-1)  are supported.
 *           $-1 and $-2 are not yet supported, because of the
 *           way the parser works
 *
 */
/*}}} */

#include "datafile.h"

#include "alloc.h"
#include "command.h"
#include "binary.h"
#include "gp_time.h"
#include "graphics.h"
#include "internal.h"
#include "misc.h"
#include "parse.h"
#include "setshow.h"
#include "util.h"

/* if you change this, change the scanf in readline */
#define NCOL   7		/* max using specs     */

/*{{{  static fns */
#if 0				/* not used */
static int get_time_cols __PROTO((char *fmt));
static void mod_def_usespec __PROTO((int specno, int jump));
#endif
static int check_missing __PROTO((char *s));
static char *df_gets __PROTO((void));
static int df_tokenise __PROTO((char *s));
static float **df_read_matrix __PROTO((int *rows, int *columns));
static void plot_option_every __PROTO((void));
static void plot_option_index __PROTO((void));
static void plot_option_thru __PROTO((void));
static void plot_option_using __PROTO((int));
static TBOOLEAN valid_format __PROTO((const char *));

/*}}} */

/*{{{  variables */
struct use_spec_s {
    int column;
    struct at_type *at;
};

/* public variables client might access */

int df_no_use_specs;		/* how many using columns were specified */
int df_line_number;
int df_datum;			/* suggested x value if none given */
TBOOLEAN df_matrix = FALSE;	/* is this a matrix splot */
int df_eof = 0;
int df_timecol[NCOL];
TBOOLEAN df_binary = FALSE;	/* this is a binary file */

/* jev -- the 'thru' function --- NULL means no dummy vars active */
/* HBB 990829: moved this here, from command.c */
struct udft_entry ydata_func;


/* private variables */

/* in order to allow arbitrary data line length, we need to use the heap
 * might consider free-ing it in df_close, especially for small systems
 */
static char *line = NULL;
static size_t max_line_len = 0;
#define DATA_LINE_BUFSIZ 160

static FILE *data_fp = NULL;
static TBOOLEAN pipe_open = FALSE;
static TBOOLEAN mixed_data_fp = FALSE;

#ifndef MAXINT			/* should there be one already defined ? */
# ifdef INT_MAX			/* in limits.h ? */
#  define MAXINT INT_MAX
# else
#  define MAXINT ((~0)>>1)
# endif
#endif

/* stuff for implementing index */
static int blank_count = 0;	/* how many blank lines recently */
static int df_lower_index = 0;	/* first mesh required */
static int df_upper_index = MAXINT;
static int df_index_step = 1;	/* 'every' for indices */
static int df_current_index;	/* current mesh */

/* stuff for every point:line */
static int everypoint = 1;
static int firstpoint = 0;
static int lastpoint = MAXINT;
static int everyline = 1;
static int firstline = 0;
static int lastline = MAXINT;
static int point_count = -1;	/* point counter - preincrement and test 0 */
static int line_count = 0;	/* line counter */

/* parsing stuff */
static struct use_spec_s use_spec[NCOL];
static char df_format[MAX_LINE_LEN + 1];

/* rather than three arrays which all grow dynamically, make one
 * dynamic array of this structure
 */

typedef struct df_column_struct {
    double datum;
    enum {
	DF_MISSING, DF_BAD, DF_GOOD
    } good;
    char *position;
} df_column_struct;

static df_column_struct *df_column = NULL;	/* we'll allocate space as needed */
static int df_max_cols = 0;	/* space allocated */
static int df_no_cols;		/* cols read */
static int fast_columns;	/* corey@cac optimization */

/* columns needing timefmt are passed in df_timecol[] after df_open */

/*}}} */


/*{{{  static char *df_gets() */
static char *
df_gets()
{
    int len = 0;

    /* HBB 20000526: prompt user for inline data, if in interactive mode */
    if (mixed_data_fp && interactive)
	fputs("input data ('e' ends) > ", stderr);

    if (!fgets(line, max_line_len, data_fp))
	return NULL;

    if (mixed_data_fp)
	++inline_num;

    for (;;) {
	len += strlen(line + len);

	if (len > 0 && line[len - 1] == '\n') {
	    /* we have read an entire text-file line.
	     * Strip the trailing linefeed and return
	     */
	    line[len - 1] = 0;
	    return line;
	}
	/* buffer we provided may not be full - dont grab extra
	 * memory un-necessarily. This may trap a problem with last
	 * line in file not being properly terminated - each time
	 * through a replot loop, it was doubling buffer size
	 */

	if ((max_line_len - len) < 32)
	    line = gp_realloc(line, max_line_len *= 2, "datafile line buffer");

	if (!fgets(line + len, max_line_len - len, data_fp))
	    return line;	/* unexpected end of file, but we have something to do */
    }

    /* NOTREACHED */
    return NULL;
}

/*}}} */

/*{{{  static int df_tokenise(s) */
static int
df_tokenise(s)
char *s;
{
    /* implement our own sscanf that takes 'missing' into account,
     * and can understand fortran quad format
     */

    df_no_cols = 0;

    while (*s) {

	/* check store - double max cols or add 20, whichever is greater */
	if (df_max_cols <= df_no_cols)
	    df_column = (df_column_struct *) gp_realloc(df_column,
							(df_max_cols += (df_max_cols < 20 ? 20 : df_max_cols)) * sizeof(df_column_struct),
							"datafile column");

	/* have always skipped spaces at this point */
	df_column[df_no_cols].position = s;

	if (check_missing(s))
	    df_column[df_no_cols].good = DF_MISSING;
	else {
#ifdef OSK
	    /* apparently %n does not work. This implementation
	     * is just as good as the non-OSK one, but close
	     * to a release (at last) we make it os-9 specific
	     */
	    int count;
	    char *p = strpbrk(s, "dqDQ");
	    if (p != NULL)
		*p = 'e';

	    count = sscanf(s, "%lf", &df_column[df_no_cols].datum);
#else
	    /* cannot trust strtod - eg strtod("-",&p) */
	    int used;
	    int count;
	    int dfncp1 = df_no_cols + 1;

/*
 * optimizations by Corey Satten, corey@cac.washington.edu
 */
	    if ((fast_columns == 0)
		|| (df_no_use_specs == 0)
		|| ((df_no_use_specs > 0)
		    && (use_spec[0].column == dfncp1
			|| (df_no_use_specs > 1
			    && (use_spec[1].column == dfncp1
				|| (df_no_use_specs > 2
				    && (use_spec[2].column == dfncp1
					|| (df_no_use_specs > 3
					    && (use_spec[3].column == dfncp1
						|| (df_no_use_specs > 4 && (use_spec[4].column == dfncp1 || df_no_use_specs > 5)
						)
					    )
					)
				    )
				)
			    )
			)
		    )
		)
		) {

#ifndef NO_FORTRAN_NUMS
		count = sscanf(s, "%lf%n", &df_column[df_no_cols].datum, &used);
#else
		while (isspace(*s))
		    ++s;
		count = *s ? 1 : 0;
		df_column[df_no_cols].datum = atof(s);
#endif /* NO_FORTRAN_NUMS */
	    } else {
		/* skip any space at start of column */
		/* HBB tells me that the cast must be to
		 * unsigned char instead of int. */
		while (isspace((unsigned char) *s))
		    ++s;
		count = *s ? 1 : 0;
		/* skip chars to end of column */
		used = 0;
		while (!isspace((unsigned char) *s) && (*s != NUL))
		    ++s;
	    }

	    /* it might be a fortran double or quad precision.
	     * 'used' is only safe if count is 1
	     */

#ifndef NO_FORTRAN_NUMS
	    if (count == 1 &&
		(s[used] == 'd' || s[used] == 'D' ||
		 s[used] == 'q' || s[used] == 'Q')) {
		/* might be fortran double */
		s[used] = 'e';
		/* and try again */
		count = sscanf(s, "%lf", &df_column[df_no_cols].datum);
	    }
#endif /* NO_FORTRAN_NUMS */
#endif /* OSK */
	    df_column[df_no_cols].good = count == 1 ? DF_GOOD : DF_BAD;
	}

	++df_no_cols;
	/*{{{  skip chars to end of column */
	while ((!isspace((int) *s)) && (*s != '\0'))
	    ++s;
	/*}}} */
	/*{{{  skip spaces to start of next column */
	while (isspace((int) *s))
	    ++s;
	/*}}} */
    }

    return df_no_cols;
}

/*}}} */

/*{{{  static float **df_read_matrix() */
/* reads a matrix from a text file
 * stores in same storage format as fread_matrix
 */

static float **
df_read_matrix(rows, cols)
int *rows, *cols;
{
    int max_rows = 0;
    int c;
    float **rmatrix = NULL;

    char *s;

    *rows = 0;
    *cols = 0;

    for (;;) {
	if (!(s = df_gets())) {
	    df_eof = 1;
	    return rmatrix;	/* NULL if we have not read anything yet */
	}
	while (isspace((int) *s))
	    ++s;

	if (!*s || is_comment(*s)) {
	    if (rmatrix)
		return rmatrix;
	    else
		continue;
	}
	if (mixed_data_fp && is_EOF(*s)) {
	    df_eof = 1;
	    return rmatrix;
	}
	c = df_tokenise(s);

	if (!c)
	    return rmatrix;

	if (*cols && c != *cols) {
	    /* its not regular */
	    int_error(NO_CARET, "Matrix does not represent a grid");
	}
	*cols = c;

	if (*rows >= max_rows) {
	    rmatrix = gp_realloc(rmatrix, (max_rows += 10) * sizeof(float *), "df_matrix");
	}
	/* allocate a row and store data */
	{
	    int i;
	    float *row = rmatrix[*rows] = (float *) gp_alloc(c * sizeof(float),
							     "df_matrix row");

	    for (i = 0; i < c; ++i) {
		if (df_column[i].good != DF_GOOD)
		    int_error(NO_CARET, "Bad number in matrix");

		row[i] = (float) df_column[i].datum;
	    }

	    ++*rows;
	}
    }
}

/*}}} */


/*{{{  int df_open(max_using) */
int
df_open(max_using)
int max_using;

/* open file, parsing using/thru/index stuff
 * return number of using specs  [well, we have to return something !]
 */

{
    /* now allocated dynamically */
    static char *filename = NULL;
    int i;
    int name_token;

    fast_columns = 1;		/* corey@cac */

    /*{{{  close file if necessary */
    if (data_fp)
	df_close();
    /*}}} */

    /*{{{  initialise static variables */
    df_format[0] = NUL;		/* no format string */

    df_no_use_specs = 0;

    for (i = 0; i < NCOL; ++i) {
	use_spec[i].column = i + 1;	/* default column */
	use_spec[i].at = NULL;	/* no expression */
    }

    if (max_using > NCOL)
	max_using = NCOL;

    df_datum = -1;		/* it will be preincremented before use */
    df_line_number = 0;		/* ditto */

    df_lower_index = 0;
    df_index_step = 1;
    df_upper_index = MAXINT;

    df_current_index = 0;
    blank_count = 2;
    /* by initialising blank_count, leading blanks will be ignored */

    everypoint = everyline = 1;	/* unless there is an every spec */
    firstpoint = firstline = 0;
    lastpoint = lastline = MAXINT;

    df_eof = 0;

    memset(df_timecol, 0, sizeof(df_timecol));

    df_binary = 1;
    /*}}} */

    assert(max_using <= NCOL);

    /* empty name means re-use last one */
    if (isstring(c_token) && token_len(c_token) == 2) {
	if (!filename || !*filename)
	    int_error(c_token, "No previous filename");
    } else {
	filename = gp_realloc(filename, token_len(c_token), "datafile name");
	quote_str(filename, c_token, token_len(c_token));
    }

    name_token = c_token++;

    /* defer opening until we have parsed the modifiers... */

    /*{{{  look for binary / matrix */
    df_binary = df_matrix = FALSE;

    if (almost_equals(c_token, "bin$ary")) {
	++c_token;
	df_binary = TRUE;
	df_matrix = TRUE;
    } else if (almost_equals(c_token, "mat$rix")) {
	++c_token;
	df_matrix = TRUE;
    }
    /*}}} */

    /*{{{  deal with index */
    if (almost_equals(c_token, "i$ndex")) {
	plot_option_index();
    }
    /*}}} */

    /*{{{  deal with every */
    if (almost_equals(c_token, "ev$ery")) {
	plot_option_every();
    }
    /*}}} */

    /*{{{  deal with thru */
    /* jev -- support for passing data from file thru user function */

    if (ydata_func.at)
	free(ydata_func.at);
    ydata_func.at = NULL;

    if (almost_equals(c_token, "thru$")) {
	plot_option_thru();
    }
    /*}}} */

    /*{{{  deal with using */
    if (almost_equals(c_token, "u$sing")) {
	plot_option_using(max_using);
    }
    /*}}} */

    /*{{{  more variable inits */
    point_count = -1;		/* we preincrement */
    line_count = 0;

    /* here so it's not done for every line in df_readline */
    if (max_line_len < DATA_LINE_BUFSIZ) {
	max_line_len = DATA_LINE_BUFSIZ;
	line = (char *) gp_alloc(max_line_len, "datafile line buffer");
    }
    /*}}} */

    /* filename cannot be static array! */
    gp_expand_tilde(&filename);

/*{{{  open file */
#if defined(PIPES)
    if (*filename == '<') {
	if ((data_fp = popen(filename + 1, "r")) == (FILE *) NULL)
	    os_error(name_token, "cannot create pipe for data");
	else
	    pipe_open = TRUE;
    } else
#endif /* PIPES */
	/* I don't want to call strcmp(). Does it make a difference? */
    if (*filename == '-' && strlen(filename) == 1) {
	plotted_data_from_stdin = 1;
	data_fp = lf_top();
	if (!data_fp)
	    data_fp = stdin;
	mixed_data_fp = TRUE;	/* don't close command file */
    } else {
#ifdef HAVE_SYS_STAT_H
	struct stat statbuf;

	if ((stat(filename, &statbuf) > -1) &&
	    !S_ISREG(statbuf.st_mode) && !S_ISFIFO(statbuf.st_mode)) {
	    os_error(name_token, "\"%s\" is not a regular file or pipe", filename);
	}
#endif /* HAVE_SYS_STAT_H */
	if ((data_fp = loadpath_fopen(filename, df_binary ? "rb" : "r")) == (FILE *) NULL) {
	    os_error(name_token, "can't read data file \"%s\"", filename);
	}
    }
/*}}} */

    return df_no_use_specs;
}

/*}}} */

/*{{{  void df_close() */
void
df_close()
{
    int i;
    /* paranoid - mark $n and column(n) as invalid */
    df_no_cols = 0;

    if (!data_fp)
	return;

    if (ydata_func.at) {
	free(ydata_func.at);
	ydata_func.at = NULL;
    }
    /*{{{  free any use expression storage */
    for (i = 0; i < df_no_use_specs; ++i)
	if (use_spec[i].at) {
	    free(use_spec[i].at);
	    use_spec[i].at = NULL;
	}
    /*}}} */

    if (!mixed_data_fp) {
#if defined(PIPES)
	if (pipe_open) {
	    (void) pclose(data_fp);
	    pipe_open = FALSE;
	} else
#endif /* PIPES */
	    (void) fclose(data_fp);
    }
    mixed_data_fp = FALSE;
    data_fp = NULL;
}

/*}}} */


static void
plot_option_every()
{
    struct value a;

    fast_columns = 0;		/* corey@cac */
    /* allow empty fields - every a:b:c::e
     * we have already established the defaults
     */

    if (!equals(++c_token, ":")) {
	everypoint = (int) real(const_express(&a));
	if (everypoint < 1)
	    int_error(c_token, "Expected positive integer");
    }
    /* if it fails on first test, no more tests will succeed. If it
     * fails on second test, next test will succeed with correct c_token
     */
    if (equals(c_token, ":") && !equals(++c_token, ":")) {
	everyline = (int) real(const_express(&a));
	if (everyline < 1)
	    int_error(c_token, "Expected positive integer");
    }
    if (equals(c_token, ":") && !equals(++c_token, ":")) {
	firstpoint = (int) real(const_express(&a));
	if (firstpoint < 0)
	    int_error(c_token, "Expected non-negative integer");
    }
    if (equals(c_token, ":") && !equals(++c_token, ":")) {
	firstline = (int) real(const_express(&a));
	if (firstline < 0)
	    int_error(c_token, "Expected non-negative integer");
    }
    if (equals(c_token, ":") && !equals(++c_token, ":")) {
	lastpoint = (int) real(const_express(&a));
	if (lastpoint < firstpoint)
	    int_error(c_token, "Last point must not be before first point");
    }
    if (equals(c_token, ":")) {
	++c_token;
	lastline = (int) real(const_express(&a));
	if (lastline < firstline)
	    int_error(c_token, "Last line must not be before first line");
    }
}


static void
plot_option_index()
{
    struct value a;

    if (df_binary)
	int_error(c_token, "Binary file format does not allow more than one surface per file");

    ++c_token;
    df_lower_index = (int) real(const_express(&a));
    if (equals(c_token, ":")) {
	++c_token;
	df_upper_index = (int) magnitude(const_express(&a));
	if (df_upper_index < df_lower_index)
	    int_error(c_token, "Upper index should be bigger than lower index");

	if (equals(c_token, ":")) {
	    ++c_token;
	    df_index_step = (int) magnitude(const_express(&a));
	    if (df_index_step < 1)
		int_error(c_token, "Index step must be positive");
	}
    } else
	df_upper_index = df_lower_index;
}


static void
plot_option_thru()
{
    c_token++;
    strcpy(c_dummy_var[0], dummy_var[0]);
    /* allow y also as a dummy variable.
     * during plot, c_dummy_var[0] and [1] are 'sacred'
     * ie may be set by  splot [u=1:2] [v=1:2], and these
     * names are stored only in c_dummy_var[]
     * so choose dummy var 2 - can anything vital be here ?
     */
    dummy_func = &ydata_func;
    strcpy(c_dummy_var[2], "y");
    ydata_func.at = perm_at();
    dummy_func = NULL;
}


static void
plot_option_using(max_using)
int max_using;
{
    if (!END_OF_COMMAND && !isstring(++c_token)) {
	struct value a;

	do {			/* must be at least one */
	    if (df_no_use_specs >= max_using)
		int_error(c_token, "Too many columns in using specification");

	    if (equals(c_token, ":")) {
		/* empty specification - use default */
		use_spec[df_no_use_specs].column = df_no_use_specs;
		++df_no_use_specs;
		/* do not increment c+token ; let while() find the : */
	    } else if (equals(c_token, "(")) {
		fast_columns = 0;	/* corey@cac */
		dummy_func = NULL;	/* no dummy variables active */
		use_spec[df_no_use_specs++].at = perm_at();	/* it will match ()'s */
	    } else {
		int col = (int) real(const_express(&a));
		if (col < -2)
		    int_error(c_token, "Column must be >= -2");
		use_spec[df_no_use_specs++].column = col;
	    }
	} while (equals(c_token, ":") && ++c_token);
    }
    if (!END_OF_COMMAND && isstring(c_token)) {
	if (df_binary)
	    int_error(NO_CARET, "Format string meaningless with binary data");

	quote_str(df_format, c_token, MAX_LINE_LEN);
	if (!valid_format(df_format))
	    int_error(c_token, "Please use a double conversion %lf");

	c_token++;		/* skip format */
    }
}


/*{{{  int df_readline(v, max) */
/* do the hard work... read lines from file,
 * - use blanks to get index number
 * - ignore lines outside range of indices required
 * - fill v[] based on using spec if given
 */

int
df_readline(v, max)
double v[];
int max;
{
    char *s;

    assert(data_fp != NULL);
    assert(!df_binary);
    assert(max_line_len);	/* alloc-ed in df_open() */
    assert(max <= NCOL);

    /* catch attempt to read past EOF on mixed-input */
    if (df_eof)
	return DF_EOF;

    while ((s = df_gets()) != NULL)
	/*{{{  process line */
    {
	int line_okay = 1;
	int output = 0;		/* how many numbers written to v[] */

	++df_line_number;
	df_no_cols = 0;

	/*{{{  check for blank lines, and reject by index/every */
	/*{{{  skip leading spaces */
	while (isspace((int) *s))
	    ++s;		/* will skip the \n too, to point at \0  */
	/*}}} */

	/*{{{  skip comments */
	if (is_comment(*s))
	    continue;		/* ignore comments */
	/*}}} */

	/*{{{  check EOF on mixed data */
	if (mixed_data_fp && is_EOF(*s)) {
	    df_eof = 1;		/* trap attempts to read past EOF */
	    return DF_EOF;
	}
	/*}}} */

	/*{{{  its a blank line - update counters and continue or return */
	if (*s == 0) {
	    /* argh - this is complicated !  we need to
	     *   ignore it if we haven't reached first index
	     *   report EOF if passed last index
	     *   report blank line unless we've already done 2 blank lines
	     *
	     * - I have probably missed some obvious way of doing all this,
	     * but its getting late
	     */

	    point_count = -1;	/* restart counter within line */

	    if (++blank_count == 1) {
		/* first blank line */
		++line_count;
	    }
	    /* just reached end of a group/surface */
	    if (blank_count == 2) {
		++df_current_index;
		line_count = 0;
		df_datum = -1;
		/* ignore line if current_index has just become
		 * first required one - client doesn't want this
		 * blank line. While we're here, check for <=
		 * - we need to do it outside this conditional, but
		 * probably no extra cost at assembler level
		 */
		if (df_current_index <= df_lower_index)
		    continue;	/* dont tell client */

		/* df_upper_index is MAXINT-1 if we are not doing index */
		if (df_current_index > df_upper_index) {
		    /* oops - need to gobble rest of input if mixed */
		    if (mixed_data_fp)
			continue;
		    else {
			df_eof = 1;
			return DF_EOF;	/* no point continuing */
		    }
		}
	    }
	    /* dont tell client if we haven't reached first index */
	    if (df_current_index < df_lower_index)
		continue;

	    /* ignore blank lines after blank_index */
	    if (blank_count > 2)
		continue;

	    return DF_FIRST_BLANK - (blank_count - 1);
	}
	/*}}} */

	/* get here => was not blank */

	blank_count = 0;

	/*{{{  ignore points outside range of index */
	/* we try to return end-of-file as soon as we pass upper index,
	 * but for mixed input stream, we must skip garbage
	 */

	if (df_current_index < df_lower_index ||
	    df_current_index > df_upper_index ||
	    ((df_current_index - df_lower_index) % df_index_step) != 0)
	    continue;
	/*}}} */

	/*{{{  reject points by every */
	/* accept only lines with (line_count%everyline) == 0 */

	if (line_count < firstline || line_count > lastline ||
	    (line_count - firstline) % everyline != 0)
	    continue;

	/* update point_count. ignore point if point_count%everypoint != 0 */

	if (++point_count < firstpoint || point_count > lastpoint ||
	    (point_count - firstpoint) % everypoint != 0)
	    continue;
	/*}}} */
	/*}}} */

	++df_datum;

	/*{{{  do a sscanf */
	if (*df_format) {
	    int i;

	    assert(NCOL == 7);

	    /* check we have room for at least 7 columns */
	    if (df_max_cols < 7) {
		df_max_cols = 7;
		df_column = (df_column_struct *) gp_realloc(df_column,
							    df_max_cols * sizeof(df_column_struct), "datafile columns");
	    }

	    df_no_cols = sscanf(line, df_format,
				&df_column[0].datum,
				&df_column[1].datum,
				&df_column[2].datum,
				&df_column[3].datum,
				&df_column[4].datum,
				&df_column[5].datum,
				&df_column[6].datum);

	    if (df_no_cols == EOF) {
		df_eof = 1;
		return DF_EOF;	/* tell client */
	    }
	    for (i = 0; i < df_no_cols; ++i) {	/* may be zero */
		df_column[i].good = DF_GOOD;
		df_column[i].position = NULL;	/* cant get a time */
	    }
	}
	/*}}} */
	else
	    df_tokenise(s);

	/*{{{  copy column[] to v[] via use[] */
	{
	    int limit = (df_no_use_specs ? df_no_use_specs : NCOL);
	    if (limit > max)
		limit = max;

	    for (output = 0; output < limit; ++output) {
		/* if there was no using spec, column is output+1 and at=NULL */
		int column = use_spec[output].column;

		if (use_spec[output].at) {
		    struct value a;
		    /* no dummy values to set up prior to... */
		    evaluate_at(use_spec[output].at, &a);
		    if (undefined)
			return DF_UNDEFINED;	/* store undefined point in plot */

		    v[output] = real(&a);
		} else if (column == -2) {
		    v[output] = df_current_index;
		} else if (column == -1) {
		    v[output] = line_count;
		} else if (column == 0) {
		    v[output] = df_datum;	/* using 0 */
		} else if (column <= 0)	/* really < -2, but */
		    int_error(NO_CARET, "internal error: column <= 0 in datafile.c");
		else if (df_timecol[output]) {
		    struct tm tm;
		    if (column > df_no_cols ||
			df_column[column - 1].good == DF_MISSING ||
			!df_column[column - 1].position ||
			!gstrptime(df_column[column - 1].position, timefmt, &tm)
			) {
			/* line bad only if user explicitly asked for this column */
			if (df_no_use_specs)
			    line_okay = 0;

			/* return or ignore line depending on line_okay */
			break;
		    }
		    v[output] = (double) gtimegm(&tm);
		} else {	/* column > 0 */
		    if ((column <= df_no_cols) && df_column[column - 1].good == DF_GOOD)
			v[output] = df_column[column - 1].datum;
		    else {
			/* line bad only if user explicitly asked for this column */
			if (df_no_use_specs)
			    line_okay = 0;
			break;	/* return or ignore depending on line_okay */
		    }
		}
	    }
	}
	/*}}} */

	if (!line_okay)
	    continue;

	/* output == df_no_use_specs if using was specified
	 * - actually, smaller of df_no_use_specs and max
	 */
	assert(df_no_use_specs == 0 || output == df_no_use_specs || output == max);

	return output;

    }
    /*}}} */

    /* get here => fgets failed */

    /* no longer needed - mark column(x) as invalid */
    df_no_cols = 0;

    df_eof = 1;
    return DF_EOF;
}

/*}}} */

/*{{{  int df_2dbinary(this_plot) */
int
df_2dbinary(this_plot)
struct curve_points *this_plot;
{
    int_error(NO_CARET, "Binary file format for 2d data not yet defined");
    return 0;			/* keep compiler happy */
}

/*}}} */

/*{{{  int df_3dmatrix(this_plot, ret_this_iso) */
/*
 * formerly in gnubin.c
 *
 * modified by div for 3.6
 *   obey the 'every' field from df_open
 *   outrange points are marked as such, not omitted
 *   obey using - treat x as column 1, y as col 2 and z as col 3
 *   ( ie $1 gets x, $2 gets y, $3 gets z)
 *
 *  we are less optimal for case of log plot and no using spec,
 * (call log too often) but that is price for flexibility
 * I suspect it didn't do autoscaling of x and y for log scale
 * properly ?
 *
 * Trouble figuring out file format ! Is it

 width  x1  x2  x3  x4  x5 ...
 y1   z11 z12 z13 z14 z15 ...
 y2   x21 z22 z23 .....
 .    .
 .        .
 .             .

 * with perhaps x and y swapped...
 *
 * - presumably rows continue to end of file, hence no indexing...
 *
 * Last update: 3/3/92 for Gnuplot 3.24.
 * Created from code for written by RKC for gnuplot 2.0b.
 *
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 *
 * Copyright (c) 1991,1992 Robert K. Cunningham, MIT Lincoln Laboratory
 *
 */

/*
 * Here we keep putting new plots onto the end of the linked list
 *
 * We assume the data's x,y values have x1<x2, x2<x3... and 
 * y1<y2, y2<y3... .
 * Actually, I think the assumption is less strong than that--it looks like
 * the direction just has to be the same.
 * This routine expects the following to be properly initialized:
 * is_log_x, is_log_y, and is_log_z 
 * base_log_x, base_log_y, and base_log_z 
 * log_base_log_x, log_base_log_y, and log_base_log_z 
 * xmin,ymin, and zmin
 * xmax,ymax, and zmax
 * autoscale_lx, autoscale_ly, and autoscale_lz
 *
 * does the autoscaling into the array versions (min_array[], max_array[])
 */

int
df_3dmatrix(this_plot)
struct surface_points *this_plot;
{
    float GPFAR *GPFAR * dmatrix, GPFAR * rt, GPFAR * ct;
    int nr, nc;
    int width, height;
    int row, col;
    struct iso_curve *this_iso;
    double used[3];		/* output from using manip */
    struct coordinate GPHUGE *point;	/* HBB 980308: added 'GPHUGE' flag */

    assert(df_matrix);

    if (df_eof)
	return 0;		/* hope caller understands this */

    if (df_binary) {
	if (!fread_matrix(data_fp, &dmatrix, &nr, &nc, &rt, &ct))
	    int_error(NO_CARET, "Binary file read error: format unknown!");
	/* fread_matrix() drains the file */
	df_eof = 1;
    } else {
	if (!(dmatrix = df_read_matrix(&nr, &nc))) {
	    df_eof = 1;
	    return 0;
	}
	rt = NULL;
	ct = NULL;
    }

    if (nc == 0 || nr == 0)
	int_error(NO_CARET, "Read grid of zero height or zero width");

    this_plot->plot_type = DATA3D;
    this_plot->has_grid_topology = TRUE;

    if (df_no_use_specs != 0 && df_no_use_specs != 3)
	int_error(NO_CARET, "Current implementation requires full using spec");

    if (df_max_cols < 3) {
	df_max_cols = 3;
	df_column = (df_column_struct *) gp_realloc(df_column,
						    df_max_cols * sizeof(df_column_struct), "datafile columns");
    }

    df_no_cols = 3;
    df_column[0].good = df_column[1].good = df_column[2].good = DF_GOOD;

    assert(everyline > 0);
    assert(everypoint > 0);
    width = (nc - firstpoint + everypoint - 1) / everypoint;	/* ? ? ? ? ? */
    height = (nr - firstline + everyline - 1) / everyline;	/* ? ? ? ? ? */

    for (row = firstline; row < nr; row += everyline) {
	df_column[1].datum = rt ? rt[row] : row;

	/*Allocate the correct number of entries */
	this_iso = iso_alloc(width);

	point = this_iso->points;

	/* Cycle through data */
	for (col = firstpoint; col < nc; col += everypoint, ++point) {
	    /*{{{  process one point */
	    int i;

	    df_column[0].datum = ct ? ct[col] : col;
	    df_column[2].datum = dmatrix[row][col];

	    /*{{{  pass through using spec */
	    for (i = 0; i < 3; ++i) {
		int column = use_spec[i].column;

		if (df_no_use_specs == 0)
		    used[i] = df_column[i].datum;
		else if (use_spec[i].at) {
		    struct value a;
		    evaluate_at(use_spec[i].at, &a);
		    if (undefined) {
			point->type = UNDEFINED;
			goto skip;	/* continue _outer_ loop */
		    }
		    used[i] = real(&a);
		} else if (column < 1 || column > df_no_cols) {
		    point->type = UNDEFINED;
		    goto skip;
		} else
		    used[i] = df_column[column - 1].datum;
	    }
	    /*}}} */

	    point->type = INRANGE;	/* so far */

	    /*{{{  autoscaling/clipping */
	    /*{{{  autoscale/range-check x */
	    if (used[0] > 0 || !is_log_x) {
		if (used[0] < min_array[FIRST_X_AXIS]) {
		    if (autoscale_lx & 1)
			min_array[FIRST_X_AXIS] = used[0];
		    else
			point->type = OUTRANGE;
		}
		if (used[0] > max_array[FIRST_X_AXIS]) {
		    if (autoscale_lx & 2)
			max_array[FIRST_X_AXIS] = used[0];
		    else
			point->type = OUTRANGE;
		}
	    }
	    /*}}} */

	    /*{{{  autoscale/range-check y */
	    if (used[1] > 0 || !is_log_y) {
		if (used[1] < min_array[FIRST_Y_AXIS]) {
		    if (autoscale_ly & 1)
			min_array[FIRST_Y_AXIS] = used[1];
		    else
			point->type = OUTRANGE;
		}
		if (used[1] > max_array[FIRST_Y_AXIS]) {
		    if (autoscale_ly & 2)
			max_array[FIRST_Y_AXIS] = used[1];
		    else
			point->type = OUTRANGE;
		}
	    }
	    /*}}} */

	    /*{{{  autoscale/range-check z */
	    if (used[2] > 0 || !is_log_z) {
		if (used[2] < min_array[FIRST_Z_AXIS]) {
		    if (autoscale_lz & 1)
			min_array[FIRST_Z_AXIS] = used[2];
		    else
			point->type = OUTRANGE;
		}
		if (used[2] > max_array[FIRST_Z_AXIS]) {
		    if (autoscale_lz & 2)
			max_array[FIRST_Z_AXIS] = used[2];
		    else
			point->type = OUTRANGE;
		}
	    }
	    /*}}} */
	    /*}}} */

	    /*{{{  log x */
	    if (is_log_x) {
		if (used[0] < 0.0) {
		    point->type = UNDEFINED;
		    goto skip;
		} else if (used[0] == 0.0) {
		    point->type = OUTRANGE;
		    used[0] = -VERYLARGE;
		} else
		    used[0] = log(used[0]) / log_base_log_x;
	    }
	    /*}}} */

	    /*{{{  log y */
	    if (is_log_y) {
		if (used[1] < 0.0) {
		    point->type = UNDEFINED;
		    goto skip;
		} else if (used[1] == 0.0) {
		    point->type = OUTRANGE;
		    used[1] = -VERYLARGE;
		} else
		    used[1] = log(used[1]) / log_base_log_y;
	    }
	    /*}}} */

	    /*{{{  log z */
	    if (is_log_z) {
		if (used[2] < 0.0) {
		    point->type = UNDEFINED;
		    goto skip;
		} else if (used[2] == 0.0) {
		    point->type = OUTRANGE;
		    used[2] = -VERYLARGE;
		} else
		    used[2] = log(used[2]) / log_base_log_z;
	    }
	    /*}}} */

	    point->x = used[0];
	    point->y = used[1];
	    point->z = used[2];


	    /* some of you wont like this, but I say goto is for this */

	  skip:
	    ;			/* ansi requires this */
	    /*}}} */
	}
	this_iso->p_count = width;
	this_iso->next = this_plot->iso_crvs;
	this_plot->iso_crvs = this_iso;
	this_plot->num_iso_read++;
    }

    free_matrix(dmatrix, 0, nr - 1, 0, nc - 1);
    if (rt)
	free_vector(rt, 0, nr - 1);
    if (ct)
	free_vector(ct, 0, nc - 1);
    return (nc);
}

/*}}} */

/* stuff for implementing the call-backs for picking up data values
 * do it here so we can make the variables private to this file
 */

/*{{{  void f_dollars(x) */
void
f_dollars(x)
union argument *x;
{
    int column = x->v_arg.v.int_val - 1;
    /* we checked it was an integer >= 0 at compile time */
    struct value a;

    if (column == -1) {
	push(Gcomplex(&a, (double) df_datum, 0.0));	/* $0 */
    } else if (column >= df_no_cols || df_column[column].good != DF_GOOD) {
	undefined = TRUE;
	push(&(x->v_arg));	/* this okay ? */
    } else
	push(Gcomplex(&a, df_column[column].datum, 0.0));
}

/*}}} */

/*{{{  void f_column() */
void
f_column()
{
    struct value a;
    int column;
    (void) pop(&a);
    column = (int) real(&a) - 1;
    if (column == -2)
	push(Ginteger(&a, line_count));
    else if (column == -1)	/* $0 = df_datum */
	push(Gcomplex(&a, (double) df_datum, 0.0));
    else if (column < 0 || column >= df_no_cols || df_column[column].good != DF_GOOD) {
	undefined = TRUE;
	push(&a);		/* any objection to this ? */
    } else
	push(Gcomplex(&a, df_column[column].datum, 0.0));
}

/*}}} */

/*{{{  void f_valid() */
void
f_valid()
{
    struct value a;
    int column, good;
    (void) pop(&a);
    column = (int) magnitude(&a) - 1;
    good = column >= 0 && column < df_no_cols && df_column[column].good == DF_GOOD;
    push(Ginteger(&a, good));
}

/*}}} */

/*{{{  void f_timecolumn() */
void
f_timecolumn()
{
    struct value a;
    int column;
    struct tm tm;
    (void) pop(&a);
    column = (int) magnitude(&a) - 1;
    if (column < 0 || column >= df_no_cols ||
	!df_column[column].position ||
	!gstrptime(df_column[column].position, timefmt, &tm)) {
	undefined = TRUE;
	push(&a);		/* any objection to this ? */
    } else
	push(Gcomplex(&a, gtimegm(&tm), 0.0));
}

/*}}} */

#if 0				/* not used */
/* count columns in timefmt */
/*{{{  static int get_time_cols(fmt) */
static int
get_time_cols(fmt)
char *fmt;			/* format string */
{
    int cnt, i;
    char *p;

    p = fmt;
    cnt = 0;
    while (isspace(*p))
	p++;
    if (!strlen(p))
	int_error(NO_CARET, "Empty time-data format");
    cnt++;
    for (i = 0; i < strlen(p) - 1; i++) {
	if (isspace(p[i]) && !isspace(p[i + 1]))
	    cnt++;
    }
    return (cnt);
}

/*}}} */

/* modify default use_spec, applies for no user spec and time datacolumns */
/*{{{  static void mod_def_usespec(specno,jump) */
static void
mod_def_usespec(specno, jump)
int specno;			/* which spec in ?:?:? */
int jump;			/* no of columns in timefmt (time data) */
{
    int i;

    for (i = specno + 1; i < NCOL; ++i)
	use_spec[i].column += jump;	/* add no of columns in time to the rest */
    df_no_use_specs = 0;
}

/*}}} */
#endif /* not used */

/*{{{  static int check_missing(s) */
static int
check_missing(s)
char *s;
{
    if (missing_val != NULL) {
	size_t len = strlen(missing_val);
	if (strncmp(s, missing_val, len) == 0 &&
	    (isspace((int) s[len]) || !s[len]))
	    return 1;	/* store undefined point in plot */
    }
    return (0);
}

/*}}} */


/* formerly in misc.c, but only used here */
/* check user defined format strings for valid double conversions */
static TBOOLEAN
valid_format(format)
const char *format;
{
    for (;;) {
	if (!(format = strchr(format, '%')))	/* look for format spec  */
	    return TRUE;	/* passed Test           */
	do {			/* scan format statement */
	    format++;
	} while (strchr("+-#0123456789.", *format));

	switch (*format) {	/* Now at format modifier */
	case '*':		/* Ignore '*' statements */
	case '%':		/* Char   '%' itself     */
	    format++;
	    continue;
	case 'l':		/* Now we found it !!! */
	    if (!strchr("fFeEgG", format[1]))	/* looking for a valid format */
		return FALSE;
	    format++;
	    break;
	default:
	    return FALSE;
	}
    }
}
