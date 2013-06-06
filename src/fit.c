#ifndef lint
static char *RCSid() { return RCSid("$Id: fit.c,v 1.115 2013/06/02 06:49:52 markisch Exp $"); }
#endif

/*  NOTICE: Change of Copyright Status
 *
 *  The author of this module, Carsten Grammes, has expressed in
 *  personal email that he has no more interest in this code, and
 *  doesn't claim any copyright. He has agreed to put this module
 *  into the public domain.
 *
 *  Lars Hecking  15-02-1999
 */

/*
 *    Nonlinear least squares fit according to the
 *      Marquardt-Levenberg-algorithm
 *
 *      added as Patch to Gnuplot (v3.2 and higher)
 *      by Carsten Grammes
 *
 * Michele Marziani (marziani@ferrara.infn.it), 930726: Recoding of the
 * Unix-like raw console I/O routines
 *
 * drd: start unitialised variables at 1 rather than NEARLY_ZERO
 *  (fit is more likely to converge if started from 1 than 1e-30 ?)
 *
 * HBB (broeker@physik.rwth-aachen.de) : fit didn't calculate the errors
 * in the 'physically correct' (:-) way, if a third data column containing
 * the errors (or 'uncertainties') of the input data was given. I think
 * I've fixed that, but I'm not sure I really understood the M-L-algo well
 * enough to get it right. I deduced my change from the final steps of the
 * equivalent algorithm for the linear case, which is much easier to
 * understand. (I also made some minor, mostly cosmetic changes)
 *
 * HBB (again): added error checking for negative covar[i][i] values and
 * for too many parameters being specified.
 *
 * drd: allow 3d fitting. Data value is now called fit_z internally,
 * ie a 2d fit is z vs x, and a 3d fit is z vs x and y.
 *
 * Lars Hecking : review update command, for VMS in particular, where
 * it is not necessary to rename the old file.
 *
 * HBB, 971023: lifted fixed limit on number of datapoints, and number
 * of parameters.
 *
 * HBB/H.Harders, 20020927: log file name now changeable from inside
 * gnuplot, not only by setting an environment variable.
 *
 * Jim Van Zandt, 090201: allow fitting functions with up to five
 * independent variables.
 *
 * Carl Michal, 120311: optionally prescale all the parameters that
 * the L-M routine sees by their initial values, so that parameters
 * that differ by many orders of magnitude do not cause problems.
 * With decent initial guesses, fits often take fewer iterations. If
 * any variables were 0, then don't do it for those variables, since
 * it may do more harm than good.
 *
 * Thomas Mattison, 130421: brief one-line reports, based on patchset #230.
 * Bastian Maerkisch, 130421: different output verbosity levels
 *
 * Bastian Maerkisch, 130427: remember parameters etc. of last fit and use
 * this data in a subsequent update command if the parameter file does not
 * exists yet.
 *
 * Thomas Mattison, 130508: New convergence criterion which is absolute
 * reduction in chisquare for an iteration of less than epsilon*chisquare
 * plus epsilon_abs (new setting).  The default convergence criterion is
 * always relative no matter what the chisquare is, but users now have the
 * flexibility of adding an absolute convergence criterion through
 * `set fit limit_abs`. Patchset #230.
 *
 */

#include "fit.h"
#include "alloc.h"
#include "axis.h"
#include "command.h"
#include "datafile.h"
#include "eval.h"
#include "gp_time.h"
#include "matrix.h"
#include "misc.h"
#include "plot.h"
#include "setshow.h"
#include "scanner.h"  /* For legal_identifier() */
#include "specfun.h"
#include "util.h"
#include "variable.h" /* For locale handling */
#include <signal.h>

/* Just temporary */
#if defined(VA_START) && defined(STDC_HEADERS)
static void Dblfn __PROTO((const char *fmt, ...));
#else
static void Dblfn __PROTO(());
#endif
#define Dblf  Dblfn
#define Dblf2 Dblfn
#define Dblf3 Dblfn
#define Dblf5 Dblfn
#define Dblf6 Dblfn

#if defined(MSDOS) 	/* non-blocking IO stuff */
# include <io.h>
# include <conio.h>
# include <dos.h>
#else /* !(MSDOS) */
# ifndef VMS
#  include <fcntl.h>
# endif				/* !VMS */
#endif /* !(MSDOS) */
#ifdef WIN32
# include "win/winmain.h"
#endif

/* constants */

#ifdef INFINITY
# undef INFINITY
#endif

#define INFINITY    1e30
#define NEARLY_ZERO 1e-30

/* create new variables with this value (was NEARLY_ZERO) */
#define INITIAL_VALUE 1.0

/* Relative change for derivatives */
#define DELTA       0.001

#define MAX_DATA    2048
#define MAX_PARAMS  32
#define MAX_LAMBDA  1e20
#define MIN_LAMBDA  1e-20
#define LAMBDA_UP_FACTOR 10
#define LAMBDA_DOWN_FACTOR 10

#if defined(MSDOS) || defined(OS2)
# define PLUSMINUS   "\xF1"	/* plusminus sign */
#else
# define PLUSMINUS   "+/-"
#endif

#define LASTFITCMDLENGTH 511

/* compatible with gnuplot philosophy */
#define STANDARD stderr

/* Suffix of a backup file */
#define BACKUP_SUFFIX ".old"


/* type definitions */
enum marq_res {
    OK, ML_ERROR, BETTER, WORSE
};
typedef enum marq_res marq_res_t;

typedef char fixstr[MAX_ID_LEN+1];


/* externally visible variables: */

/* fit control */
char *fitlogfile = NULL;
TBOOLEAN fit_errorvariables = FALSE;
verbosity_level fit_verbosity = BRIEF;
TBOOLEAN fit_errorscaling = TRUE;
TBOOLEAN fit_prescale = FALSE;
char *fit_script = NULL;
int fit_wrap = 0;

/* names of user control variables */
const char * FITLIMIT = "FIT_LIMIT";
const char * FITSTARTLAMBDA = "FIT_START_LAMBDA";
const char * FITLAMBDAFACTOR = "FIT_LAMBDA_FACTOR";
const char * FITMAXITER = "FIT_MAXITER";

TBOOLEAN ctrlc_flag = FALSE;
char fitbuf[256]; /* for Eex and error_ex */

/* private variables: */

static double epsilon = DEF_FIT_LIMIT;	/* relative convergence limit */
double epsilon_abs = 0.0;               /* default to zero non-relative limit */
static int maxiter = 0;
static double startup_lambda = 0;
static double lambda_down_factor = LAMBDA_DOWN_FACTOR;
static double lambda_up_factor = LAMBDA_UP_FACTOR;

static const char fitlogfile_default[] = "fit.log";
static const char GNUFITLOG[] = "FIT_LOG";
static FILE *log_f = NULL;
static const char *GP_FIXED = "# FIXED";
static const char *FITSCRIPT = "FIT_SCRIPT";
static const char *DEFAULT_CMD = "replot";	/* if no fitscript spec. */

static int num_data, num_params;
static int num_indep;	 /* # independent variables in fit function */
static int columns;	 /* # values read from data file for each point */
static double *fit_x = 0;	/* all independent variable values,
				   e.g. value of the ith variable from
				   the jth data point is in
				   fit_x[j*num_indep+i] */
static double *fit_z = 0;	/* dependent data values */
static double *err_data = 0;	/* standard deviations of dependent data */
static double *a = 0;		/* array of fitting parameters */
static TBOOLEAN user_stop = FALSE;
static double *scale_params = 0; /* scaling values for parameters */
static struct udft_entry func;
static fixstr *par_name;

static fixstr *last_par_name = NULL;
static int last_num_params = 0;
static char last_dummy_var[5][MAX_ID_LEN+1];
static char last_fit_command[LASTFITCMDLENGTH+1] = "";


/*****************************************************************
			 internal Prototypes
*****************************************************************/

#if !defined(WIN32) || defined(WGP_CONSOLE)
static RETSIGTYPE ctrlc_handle __PROTO((int an_int));
#endif
static void ctrlc_setup __PROTO((void));
static marq_res_t marquardt __PROTO((double a[], double **alpha, double *chisq,
				     double *lambda));
static TBOOLEAN analyze __PROTO((double a[], double **alpha, double beta[],
				 double *chisq));
static void calculate __PROTO((double *zfunc, double **dzda, double a[]));
static void call_gnuplot __PROTO((double *par, double *data));
static TBOOLEAN fit_interrupt __PROTO((void));
static TBOOLEAN regress __PROTO((double a[]));
static void show_fit __PROTO((int i, double chisq, double last_chisq, double *a,
			      double lambda, FILE * device));
static void show_fit1 __PROTO((int iter, double chisq, double last_chisq, double *parms,
                            double lambda, FILE * device));
static void show_results __PROTO((double chisq, double last_chisq, double* a, double* dpar, double** corel));
static void log_axis_restriction __PROTO((FILE *log_f, AXIS_INDEX axis, char *name));
static TBOOLEAN is_empty __PROTO((char *s));
static int getivar __PROTO((const char *varname));
static double getdvar __PROTO((const char *varname));
static double createdvar __PROTO((char *varname, double value));
static void setvar __PROTO((char *varname, double value));
static void setvarerr __PROTO((char *varname, double value));
static char *get_next_word __PROTO((char **s, char *subst));
static void backup_file __PROTO((char *, const char *));


/*****************************************************************
    Small function to write the last fit command into a file
    Arg: Pointer to the file; if NULL, nothing is written,
         but only the size of the string is returned.
*****************************************************************/

size_t
wri_to_fil_last_fit_cmd(FILE *fp)
{
    if (fp == NULL)
	return strlen(last_fit_command);
    else
	return (size_t) fputs(last_fit_command, fp);
}


/*****************************************************************
    This is called when a SIGINT occurs during fit
*****************************************************************/

#if !defined(WIN32) || defined(WGP_CONSOLE)
static RETSIGTYPE
ctrlc_handle(int an_int)
{
    (void) an_int;		/* avoid -Wunused warning */
    /* reinstall signal handler (necessary on SysV) */
    (void) signal(SIGINT, (sigfunc) ctrlc_handle);
    ctrlc_flag = TRUE;
}
#endif


/*****************************************************************
    setup the ctrl_c signal handler
*****************************************************************/
static void
ctrlc_setup()
{
/*
 *  MSDOS defines signal(SIGINT) but doesn't handle it through
 *  real interrupts. So there remain cases in which a ctrl-c may
 *  be uncaught by signal. We must use kbhit() instead that really
 *  serves the keyboard interrupt (or write an own interrupt func
 *  which also generates #ifdefs)
 *
 *  I hope that other OSes do it better, if not... add #ifdefs :-(
 */
#if (defined(__EMX__) || !defined(MSDOS)) && (!defined(WIN32) || defined(WGP_CONSOLE))
    (void) signal(SIGINT, (sigfunc) ctrlc_handle);
#endif
}


/*****************************************************************
    getch that handles also function keys etc.
*****************************************************************/
#if defined(MSDOS)

/* HBB 980317: added a prototype... */
int getchx __PROTO((void));

int
getchx()
{
    int c = getch();
    if (!c || c == 0xE0) {
	c <<= 8;
	c |= getch();
    }
    return c;
}
#endif


/*****************************************************************
    in case of fatal errors
*****************************************************************/
void
error_ex()
{
    char *sp;

    memcpy(fitbuf, "         ", 9);	/* start after GNUPLOT> */
    sp = strchr(fitbuf, NUL);
    while (*--sp == '\n');
    strcpy(sp + 1, "\n");
    fputs(fitbuf, STANDARD);
    strcpy(sp + 2, "\n");	/* terminate with exactly 2 newlines */
    if (log_f) {
	fprintf(log_f, "BREAK: %s", fitbuf);
	(void) fclose(log_f);
	log_f = NULL;
    }
    if (func.at) {
	free_at(func.at);		/* release perm. action table */
	func.at = (struct at_type *) NULL;
    }
    /* restore original SIGINT function */
    interrupt_setup();

    /* exit via int_error() so that it can clean up state variables */
    int_error(NO_CARET, "error during fit");
}


/* HBB 990829: removed the debug print routines */


/*****************************************************************
    Marquardt's nonlinear least squares fit
*****************************************************************/
static marq_res_t
marquardt(double a[], double **C, double *chisq, double *lambda)
{
    int i, j;
    static double *da = 0,	/* delta-step of the parameter */
    *temp_a = 0,		/* temptative new params set   */
    *d = 0, *tmp_d = 0, **tmp_C = 0, *residues = 0;
    double tmp_chisq;

    /* Initialization when lambda == -1 */

    if (*lambda == -1) {	/* Get first chi-square check */
	TBOOLEAN analyze_ret;

	temp_a = vec(num_params);
	d = vec(num_data + num_params);
	tmp_d = vec(num_data + num_params);
	da = vec(num_params);
	residues = vec(num_data + num_params);
	tmp_C = matr(num_data + num_params, num_params);

	analyze_ret = analyze(a, C, d, chisq);

	/* Calculate a useful startup value for lambda, as given by Schwarz */
	/* FIXME: this is doesn't turn out to be much better, really... */
	if (startup_lambda != 0)
	    *lambda = startup_lambda;
	else {
	    *lambda = 0;
	    for (i = 0; i < num_data; i++)
		for (j = 0; j < num_params; j++)
		    *lambda += C[i][j] * C[i][j];
	    *lambda = sqrt(*lambda / num_data / num_params);
	}

	/* Fill in the lower square part of C (the diagonal is filled in on
	   each iteration, see below) */
	for (i = 0; i < num_params; i++)
	    for (j = 0; j < i; j++)
		C[num_data + i][j] = 0, C[num_data + j][i] = 0;
	return analyze_ret ? OK : ML_ERROR;
    }
    /* once converged, free dynamic allocated vars */

    if (*lambda == -2) {
	free(d);
	free(tmp_d);
	free(da);
	free(temp_a);
	free(residues);
	free_matr(tmp_C);
	return OK;
    }

    /* Givens calculates in-place, so make working copies of C and d */

    for (j = 0; j < num_data + num_params; j++)
	memcpy(tmp_C[j], C[j], num_params * sizeof(double));
    memcpy(tmp_d, d, num_data * sizeof(double));

    /* fill in additional parts of tmp_C, tmp_d */

    for (i = 0; i < num_params; i++) {
	/* fill in low diag. of tmp_C ... */
	tmp_C[num_data + i][i] = *lambda;
	/* ... and low part of tmp_d */
	tmp_d[num_data + i] = 0;
    }

    /* FIXME: residues[] isn't used at all. Why? Should it be used? */
    Givens(tmp_C, tmp_d, da, residues, num_params + num_data, num_params, 1);

    /* check if trial did ameliorate sum of squares */
    for (j = 0; j < num_params; j++)
	temp_a[j] = a[j] + da[j];

    if (!analyze(temp_a, tmp_C, tmp_d, &tmp_chisq)) {
	/* FIXME: will never be reached: always returns TRUE */
	return ML_ERROR;
    }

    /* tsm patchset 230: Changed < to <= in next line */
    /* so finding exact minimum stops iteration instead of just increasing lambda. */
    /* Disadvantage is that if lambda is large enough so that chisq doesn't change */
    /* is taken as success. */
    if (tmp_chisq <= *chisq) {	/* Success, accept new solution */
	if (*lambda > MIN_LAMBDA) {
	    if (fit_verbosity == VERBOSE)
		putc('/', stderr);
	    *lambda /= lambda_down_factor;
	}
	/* update chisq, C, d, a */
	*chisq = tmp_chisq;
	for (j = 0; j < num_data; j++) {
	    memcpy(C[j], tmp_C[j], num_params * sizeof(double));
	    d[j] = tmp_d[j];
	}
	for (j = 0; j < num_params; j++)
	    a[j] = temp_a[j];
	return BETTER;
    } else {			/* failure, increase lambda and return */
	*lambda *= lambda_up_factor;
	if (fit_verbosity == VERBOSE)
	    (void) putc('*', stderr);
	else if (fit_verbosity == BRIEF)  /* one-line report even if chisq increases */
	    show_fit1(-1, tmp_chisq, *chisq, temp_a, *lambda, STANDARD);

	return WORSE;
    }
}


/*****************************************************************
    compute chi-square and numeric derivations
*****************************************************************/
/* Used by marquardt to evaluate the linearized fitting matrix C and
 * vector d. Fills in only the top part of C and d. I don't use a
 * temporary array zfunc[] any more. Just use d[] instead.  */
static TBOOLEAN
analyze(double a[], double **C, double d[], double *chisq)
{
    int i, j;

    calculate(d, C, a);

    for (i = 0; i < num_data; i++) {
	/* note: order reversed, as used by Schwarz */
	d[i] = (d[i] - fit_z[i]) / err_data[i];
	for (j = 0; j < num_params; j++)
	    C[i][j] /= err_data[i];
    }
    *chisq = sumsq_vec(num_data, d);

    /* FIXME: why return a value that is always TRUE ? */
    return TRUE;
}


/*****************************************************************
    compute function values and partial derivatives of chi-square
*****************************************************************/
/* To use the more exact, but slower two-side formula, activate the
   following line: */
/*#define TWO_SIDE_DIFFERENTIATION */
static void
calculate(double *zfunc, double **dzda, double a[])
{
    int k, p;
    double tmp_a;
    double *tmp_high, *tmp_pars;
#ifdef TWO_SIDE_DIFFERENTIATION
    double *tmp_low;
#endif

    tmp_high = vec(num_data);	/* numeric derivations */
#ifdef TWO_SIDE_DIFFERENTIATION
    tmp_low = vec(num_data);
#endif
    tmp_pars = vec(num_params);

    /* first function values */

    call_gnuplot(a, zfunc);

    /* then derivatives */

    for (p = 0; p < num_params; p++)
	tmp_pars[p] = a[p];
    for (p = 0; p < num_params; p++) {
	tmp_a = fabs(a[p]) < NEARLY_ZERO ? NEARLY_ZERO : a[p];
	tmp_pars[p] = tmp_a * (1 + DELTA);
	call_gnuplot(tmp_pars, tmp_high);
#ifdef TWO_SIDE_DIFFERENTIATION
	tmp_pars[p] = tmp_a * (1 - DELTA);
	call_gnuplot(tmp_pars, tmp_low);
#endif
	for (k = 0; k < num_data; k++)
#ifdef TWO_SIDE_DIFFERENTIATION
	    dzda[k][p] = (tmp_high[k] - tmp_low[k]) / (2 * tmp_a * DELTA);
#else
	    dzda[k][p] = (tmp_high[k] - zfunc[k]) / (tmp_a * DELTA);
#endif
	tmp_pars[p] = a[p];
    }

#ifdef TWO_SIDE_DIFFERENTIATION
    free(tmp_low);
#endif
    free(tmp_high);
    free(tmp_pars);
}


/*****************************************************************
    call internal gnuplot functions
*****************************************************************/
static void
call_gnuplot(double *par, double *data)
{
    int i, j;
    struct value v;

    /* set parameters first */
    for (i = 0; i < num_params; i++)
	setvar(par_name[i], par[i] * scale_params[i]);

    for (i = 0; i < num_data; i++) {
	/* calculate fit-function value */
	/* initialize extra dummy variables from the corresponding
	 actual variables, if any. */
	for (j = 0; j < 5; j++) {
	    struct udvt_entry *udv = add_udv_by_name(c_dummy_var[j]);
	    Gcomplex(&func.dummy_values[j],
	             udv->udv_undef ? 0 : getdvar(c_dummy_var[j]),
	             0.0);
	}
	/* set actual dummy variables from file data */
	for (j = 0; j < num_indep; j++)
	    Gcomplex(&func.dummy_values[j],
	             fit_x[i * num_indep + j], 0.0);
	evaluate_at(func.at, &v);

	data[i] = real(&v);
	if (undefined || isnan(data[i])) {
	    /* Print useful info on undefined-function error. */
	    Dblf("\nCurrent data point\n");
	    Dblf("=========================\n");
	    Dblf3("%-15s = %i out of %i\n", "#", i + 1, num_data);
	    for (j = 0; j < num_indep; j++)
		Dblf3("%-15.15s = %-15g\n", c_dummy_var[j], par[j] * scale_params[j]);
	    Dblf3("%-15.15s = %-15g\n", "z", fit_z[i]);
	    Dblf("\nCurrent set of parameters\n");
	    Dblf("=========================\n");
	    for (j = 0; j < num_params; j++)
		Dblf3("%-15.15s = %-15g\n", par_name[j], par[j] * scale_params[j]);
	    Dblf("\n");
	    /* FIXME: Eex() aborts before memory is freed. */
	    if (undefined) {
		Eex("Undefined value during function evaluation");
	    } else {
		Eex("Function evaluation yields NaN (\"not a number\")");
	    }
	}
    }
}


/*****************************************************************
    handle user interrupts during fit
*****************************************************************/
static TBOOLEAN
fit_interrupt()
{
    while (TRUE) {
	fputs("\n\n(S)top fit, (C)ontinue, (E)xecute FIT_SCRIPT:  ", STANDARD);
#ifdef WIN32
	WinRaiseConsole();
#endif
	switch (getchar()) {

	case EOF:
	case 's':
	case 'S':
	    fputs("Stop.\n", STANDARD);
	    user_stop = TRUE;
	    return FALSE;

	case 'c':
	case 'C':
	    fputs("Continue.\n", STANDARD);
	    return TRUE;

	case 'e':
	case 'E':{
		int i;
		char *tmp;

		tmp = getfitscript();
		fprintf(STANDARD, "executing: %s\n", tmp);
		/* FIXME: Shouldn't we also set FIT_STDFIT etc? */
		/* set parameters visible to gnuplot */
		for (i = 0; i < num_params; i++)
		    setvar(par_name[i], a[i] * scale_params[i]);
		do_string(tmp);
		free(tmp);
	    }
	}
    }
    return TRUE;
}


/*****************************************************************
    determine current setting of FIT_SCRIPT
*****************************************************************/
char *
getfitscript(void)
{
    char *tmp;

    if (fit_script != NULL)
	return gp_strdup(fit_script);
    if ((tmp = getenv(FITSCRIPT)) != NULL)
	return gp_strdup(tmp);
    else
	return gp_strdup(DEFAULT_CMD);
}


/*****************************************************************
    frame routine for the marquardt-fit
*****************************************************************/
static TBOOLEAN
regress(double a[])
{
    double **covar, *dpar, **C, chisq, last_chisq, lambda;
    int iter, i, j;
    marq_res_t res;
    struct udvt_entry *v;	/* For exporting results to the user */

    chisq = last_chisq = INFINITY;
    C = matr(num_data + num_params, num_params);
    lambda = -1;		/* use sign as flag */
    iter = 0;			/* iteration counter  */

    /* ctrlc now serves as Hotkey */
    ctrlc_setup();

    /* Initialize internal variables and 1st chi-square check */
    if ((res = marquardt(a, C, &chisq, &lambda)) == ML_ERROR)
	Eex("FIT: error occurred during fit");
    res = BETTER;

    if (fit_verbosity == VERBOSE) {
	show_fit(iter, chisq, chisq, a, lambda, STANDARD);
	show_fit(iter, chisq, chisq, a, lambda, log_f);
    } else if (fit_verbosity == BRIEF) {
	show_fit1(iter, chisq, chisq, a, lambda, STANDARD);
	show_fit1(iter, chisq, chisq, a, lambda, log_f);
    }

    /* Reset flag describing fit result status */
    v = add_udv_by_name("FIT_CONVERGED");
    v->udv_undef = FALSE;
    Ginteger(&v->udv_value, 0);

    /* MAIN FIT LOOP: do the regression iteration */

    /* HBB 981118: initialize new variable 'user_break' */
    user_stop = FALSE;
    /* FIXME: This really should not be necessary, but it is not properly initialised in wgnuplot otherwise. */
    ctrlc_flag = FALSE;

    do {
/*
 *  MSDOS defines signal(SIGINT) but doesn't handle it through
 *  real interrupts. So there remain cases in which a ctrl-c may
 *  be uncaught by signal. We must use kbhit() instead that really
 *  serves the keyboard interrupt (or write an own interrupt func
 *  which also generates #ifdefs)
 *
 *  I hope that other OSes do it better, if not... add #ifdefs :-(
 *  EMX does not have kbhit.
 *
 *  HBB: I think this can be enabled for DJGPP V2. SIGINT is actually
 *  handled there, AFAIK.
 */
#if (defined(MSDOS) && !defined(__EMX__))
	if (kbhit()) {
	    do {
		getchx();
	    } while (kbhit());
	    ctrlc_flag = TRUE;
	}
#endif
#ifdef WIN32
	/* This call makes the Windows GUI functional during fits.
	   Pressing Ctrl-Break now finally has an effect. */
	WinMessageLoop();
#endif

	if (ctrlc_flag) {
	    /* Always report on current status. */
	    if (fit_verbosity == VERBOSE)
		show_fit(iter, chisq, last_chisq, a, lambda, STANDARD);
	    else
		show_fit1(iter, chisq, last_chisq, a, lambda, STANDARD);

	    ctrlc_flag = FALSE;
	    if (!fit_interrupt())	/* handle keys */
		break;
	}
	if (res == BETTER) {
	    iter++;
	    last_chisq = chisq;
	}
	if ((res = marquardt(a, C, &chisq, &lambda)) == BETTER) {
	    if (fit_verbosity == VERBOSE)
		show_fit(iter, chisq, last_chisq, a, lambda, STANDARD);
	    else if (fit_verbosity == BRIEF)
		show_fit1(iter, chisq, last_chisq, a, lambda, STANDARD);
	}
    } while ((res != ML_ERROR)
	     && (lambda < MAX_LAMBDA)
	     && ((maxiter == 0) || (iter <= maxiter))
	     && (res == WORSE ||
	    /* tsm patchset 230: change to new convergence criterion */
	         ((last_chisq - chisq) > (epsilon * chisq + epsilon_abs)))
	);

    /* fit done */

    /* tsm patchset 230: final progress report labels to console */
    if (fit_verbosity == BRIEF)
	show_fit1(-2, chisq, chisq, a, lambda, STANDARD);

    /* tsm patchset 230: final progress report to log file */
    if (fit_verbosity == VERBOSE)
	show_fit(iter, chisq, last_chisq, a, lambda, log_f);
    else
	show_fit1(iter, chisq, last_chisq, a, lambda, log_f);

    /* restore original SIGINT function */
    interrupt_setup();

    /* HBB 970304: the maxiter patch: */
    if ((maxiter > 0) && (iter > maxiter)) {
	Dblf2("\nMaximum iteration count (%d) reached. Fit stopped.\n", maxiter);
    } else if (user_stop) {
	Dblf2("\nThe fit was stopped by the user after %d iterations.\n", iter);
    } else if (lambda >= MAX_LAMBDA) {
	Dblf2("\nThe maximum lambda = %e was exceeded. Fit stopped.\n", MAX_LAMBDA);
    } else {
	Dblf2("\nAfter %d iterations the fit converged.\n", iter);
	v = add_udv_by_name("FIT_CONVERGED");
	v->udv_undef = FALSE;
	Ginteger(&v->udv_value, 1);
    }

    if (res == ML_ERROR)
	Eex("FIT: error occurred during fit");

    /* fit results */
    {
	int ndf          = num_data - num_params;
	double stdfit    = sqrt(chisq/ndf);
	double pvalue    = 1. - chisq_cdf(ndf, chisq);

	/* Export these to user-accessible variables */
	v = add_udv_by_name("FIT_NDF");
	v->udv_undef = FALSE;
	Ginteger(&v->udv_value, ndf);
	v = add_udv_by_name("FIT_STDFIT");
	v->udv_undef = FALSE;
	Gcomplex(&v->udv_value, stdfit, 0);
	v = add_udv_by_name("FIT_WSSR");
	v->udv_undef = FALSE;
	Gcomplex(&v->udv_value, chisq, 0);
	v = add_udv_by_name("FIT_P");
	v->udv_undef = FALSE;
	Gcomplex(&v->udv_value, pvalue, 0);
    }

    /* Save final parameters. This is necessary since they might have
       been changed in calculate() to obtain derivatives. */
    for (i = 0; i < num_params; i++)
	setvar(par_name[i], a[i] * scale_params[i]);

    /* compute errors in the parameters */
    if (fit_errorvariables)
	/* Set error variable to zero before doing this, */
	/* thus making sure they are created. */
	for (i = 0; i < num_params; i++)
	    setvarerr(par_name[i], 0.0);

    /* get covariance-, Correlations- and Kurvature-Matrix */
    /* and errors in the parameters                     */

    /* compute covar[][] directly from C */
    Givens(C, 0, 0, 0, num_data, num_params, 0);

    /* Use lower square of C for covar */
    covar = C + num_data;
    Invert_RtR(C, covar, num_params);

    /* calculate unscaled parameter errors in dpar[]: */
    dpar = vec(num_params);
    for (i = 0; i < num_params; i++) {
	/* FIXME: can this still happen ? */
	if (covar[i][i] <= 0.0)	/* HBB: prevent floating point exception later on */
	    Eex("Calculation error: non-positive diagonal element in covar. matrix");
	dpar[i] = sqrt(covar[i][i]);
    }

    /* transform covariances into correlations */
    for (i = 0; i < num_params; i++) {
	/* only lower triangle needs to be handled */
	for (j = 0; j <= i; j++)
	    covar[i][j] /= dpar[i] * dpar[j];
    }

    if ((fit_errorscaling) || (columns < 3)) {
	/* scale parameter errors based on chisq */
	chisq = sqrt(chisq / (num_data - num_params));
	for (i = 0; i < num_params; i++)
	    dpar[i] *= chisq;
    }

    /* Save user error variables. */
    for (i = 0; i < num_params; i++) {
	if (fit_errorvariables)
	    setvarerr(par_name[i], dpar[i] * scale_params[i]);
    }

    /* Report final results for VERBOSE, BRIEF, and RESULTS verbosity levels. */
    if (fit_verbosity != QUIET)
	show_results(chisq, last_chisq, a, dpar, covar);

    free(dpar);

    /* call destructor for allocated vars */
    lambda = -2;		/* flag value, meaning 'destruct!' */
    (void) marquardt(a, C, &chisq, &lambda);

    free_matr(C);
    return TRUE;
}


/*****************************************************************
    display results of the fit
*****************************************************************/
static void
show_results(double chisq, double last_chisq, double* a, double* dpar, double** corel)
{
    int i, j;

    Dblf2("final sum of squares of residuals : %g\n", chisq);
    if (chisq > NEARLY_ZERO) {
	Dblf2("rel. change during last iteration : %g\n\n", (chisq - last_chisq) / chisq);
    } else {
	Dblf2("abs. change during last iteration : %g\n\n", (chisq - last_chisq));
    }

    if ((num_data == num_params) && ((columns < 3) || fit_errorscaling)) {
	int k;

	Dblf("\nExactly as many data points as there are parameters.\n");
	Dblf("In this degenerate case, all errors are zero by definition.\n\n");
	Dblf("Final set of parameters \n");
	Dblf("======================= \n\n");
	for (k = 0; k < num_params; k++)
	    Dblf3("%-15.15s = %-15g\n", par_name[k], a[k] * scale_params[k]);
    } else if ((chisq < NEARLY_ZERO) && ((columns < 3) || fit_errorscaling)) {
	int k;

	Dblf("\nHmmmm.... Sum of squared residuals is zero. Can't compute errors.\n\n");
	Dblf("Final set of parameters \n");
	Dblf("======================= \n\n");
	for (k = 0; k < num_params; k++)
	    Dblf3("%-15.15s = %-15g\n", par_name[k], a[k] * scale_params[k]);
    } else {
	int ndf          = num_data - num_params;
	double stdfit    = sqrt(chisq/ndf);
	double pvalue    = 1. - chisq_cdf(ndf, chisq);

	Dblf2("degrees of freedom    (FIT_NDF)                        : %d\n", ndf);
	Dblf2("rms of residuals      (FIT_STDFIT) = sqrt(WSSR/ndf)    : %g\n", stdfit);
	Dblf2("variance of residuals (reduced chisquare) = WSSR/ndf   : %g\n", chisq / ndf);
	/* We cannot know if the errors supplied by the user are weighting factors
	or real errors, so we print the p-value in any case, although it does not
	make much sense in the first case.  This means that we always print this
	for x,y,z-fits without errors since current syntax requires 4 columns in
	this case. */
	if (columns >= 3)
	    Dblf2("p-value of the Chisq distribution (FIT_P)              : %g\n", pvalue);
	Dblf("\n");

	if ((fit_errorscaling) || (columns < 3))
	    Dblf("Final set of parameters            Asymptotic Standard Error\n");
	else
	    Dblf("Final set of parameters            Standard Deviation\n");
	Dblf("=======================            ==========================\n");

	for (i = 0; i < num_params; i++) {
	    double temp = (fabs(a[i]) < NEARLY_ZERO)
		? 0.0
		: fabs(100.0 * dpar[i] / a[i]);

	    Dblf6("%-15.15s = %-15g  %-3.3s %-12.4g (%.4g%%)\n",
		  par_name[i], a[i] * scale_params[i], PLUSMINUS, dpar[i] * scale_params[i], temp);
	}

	/* Print correlation matrix only if there is more than one parameter. */
	if (num_params > 1) {
	    Dblf("\ncorrelation matrix of the fit parameters:\n");

	    Dblf("                ");
	    for (j = 0; j < num_params; j++)
		Dblf2("%-6.6s ", par_name[j]);
	    Dblf("\n");

	    for (i = 0; i < num_params; i++) {
		Dblf2("%-15.15s", par_name[i]);
		for (j = 0; j <= i; j++) {
		    /* Only print lower triangle of symmetric matrix */
		    Dblf2("%6.3f ", corel[i][j]);
		}
		Dblf("\n");
	    }
	}
    }
}


/*****************************************************************
    display actual state of the fit
*****************************************************************/
static void
show_fit(int i, double chisq, double last_chisq, double* a, double lambda, FILE *device)
{
    int k;

    fprintf(device, "\n\n\
 Iteration %d\n\
 WSSR        : %-15g   delta(WSSR)/WSSR   : %g\n\
 delta(WSSR) : %-15g   limit for stopping : %g\n\
 lambda	  : %g\n\n%s parameter values\n\n",
	    i, chisq, chisq > NEARLY_ZERO ? (chisq - last_chisq) / chisq : 0.0,
	    chisq - last_chisq, epsilon, lambda,
	    (i > 0 ? "resultant" : "initial set of free"));
    for (k = 0; k < num_params; k++)
	fprintf(device, "%-15.15s = %g\n", par_name[k], a[k] * scale_params[k]);
}


/* If the exponent of a floating point number in scientific format (%e) has three
digits and the highest digit is zero, it will get removed by this routine. */
static char *
pack_float(char *num)
{
    static int needs_packing = -1;
    if (needs_packing < 0) {
	/* perform the test only once */
	char buf[12];
	snprintf(buf, sizeof(buf), "%.2e", 1.00); /* "1.00e+000" or "1.00e+00" */
	needs_packing = (strlen(buf) == 9);
    }
    if (needs_packing) {
	char *p = strchr(num, 'e');
	if (p == NULL)
	    p = strchr(num, 'E');
	if (p != NULL) {
	    p += 2;  /* also skip sign of exponent */
	    if (*p == '0') {
		do {
		    *p = *(p + 1);
		} while (*++p != NUL);
	    }
	}
    }
    return num;
}


/* tsm patchset 230: new one-line version of progress report */
static void
show_fit1(int iter, double chisq, double last_chisq, double* parms, double lambda, FILE *device)
{
    int k, len;
    double delta, lim;
    char buf[256];
    char *p;
    const int indent = 4;

    /* on iteration 0 or -2, print labels */
    if (iter == 0 || iter == -2) {
	strcpy(buf, "iter      chisq       delta/lim  lambda  ");
	          /* 9999 1.1234567890e+00 -1.12e+00 1.00e+00 */
	fputs(buf, device);
	len = strlen(buf);
	for (k = 0; k < num_params; k++) {
	    snprintf(buf, sizeof(buf), " %-13.13s", par_name[k]);
	    len += strlen(buf);
	    if ((fit_wrap > 0) && (len >= fit_wrap)) {
		fprintf(device, "\n%*c", indent, ' ');
		len = indent;
	    }
	    fputs(buf, device);
	}
	fputs("\n", device);
    }

    /* on iteration -2, don't print anything else */
    if (iter == -2) return;

    /* new convergence test quantities */
    delta = chisq - last_chisq;
    lim = epsilon * chisq + epsilon_abs;

    /* print values */
    if (iter >= 0)
	snprintf(buf, sizeof(buf), "%4i", iter);
    else /* -1 indicates that chisquare increased */
	snprintf(buf, sizeof(buf), "%4c", '*');
    snprintf(buf + 4, sizeof(buf) - 4, " %-17.10e %- 10.2e %-9.2e", chisq, delta / lim, lambda);
    for (k = 0, p = buf + 4; (k < 3) && (p != NULL); k++) {
	p++;
	pack_float(p);
	p = strchr(p, 'e');
    }
    fputs(buf, device);
    len = strlen(buf);
    for (k = 0; k < num_params; k++) {
	snprintf(buf, sizeof(buf), " % 14.6e", parms[k] * scale_params[k]);
	pack_float(buf);
	len += strlen(buf);
	if ((fit_wrap > 0) && (len >= fit_wrap)) {
	    fprintf(device, "\n%*c", indent, ' ');
	    len = indent;
	}
	fputs(buf, device);
    }
    fputs("\n", device);
}


/*****************************************************************
    is_empty: check for valid string entries
*****************************************************************/
static TBOOLEAN
is_empty(char *s)
{
    while (*s == ' ' || *s == '\t' || *s == '\n')
	s++;
    return (TBOOLEAN) (*s == '#' || *s == '\0');
}


/*****************************************************************
    get next word of a multi-word string, advance pointer
*****************************************************************/
static char *
get_next_word(char **s, char *subst)
{
    char *tmp = *s;

    while (*tmp == ' ' || *tmp == '\t' || *tmp == '=')
	tmp++;
    if (*tmp == '\n' || *tmp == '\0')	/* not found */
	return NULL;
    if ((*s = strpbrk(tmp, " =\t\n")) == NULL)
	*s = tmp + strlen(tmp);
    *subst = **s;
    *(*s)++ = '\0';
    return tmp;
}


/*****************************************************************
    first time settings
*****************************************************************/
void
init_fit()
{
    func.at = (struct at_type *) NULL;	/* need to parse 1 time */
}


/*****************************************************************
    Set a GNUPLOT user-defined variable
******************************************************************/

static void
setvar(char *varname, double data)
{
    /* Despite its name it is actually usable for any variable. */
    fill_gpval_float(varname, data);
}


/*****************************************************************
    Set a GNUPLOT user-defined variable for an error
    variable: so take the parameter name, turn it
    into an error parameter name (e.g. a to a_err)
    and then set it.
******************************************************************/
static void
setvarerr(char *varname, double value)
{
	/* Create the variable name by appending _err */
	char * pErrValName = gp_alloc(strlen(varname) + 5 * sizeof(char), "setvarerr");
	sprintf(pErrValName, "%s_err", varname);
	setvar(pErrValName, value);
	free(pErrValName);
}


/*****************************************************************
    Get integer variable value
*****************************************************************/
static int
getivar(const char *varname)
{
    struct udvt_entry * v = get_udv_by_name((char *)varname);
    if ((v != NULL) && (!v->udv_undef))
	return real_int(&(v->udv_value));
    else
	return 0;
}


/*****************************************************************
    Get double variable value
*****************************************************************/
static double
getdvar(const char *varname)
{
    struct udvt_entry * v = get_udv_by_name((char *)varname);
    if ((v != NULL) && (!v->udv_undef))
	return real(&(v->udv_value));
    else
	return 0;
}


/*****************************************************************
   like getdvar, but
   - create it and set to `value` if not found or undefined
   - convert it from integer to real if necessary
*****************************************************************/
static double
createdvar(char *varname, double value)
{
    struct udvt_entry *udv_ptr = add_udv_by_name((char *)varname);
    if (udv_ptr->udv_undef) { /* new variable */
	udv_ptr->udv_undef = FALSE;
	Gcomplex(&udv_ptr->udv_value, value, 0.0);
    } else if (udv_ptr->udv_value.type == INTGR) { /* convert to CMPLX */
	Gcomplex(&udv_ptr->udv_value, (double) udv_ptr->udv_value.v.int_val, 0.0);
    }
    return real(&(udv_ptr->udv_value));
}


/* argument: char *fn */
#define VALID_FILENAME(fn) ((fn) != NULL && (*fn) != '\0')

/*****************************************************************
    write the actual parameters to start parameter file
*****************************************************************/
void
update(char *pfile, char *npfile)
{
    char ifilename[PATH_MAX];
    char *ofilename;
    TBOOLEAN createfile = FALSE;

    if (existfile(pfile)) {
	/* update pfile npfile:
	   if npfile is a valid file name, take pfile as input file and
	   npfile as output file
	*/
	if (VALID_FILENAME(npfile)) {
	    safe_strncpy(ifilename, pfile, sizeof(ifilename));
	    ofilename = npfile;
	} else {
#ifdef BACKUP_FILESYSTEM
	    /* filesystem will keep original as previous version */
	    safe_strncpy(ifilename, pfile, sizeof(ifilename));
#else
	    backup_file(ifilename, pfile);	/* will Eex if it fails */
#endif
	    ofilename = pfile;
	}
    } else {
	/* input file does not exists; will create new file */
	createfile = TRUE;
	if (VALID_FILENAME(npfile))
	    ofilename = npfile;
	else
	    ofilename = pfile;
    }

    if (createfile) {
	/* The input file does not exists and--strictly speaking--there is
	   nothing to 'update'.  Instead of bailing out we guess the intended use:
	   We output all INTGR/CMPLX user variables and mark them as '# FIXED' if
	   they were not used during the last fit command. */
	struct udvt_entry *udv = first_udv;
	FILE *nf;

	if ((last_fit_command == NULL) || (strlen(last_fit_command) == 0)) {
	    /* Technically, a prior fit command isn't really required.  But since
	    all variables in the parameter file would be marked '# FIXED' in that
	    case, it cannot be directly used in a subsequent fit command. */
#if 1
	    Eex2("'update' requires a prior 'fit' since the parameter file %s does not exist yet.", ofilename);
#else
	    fprintf(stderr, "'update' without a prior 'fit' and without a previous parameter file:\n");
	    fprintf(stderr, " all variables will be marked '# FIXED'!\n");
#endif
	}

	if (!(nf = fopen(ofilename, "w")))
	    Eex2("new parameter file %s could not be created", ofilename);

	fputs("# Parameter file created by 'update' from current variables\n", nf);
	if ((last_fit_command != NULL) && (strlen(last_fit_command) > 0))
	    fprintf(nf, "## %s\n", last_fit_command);

	while (udv) {
	    if ((strncmp(udv->udv_name, "GPVAL_", 6) == 0) ||
	        (strncmp(udv->udv_name, "MOUSE_", 6) == 0) ||
	        (strncmp(udv->udv_name, "FIT_", 4) == 0) ||
	        (strcmp(udv->udv_name, "NaN") == 0) ||
	        (strcmp(udv->udv_name, "pi") == 0)) {
		/* skip GPVAL_, MOUSE_, FIT_ and builtin variables */
		udv = udv->next_udv;
		continue;
	    }
	    if (!udv->udv_undef &&
	        ((udv->udv_value.type == INTGR) || (udv->udv_value.type == CMPLX))) {
		int k;

		/* ignore indep. variables */
		for (k = 0; k < 5; k++) {
		    if (strcmp(last_dummy_var[k], udv->udv_name) == 0)
			break;
		}
		if (k != 5) {
		    udv = udv->next_udv;
		    continue;
		}

		if (udv->udv_value.type == INTGR)
		    fprintf(nf, "%-15s = %-22i", udv->udv_name, udv->udv_value.v.int_val);
		else /* CMPLX */
		    fprintf(nf, "%-15s = %-22s", udv->udv_name, num_to_str(udv->udv_value.v.cmplx_val.real));
		/* mark variables not used for the last fit as fixed */
		for (k = 0; k < last_num_params; k++) {
		    if (strcmp(last_par_name[k], udv->udv_name) == 0)
			break;
		}
		if (k == last_num_params)
		    fprintf(nf, "   %s", GP_FIXED);
		putc('\n', nf);
	    }
	    udv = udv->next_udv;
	}

	if (fclose(nf))
	    Eex("I/O error during update");

    } else { /* !createfile */

	/* input file exists - this is the originally intended case of
	   the update command: update an existing parameter file */
	char sstr[256];
	char *s = sstr;
	char * fnam;
	FILE *of, *nf;

	if (!(of = loadpath_fopen(ifilename, "r")))
	    Eex2("parameter file %s could not be read", ifilename);

	if (!(nf = fopen(ofilename, "w")))
	    Eex2("new parameter file %s could not be created", ofilename);

	fnam = gp_basename(ifilename); /* strip off the path */
	if (fnam == NULL)
	    fnam = ifilename;

	while (fgets(s = sstr, sizeof(sstr), of) != NULL) {
	    char pname[64]; /* name of parameter */
	    double pval;    /* parameter value */
	    char tail[127]; /* trailing characters */
	    char * tmp;
	    char c;

	    if (is_empty(s)) {
		fputs(s, nf);	/* preserve comments */
		continue;
	    }
	    if ((tmp = strchr(s, '#')) != NULL) {
		safe_strncpy(tail, tmp, sizeof(tail));
		*tmp = NUL;
	    } else
		strcpy(tail, "\n");

	    tmp = get_next_word(&s, &c);
	    if (!legal_identifier(tmp) || strlen(tmp) > MAX_ID_LEN) {
		fclose(nf);
		fclose(of);
		Eex2("syntax error in parameter file %s", fnam);
	    }
	    safe_strncpy(pname, tmp, sizeof(pname));
	    /* next must be '=' */
	    if (c != '=') {
		tmp = strchr(s, '=');
		if (tmp == NULL) {
		    fclose(nf);
		    fclose(of);
		    Eex2("syntax error in parameter file %s", fnam);
		}
		s = tmp + 1;
	    }
	    tmp = get_next_word(&s, &c);
	    if (!sscanf(tmp, "%lf", &pval)) {
		fclose(nf);
		fclose(of);
		Eex2("syntax error in parameter file %s", fnam);
	    }
	    if ((tmp = get_next_word(&s, &c)) != NULL) {
		fclose(nf);
		fclose(of);
		Eex2("syntax error in parameter file %s", fnam);
	    }

	    /* now modify */
	    pval = getdvar(pname);
	    fprintf(nf, "%-15s = %-22s   %s", pname, num_to_str(pval), tail);
	}

	if (fclose(nf) || fclose(of))
	    Eex("I/O error during update");
    }
}


/*****************************************************************
    Backup a file by renaming it to something useful. Return
    the new name in tofile
*****************************************************************/

/* tofile must point to a char array[] or allocated data. See update() */

static void
backup_file(char *tofile, const char *fromfile)
{
#if defined(MSDOS) || defined(VMS)
    char *tmpn;
#endif

/* first attempt, for all o/s other than MSDOS */

#ifndef MSDOS
    strcpy(tofile, fromfile);
#ifdef VMS
    /* replace all dots with _, since we will be adding the only
     * dot allowed in VMS names
     */
    while ((tmpn = strchr(tofile, '.')) != NULL)
	*tmpn = '_';
#endif /*VMS */
    strcat(tofile, BACKUP_SUFFIX);
    if (rename(fromfile, tofile) == 0)
	return;			/* hurrah */
    if (existfile(tofile))
	Eex2("The backup file %s already exists and will not be overwritten.", tofile);
#endif

#ifdef MSDOS
    /* first attempt for msdos. */

    /* Copy only the first 8 characters of the filename, to comply
     * with the restrictions of FAT filesystems. */
    safe_strncpy(tofile, fromfile, 8 + 1);

    while ((tmpn = strchr(tofile, '.')) != NULL)
	*tmpn = '_';

    strcat(tofile, BACKUP_SUFFIX);

    if (rename(fromfile, tofile) == 0)
	return;			/* success */
#endif /* MSDOS */

    /* get here => rename failed. */
    Eex3("Could not rename file %s to %s", fromfile, tofile);
}


/* A modified copy of save.c:save_range(), but this one reports
 * _current_ values, not the 'set' ones, by default */
static void
log_axis_restriction(FILE *log_f, AXIS_INDEX axis, char *name)
{
    char s[80];
    AXIS *this_axis = axis_array + axis;

    fprintf(log_f, "\t%s range restricted to [", name);
    if (this_axis->autoscale & AUTOSCALE_MIN) {
	putc('*', log_f);
    } else if (this_axis->datatype == DT_TIMEDATE) {
	putc('"', log_f);
	gstrftime(s, 80, this_axis->timefmt, this_axis->min);
	fputs(s, log_f);
	putc('"', log_f);
    } else {
	fprintf(log_f, "%#g", this_axis->min);
    }

    fputs(" : ", log_f);
    if (this_axis->autoscale & AUTOSCALE_MAX) {
	putc('*', log_f);
    } else if (this_axis->datatype == DT_TIMEDATE) {
	putc('"', log_f);
	gstrftime(s, 80, this_axis->timefmt, this_axis->max);
	fputs(s, log_f);
	putc('"', log_f);
    } else {
	fprintf(log_f, "%#g", this_axis->max);
    }
    fputs("]\n", log_f);
}


/*****************************************************************
    Interface to the classic gnuplot-software
*****************************************************************/

void
fit_command()
{
/* HBB 20000430: revised this completely, to make it more similar to
 * what plot3drequest() does */

    int num_ranges=0;	 /* # range specs */
    static int var_order[]={FIRST_X_AXIS, FIRST_Y_AXIS, T_AXIS, U_AXIS, V_AXIS, FIRST_Z_AXIS};
    static char dummy_default[MAX_NUM_VAR][MAX_ID_LEN+1]={"x","y","t","u","v","z"};

    int max_data;
    int max_params;

    int dummy_token[7] = {-1,-1,-1,-1,-1,-1,-1};
    int num_points=0;	    /* number of data points read from file */
    int skipped[12];	    /* num points out of range */
    int zrange_token = -1;

    int i, j;
    double v[7];
    double tmpd;
    time_t timer;
    int token1, token2, token3;
    char *tmp, *file_name;
    AXIS saved_axis;

    c_token++;

    /* first look for a restricted x fit range... */

    /* put stuff into arrays to simplify access */
    AXIS_INIT3D(FIRST_X_AXIS, 0, 0);
    AXIS_INIT3D(FIRST_Y_AXIS, 0, 0);
    AXIS_INIT3D(T_AXIS, 0, 0);
    AXIS_INIT3D(U_AXIS, 0, 0);
    AXIS_INIT3D(V_AXIS, 0, 0);
    AXIS_INIT3D(FIRST_Z_AXIS, 0, 1);

    /* use global default indices in axis.c to simplify access to
     * per-axis variables */
    x_axis = FIRST_X_AXIS;
    y_axis = FIRST_Y_AXIS;
    z_axis = FIRST_Z_AXIS;

    /* JRV 20090201: Parsing the range specs would be much simpler if
     * we knew how many independent variables we had, but we won't
     * know that until parsing the using specs.  Assume we have one
     * independent variable to start with (x), and adjust later
     * if needed. */

    while (equals(c_token, "[")) {
      int i;
      if (num_ranges > 5)
	int_error(c_token, "only 6 range specs are permitted");
      i=var_order[num_ranges];
      /* Store the Z axis dummy variable and range (if any) in the axis
       * for the next independent variable.  Save the current values to be
       * restored later if there are fewer than 5 independent variables.
       */
      saved_axis = axis_array[i];
      dummy_token[6] = dummy_token[i];

      dummy_token[num_ranges] = parse_range(i);
      zrange_token = c_token;
      num_ranges++;
    }

    /* now compile the function */

    token1 = c_token;

    if (func.at) {
	free_at(func.at);
	func.at = NULL;		/* in case perm_at() does int_error */
    }
    dummy_func = &func;

    /* set all five dummy variable names, even if we're using fewer */
    strcpy(dummy_default[0], set_dummy_var[0]);
    strcpy(dummy_default[1], set_dummy_var[1]);
    for (i = 0; i < 5; i++) {
	if (dummy_token[i] > 0)
	    copy_str(c_dummy_var[i], dummy_token[i], MAX_ID_LEN);
	else
	    strcpy(c_dummy_var[i], dummy_default[i]);
    }

    func.at = perm_at();	/* parse expression and save action table */
    dummy_func = NULL;

    token2 = c_token;

    /* get filename */
    file_name = string_or_express(NULL);
    if (file_name )
	file_name = gp_strdup(file_name);
    else
	int_error(token2, "missing filename or datablock");

    /* use datafile module to parse the datafile and qualifiers */
    df_set_plot_mode(MODE_QUERY);  /* Does nothing except for binary datafiles */
    columns = df_open(file_name, 7, NULL);	/* up to 7 using specs allowed */
    free(file_name);
    if (columns < 0)
	int_error(NO_CARET,"Can't read data file");
    if (columns == 1)
	int_error(c_token, "Need 2 to 7 using specs");

    num_indep = (columns < 3) ? 1 : columns - 2;

    /* The following patch was made by Remko Scharroo, 25-Mar-1999
     * We need to check if one of the columns is time data, like
     * in plot2d and plot3d */

    if (X_AXIS.datatype == DT_TIMEDATE) {
	if (columns < 2)
	    int_error(c_token, "Need full using spec for x time data");
    }
    if (Y_AXIS.datatype == DT_TIMEDATE) {
	if (columns < 1)
	    int_error(c_token, "Need using spec for y time data");
    }
    /* No need for a time data check for the remaining axes.  Those
     * axes are only used iff columns>3, i.e. there are using specs
     * for all the columns, already. */

    for (i=0; i<num_indep; i++)
      df_axis[i] = var_order[i];
    df_axis[i++] = FIRST_Z_AXIS;
    /* don't parse sigma_z as times */
    df_axis[i] = NO_AXIS;


    /* HBB 980401: if this is a single-variable fit, we shouldn't have
     * allowed a variable name specifier for 'y': */
    if ((dummy_token[1] > 0) && (num_indep==1))
	int_error(dummy_token[1], "Can't re-name 'y' in a one-variable fit");

    /* depending on number of independent variables, the last range
     * spec may be for the Z axis */
    if (num_ranges > num_indep+1)
      int_error(zrange_token, "Too many range-specs for a %d-variable fit", num_indep);
    else if (num_ranges == num_indep+1 && num_indep < 5) {
      /* last range spec is for the Z axis */
      int i = var_order[num_ranges-1]; /* index for the last range spec */

      Z_AXIS.autoscale = axis_array[i].autoscale;
      if (!(axis_array[i].autoscale & AUTOSCALE_MIN))
	Z_AXIS.min = axis_array[i].min;
      if (!(axis_array[i].autoscale & AUTOSCALE_MAX))
	Z_AXIS.max = axis_array[i].max;

      /* restore former values */
      axis_array[i] = saved_axis;
      dummy_token[num_ranges-1] = dummy_token[6];
    }

    /* defer actually reading the data until we have parsed the rest
     * of the line */
    token3 = c_token;

    tmpd = getdvar(FITLIMIT);	/* get epsilon if given explicitly */
    if (tmpd < 1.0 && tmpd > 0.0)
	epsilon = tmpd;
    else
	epsilon = DEF_FIT_LIMIT;
    FPRINTF((STANDARD, "epsilon=%e\n", epsilon));

    /* tsm patchset 230: new absolute convergence variable */
    FPRINTF((STANDARD, "epsilon_abs=%e\n", epsilon_abs));

    /* HBB 970304: maxiter patch */
    maxiter = getivar(FITMAXITER);
    if (maxiter < 0)
	maxiter = 0;
    FPRINTF((STANDARD, "maxiter=%i\n", maxiter));

    /* get startup value for lambda, if given */
    tmpd = getdvar(FITSTARTLAMBDA);
    if (tmpd > 0.0) {
	startup_lambda = tmpd;
	Dblf2("lambda start value set: %g\n", startup_lambda);
    } else {
	startup_lambda = 0.0;
    }

    /* get lambda up/down factor, if given */
    tmpd = getdvar(FITLAMBDAFACTOR);
    if (tmpd > 0.0) {
	lambda_up_factor = lambda_down_factor = tmpd;
	Dblf2("lambda scaling factors reset:  %g\n", lambda_up_factor);
    } else {
	lambda_down_factor = LAMBDA_DOWN_FACTOR;
	lambda_up_factor = LAMBDA_UP_FACTOR;
    }

    FPRINTF((STANDARD, "prescale=%i\n", fit_prescale));
    FPRINTF((STANDARD, "errorscaling=%i\n", fit_errorscaling));

    {
	char *logfile = getfitlogfile();
	if ((logfile != NULL) && !log_f && !(log_f = fopen(logfile, "a")))
	    Eex2("could not open log-file %s", logfile);
	FPRINTF((STANDARD, "log-file=%s\n", logfile));
	free(logfile);
    }

    fputs("\n\n*******************************************************************************\n", log_f);
    (void) time(&timer);
    fprintf(log_f, "%s\n\n", ctime(&timer));
    {
	char *line = NULL;

	m_capture(&line, token2, token3 - 1);
	fprintf(log_f, "FIT:    data read from %s\n", line);
	fprintf(log_f, "        format = ");
	free(line);
	for (i = 0; (i < num_indep) && (i < columns - 1); i++)
	    fprintf(log_f, "%s:", c_dummy_var[i]);
	fprintf(log_f, (columns <= 2) ? "z\n" : "z:s\n");
    }

    /* report all range specs */
    j = FIRST_Z_AXIS;		/* check Z axis first */
    for (i = 0; i <= num_indep; i++) {
	if ((axis_array[j].autoscale & AUTOSCALE_BOTH) != AUTOSCALE_BOTH)
	    log_axis_restriction(log_f, j, i ? c_dummy_var[i] : "z");
	j = var_order[i];
    }

    max_data = MAX_DATA;
    fit_x = vec(max_data * num_indep); /* start with max. value */
    fit_z = vec(max_data);
    err_data = vec(max_data);
    num_data = 0;

    for (i = 0; i < sizeof(skipped) / sizeof(int); i++)
	skipped[i] = 0;

    /* first read in experimental data */

    /* If the user has set an explicit locale for numeric input, apply it */
    /* here so that it affects data fields read from the input file.      */
    set_numeric_locale();

    while ((i = df_readline(v, 7)) != DF_EOF) {
        if (num_data >= max_data) {
	    /* increase max_data by factor of 1.5 */
	    max_data = (max_data * 3) / 2;
	    if (0
		|| !redim_vec(&fit_x, max_data*num_indep)
		|| !redim_vec(&fit_z, max_data)
		|| !redim_vec(&err_data, max_data)
		) {
		/* Some of the reallocations went bad: */
		df_close();
		Eex2("Out of memory in fit: too many datapoints (%d)?", max_data);
	    } else {
		/* Just so we know that the routine is at work: */
		fprintf(STANDARD, "Max. number of data points scaled up to: %d\n",
			max_data);
	    }
	} /* if (need to extend storage space) */

	/* BM: silently ignore lines with NaN */
	{
	    TBOOLEAN skip_nan = FALSE;
	    int k;
	    for (k = 0; k < i; k++) {
		if (isnan(v[k]))
		    skip_nan = TRUE;
	    }
	    if (skip_nan)
		continue;
	}

	switch (i) {
	case DF_MISSING:
	case DF_UNDEFINED:
	case DF_FIRST_BLANK:
	case DF_SECOND_BLANK:
	    continue;
	case 0:
	    Eex2("bad data on line %d of datafile", df_line_number);
	    break;
	case 1:		/* only z provided */
	    v[1] = v[0];
	    v[0] = (double) df_datum;
	    break;
	case 2:		/* x, z */
	case 3:		/* x, z, error */
	case 4:		/* x, y, z, error */
	case 5:		/* x, y, t, z, error */
	case 6:		/* x, y, t, u, z, error */
	case 7:		/* x, y, t, u, v, z, error */
	    break;
	}
	num_points++;

	/* skip this point if it is out of range */
	for (i=0; i<num_indep; i++){
	  int j = var_order[i];
	  AXIS *this = axis_array + j;

	  if (!(this->autoscale & AUTOSCALE_MIN) && (v[i] < this->min)) {
	    skipped[j]++;
	    goto out_of_range;
	  }
	  if (!(this->autoscale & AUTOSCALE_MAX) && (v[i] > this->max)) {
	    skipped[j]++;
	    goto out_of_range;
	  }
	  fit_x[num_data*num_indep+i] = v[i]; /* save independent variable data */
	}
	/* check Z value too */
	{
	  AXIS *this = axis_array + FIRST_Z_AXIS;

	  if (!(this->autoscale & AUTOSCALE_MIN) && (v[i] < this->min)) {
	    skipped[FIRST_Z_AXIS]++;
	    goto out_of_range;
	  }
	  if (!(this->autoscale & AUTOSCALE_MAX) && (v[i] > this->max)) {
	    skipped[FIRST_Z_AXIS]++;
	    goto out_of_range;
	  }
	  fit_z[num_data] = v[i++];	      /* save dependent variable data */
	}

	/* only use error from data file if _explicitly_ asked for by
	 * a using spec
	 */
	err_data[num_data++] = (columns > 2) ? v[i] : 1;

    out_of_range:
	;
    }
    df_close();

    /* We are finished reading user input; return to C locale for internal use */
    reset_numeric_locale();

    if (num_data <= 1) {
	/* no data! Try to explain why. */
	printf("         Read %d points\n", num_points);
	for (i = 0; i < 6; i++) {
	    int j = var_order[i];
	    AXIS *this = axis_array + j;
	    if (skipped[j]){
		printf("         Skipped %d points outside range [%s=",
		skipped[j], i < 5 ? c_dummy_var[i] : "z");
		if (this->autoscale&AUTOSCALE_MIN)
		    printf("*:");
		else
		    printf("%g:", this->min);
		if (this->autoscale&AUTOSCALE_MAX)
		    printf("*]\n");
		else
		    printf("%g]\n", this->max);
	    }
	}
	Eex("No data to fit ");
    }

    /* tsm patchset 230: check for zero error values */
    for (i = 0; i < num_data; i++) {
	if (err_data[i] != 0.0)
	    continue;
	Dblf("\nCurrent data point\n");
	Dblf("=========================\n");
	Dblf3("%-15s = %i out of %i\n", "#", i + 1, num_data);
	for (j = 0; j < num_indep; j++)
	    Dblf3("%-15.15s = %-15g\n", c_dummy_var[j], fit_x[i * num_indep + j]);
	Dblf3("%-15.15s = %-15g\n", "z", fit_z[i]);
	Dblf3("%-15.15s = %-15g\n", "s", err_data[i]);
	Dblf("\n");
	Eex("Zero error value in data file");
	/* FIXME: Eex() aborts before memory is freed! */
    }

    /* now resize fields to actual length: */
    redim_vec(&fit_x, num_data*num_indep);
    redim_vec(&fit_z, num_data);
    redim_vec(&err_data, num_data);

    fprintf(log_f, "        #datapoints = %d\n", num_data);

    if (columns < 3)
	fputs("        residuals are weighted equally (unit weight)\n\n", log_f);

    {
	char *line = NULL;

	m_capture(&line, token1, token2 - 1);
	fprintf(log_f, "function used for fitting: %s\n", line);
	free(line);
    }

    /* read in parameters */

    max_params = MAX_PARAMS;	/* HBB 971023: make this resizeable */

    if (!equals(c_token++, "via"))
	int_error(c_token, "Need via and either parameter list or file");

    /* allocate arrays for parameter values, names */
    a = vec(max_params);
    par_name = (fixstr *) gp_alloc((max_params + 1) * sizeof(fixstr),
				   "fit param");
    num_params = 0;

    if (isstringvalue(c_token)) {	/* It's a parameter *file* */
	TBOOLEAN fixed;
	double tmp_par;
	char c, *s;
	char sstr[MAX_LINE_LEN + 1];
	FILE *f;

	static char *viafile = NULL;
	free(viafile);			/* Free previous name, if any */
	viafile = try_to_get_string();	/* Cannot fail since isstringvalue succeeded */
	fprintf(log_f, "fitted parameters and initial values from file: %s\n\n", viafile);
	if (!(f = loadpath_fopen(viafile, "r")))
	    Eex2("could not read parameter-file \"%s\"", viafile);

	/* get parameters and values out of file and ignore fixed ones */

	while (TRUE) {
	    if (!fgets(s = sstr, sizeof(sstr), f))	/* EOF found */
		break;
	    if ((tmp = strstr(s, GP_FIXED)) != NULL) {	/* ignore fixed params */
		*tmp = NUL;
		fprintf(log_f, "FIXED:  %s\n", s);
		fixed = TRUE;
	    } else
		fixed = FALSE;
	    if ((tmp = strchr(s, '#')) != NULL)
		*tmp = NUL;
	    if (is_empty(s))
		continue;
	    tmp = get_next_word(&s, &c);
	    if (!legal_identifier(tmp) || strlen(tmp) > MAX_ID_LEN) {
		(void) fclose(f);
		Eex("syntax error in parameter file");
	    }
	    safe_strncpy(par_name[num_params], tmp, sizeof(fixstr));
	    /* next must be '=' */
	    if (c != '=') {
		tmp = strchr(s, '=');
		if (tmp == NULL) {
		    (void) fclose(f);
		    Eex("syntax error in parameter file");
		}
		s = tmp + 1;
	    }
	    tmp = get_next_word(&s, &c);
	    if (sscanf(tmp, "%lf", &tmp_par) != 1) {
		(void) fclose(f);
		Eex("syntax error in parameter file");
	    }
	    /* make fixed params visible to GNUPLOT */
	    if (fixed) {
		/* use parname as temp */
		setvar(par_name[num_params], tmp_par);
	    } else {
		if (num_params >= max_params) {
		    max_params = (max_params * 3) / 2;
		    if (0
			|| !redim_vec(&a, max_params)
			|| !(par_name = gp_realloc(par_name, (max_params + 1) * sizeof(fixstr), "fit param resize"))
			) {
			(void) fclose(f);
			Eex("Out of memory in fit: too many parameters?");
		    }
		}
		a[num_params++] = tmp_par;
	    }

	    if ((tmp = get_next_word(&s, &c)) != NULL) {
		(void) fclose(f);
		Eex("syntax error in parameter file");
	    }
	}
	(void) fclose(f);

    } else {
	/* not a string after via: it's a variable listing */

	fputs("fitted parameters initialized with current variable values\n\n", log_f);
	do {
	    if (!isletter(c_token))
		Eex("no parameter specified");
	    capture(par_name[num_params], c_token, c_token, (int) sizeof(par_name[0]));
	    if (num_params >= max_params) {
		max_params = (max_params * 3) / 2;
		if (0
		    || !redim_vec(&a, max_params)
		    || !(par_name = gp_realloc(par_name, (max_params + 1) * sizeof(fixstr), "fit param resize"))
		    ) {
		    Eex("Out of memory in fit: too many parameters?");
		}
	    }
	    /* create variable if it doesn't exist */
	    a[num_params] = createdvar(par_name[num_params], INITIAL_VALUE);
	    ++num_params;
	} while (equals(++c_token, ",") && ++c_token);
    }

    redim_vec(&a, num_params);
    par_name = (fixstr *) gp_realloc(par_name, (num_params + 1) * sizeof(fixstr), "fit param");

    if (num_data < num_params)
	Eex("Number of data points smaller than number of parameters");

    /* initialize scaling parameters */
    if (!redim_vec(&scale_params,num_params)){
	df_close();
	Eex2("Out of memory in fit: too many datapoints (%d)?", max_data);
    }

    for (i = 0; i < num_params; i++) {
	/* avoid parameters being equal to zero */
	if (a[i] == 0) {
	    a[i] = NEARLY_ZERO;
	    scale_params[i] = 1.0;
	} else if (fit_prescale) {
	    /* scale parameters, but preserve sign */
	    double a_sign = (a[i] > 0) - (a[i] < 0);
	    scale_params[i] = a_sign * a[i];
	    a[i] = a_sign;
	} else {
	    scale_params[i] = 1.0;
	}
    }

    if (num_params == 0)
	int_warn(NO_CARET, "No fittable parameters!\n");
    else
	(void) regress(a);	/* fit */

    (void) fclose(log_f);

    log_f = NULL;
    free(fit_x);
    free(fit_z);
    free(err_data);
    free(a);
    if (func.at) {
	free_at(func.at);		/* release perm. action table */
	func.at = (struct at_type *) NULL;
    }
    /* remember parameter names for 'update' */
    last_num_params = num_params;
    free(last_par_name);
    last_par_name = par_name;
    /* remember names of indep. variables for 'update' */
    memcpy(last_dummy_var, c_dummy_var, sizeof(last_dummy_var));
    /* remember last fit command for 'save' */
    safe_strncpy(last_fit_command, gp_input_line, sizeof(last_fit_command));
    /* save fit command to user variable */
    fill_gpval_string("GPVAL_LAST_FIT", last_fit_command);
}

/*
 * Print message to stderr and log file
 */
#if defined(VA_START) && defined(STDC_HEADERS)
static void
Dblfn(const char *fmt, ...)
#else
static void
Dblfn(const char *fmt, va_dcl)
#endif
{
#ifdef VA_START
    va_list args;

    VA_START(args, fmt);
# if defined(HAVE_VFPRINTF) || _LIBC
    if (fit_verbosity != QUIET)
	vfprintf(STANDARD, fmt, args);
    va_end(args);
    VA_START(args, fmt);
    vfprintf(log_f, fmt, args);
# else
    if (fit_verbosity != QUIET)
	_doprnt(fmt, args, STANDARD);
    _doprnt(fmt, args, log_f);
# endif
    va_end(args);
#else
    if (fit_verbosity != QUIET)
	fprintf(STANDARD, fmt, a1, a2, a3, a4, a5, a6, a7, a8);
    fprintf(log_f, fmt, a1, a2, a3, a4, a5, a6, a7, a8);
#endif /* VA_START */
}


/*****************************************************************
    Get name of current log-file
*****************************************************************/
char *
getfitlogfile()
{
    char *logfile = NULL;

    if (fitlogfile == NULL) {
	char *tmp = getenv(GNUFITLOG);	/* open logfile */

	if (tmp != NULL && *tmp != '\0') {
	    char *tmp2 = tmp + (strlen(tmp) - 1);

	    /* if given log file name ends in path separator, treat it
	     * as a directory to store the default "fit.log" in */
	    if (*tmp2 == '/' || *tmp2 == '\\') {
		logfile = gp_alloc(strlen(tmp)
				   + strlen(fitlogfile_default) + 1,
				   "logfile");
		strcpy(logfile, tmp);
		strcat(logfile, fitlogfile_default);
	    } else {
		logfile = gp_strdup(tmp);
	    }
	} else {
	    logfile = gp_strdup(fitlogfile_default);
	}
    } else {
	logfile = gp_strdup(fitlogfile);
    }
    return logfile;
}
