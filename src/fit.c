#ifndef lint
static char *RCSid() { return RCSid("$Id: fit.c,v 1.72 2010/07/30 19:11:40 sfeam Exp $"); }
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
 *      930726:     Recoding of the Unix-like raw console I/O routines by:
 *                  Michele Marziani (marziani@ferrara.infn.it)
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
 * Jim Van Zandt, 090201: allow fitting functions with up to five
 * independent variables.
 */

#include "fit.h"

#include <signal.h>

#include "alloc.h"
#include "axis.h"
#include "command.h"
#include "datafile.h"
#include "eval.h"
#include "gp_time.h"
#include "matrix.h"
#include "plot.h"
#include "misc.h"
#include "util.h"
#include "variable.h" /* For locale handling */

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
# ifndef _Windows		/* WIN16 does define MSDOS .... */
#  include <conio.h>
# endif
# include <dos.h>
#else /* !(MSDOS) */
# ifndef VMS
#  include <fcntl.h>
# endif				/* !VMS */
#endif /* !(MSDOS) */

enum marq_res {
    OK, ML_ERROR, BETTER, WORSE
};
typedef enum marq_res marq_res_t;

#ifdef INFINITY
# undef INFINITY
#endif

#define INFINITY    1e30
#define NEARLY_ZERO 1e-30

/* create new variables with this value (was NEARLY_ZERO) */
#define INITIAL_VALUE 1.0

/* Relative change for derivatives */
#define DELTA	    0.001

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

/* externally visible variables: */

/* saved copy of last 'fit' command -- for output by "save" */
char fitbuf[256];

/* log-file for fit command */
char *fitlogfile = NULL;
TBOOLEAN fit_errorvariables = FALSE;
TBOOLEAN fit_quiet = FALSE;

/* private variables: */

static int max_data;
static int max_params;

static double epsilon = 1e-5;	/* convergence limit */
static int maxiter = 0;

static char *fit_script = NULL;

/* HBB/H.Harders 20020927: log file name now changeable from inside
 * gnuplot */
static const char fitlogfile_default[] = "fit.log";
static const char GNUFITLOG[] = "FIT_LOG";

static const char *GP_FIXED = "# FIXED";
static const char *FITLIMIT = "FIT_LIMIT";
static const char *FITSTARTLAMBDA = "FIT_START_LAMBDA";
static const char *FITLAMBDAFACTOR = "FIT_LAMBDA_FACTOR";
static const char *FITMAXITER = "FIT_MAXITER";	/* HBB 970304: maxiter patch */
static const char *FITSCRIPT = "FIT_SCRIPT";
static const char *DEFAULT_CMD = "replot";	/* if no fitscript spec. */
static char last_fit_command[LASTFITCMDLENGTH+1] = "";

static FILE *log_f = NULL;

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
static TBOOLEAN ctrlc_flag = FALSE;
static TBOOLEAN user_stop = FALSE;

static struct udft_entry func;

typedef char fixstr[MAX_ID_LEN+1];

static fixstr *par_name;

static double startup_lambda = 0, lambda_down_factor = LAMBDA_DOWN_FACTOR, lambda_up_factor = LAMBDA_UP_FACTOR;

/*****************************************************************
			 internal Prototypes
*****************************************************************/

static RETSIGTYPE ctrlc_handle __PROTO((int an_int));
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
static void log_axis_restriction __PROTO((FILE *log_f, AXIS_INDEX axis, char *name));
static TBOOLEAN is_empty __PROTO((char *s));
static TBOOLEAN is_variable __PROTO((char *s));
static double getdvar __PROTO((const char *varname));
static int getivar __PROTO((const char *varname));
static void setvar __PROTO((char *varname, struct value data));
static char *get_next_word __PROTO((char **s, char *subst));
static double createdvar __PROTO((char *varname, double value));
static void splitpath __PROTO((char *s, char *p, char *f));
static void backup_file __PROTO((char *, const char *));
static void setvarerr __PROTO((char *varname, double value));

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

static RETSIGTYPE
ctrlc_handle(int an_int)
{
    (void) an_int;		/* avoid -Wunused warning */
    /* reinstall signal handler (necessary on SysV) */
    (void) signal(SIGINT, (sigfunc) ctrlc_handle);
    ctrlc_flag = TRUE;
}


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
#if (defined(__EMX__) || !defined(MSDOS))
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
    strcpy(sp + 1, "\n\n");	/* terminate with exactly 2 newlines */
    fputs(fitbuf, STANDARD);
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

    bail_to_command_line();
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
    if (tmp_chisq < *chisq) {	/* Success, accept new solution */
	if (*lambda > MIN_LAMBDA) {
	    if (!fit_quiet)
	        (void) putc('/', stderr);
	    *lambda /= lambda_down_factor;
	}
	*chisq = tmp_chisq;
	for (j = 0; j < num_data; j++) {
	    memcpy(C[j], tmp_C[j], num_params * sizeof(double));
	    d[j] = tmp_d[j];
	}
	for (j = 0; j < num_params; j++)
	    a[j] = temp_a[j];
	return BETTER;
    } else {			/* failure, increase lambda and return */
        if (!fit_quiet)
	    (void) putc('*', stderr);
	*lambda *= lambda_up_factor;
	return WORSE;
    }
}


/*****************************************************************
    compute chi-square and numeric derivations
*****************************************************************/
/* used by marquardt to evaluate the linearized fitting matrix C and
 * vector d, fills in only the top part of C and d I don't use a
 * temporary array zfunc[] any more. Just use d[] instead.  */
/* FIXME: in the new code, this function doesn't really do enough to
 * be useful. Maybe it ought to be deleted, i.e. integrated with
 * calculate() ? */
static TBOOLEAN
analyze(double a[], double **C, double d[], double *chisq)
{
    int i, j;

    *chisq = 0;
    calculate(d, C, a);

    for (i = 0; i < num_data; i++) {
	/* note: order reversed, as used by Schwarz */
	d[i] = (d[i] - fit_z[i]) / err_data[i];
	*chisq += d[i] * d[i];
	for (j = 0; j < num_params; j++)
	    C[i][j] /= err_data[i];
    }
    /* FIXME: why return a value that is always TRUE ? */
    return TRUE;
}


/* To use the more exact, but slower two-side formula, activate the
   following line: */
/*#define TWO_SIDE_DIFFERENTIATION */
/*****************************************************************
    compute function values and partial derivatives of chi-square
*****************************************************************/
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
    for (i = 0; i < num_params; i++) {
	(void) Gcomplex(&v, par[i], 0.0);
	setvar(par_name[i], v);
    }

    for (i = 0; i < num_data; i++) {
      /* calculate fit-function value */
      /* initialize extra dummy variables from the corresponding
	 actual variables, if any. */
      for (j=0; j<5; j++) {
	struct udvt_entry *udv = add_udv_by_name(c_dummy_var[j]);
	(void) Gcomplex(&func.dummy_values[j],
			udv->udv_undef ? 0 : getdvar(c_dummy_var[j]),
			0.0);
      }
      /* set actual dummy variables from file data */
      for (j=0; j<num_indep; j++)
	(void) Gcomplex(&func.dummy_values[j],
			fit_x[i*num_indep+j], 0.0);
      evaluate_at(func.at, &v);
      if (undefined)
	Eex("Undefined value during function evaluation");
      data[i] = real(&v);
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
	switch (getc(stdin)) {

	case EOF:
	case 's':
	case 'S':
	    fputs("Stop.", STANDARD);
	    user_stop = TRUE;
	    return FALSE;

	case 'c':
	case 'C':
	    fputs("Continue.", STANDARD);
	    return TRUE;

	case 'e':
	case 'E':{
		int i;
		struct value v;
		const char *tmp;

		tmp = fit_script ? fit_script : DEFAULT_CMD;
		fprintf(STANDARD, "executing: %s", tmp);
		/* set parameters visible to gnuplot */
		for (i = 0; i < num_params; i++) {
		    (void) Gcomplex(&v, a[i], 0.0);
		    setvar(par_name[i], v);
		}
		do_string(tmp);
	    }
	}
    }
    return TRUE;
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

    if (!fit_quiet)
        show_fit(iter, chisq, chisq, a, lambda, STANDARD);
    show_fit(iter, chisq, chisq, a, lambda, log_f);

    /* Reset flag describing fit result status */
	v = add_udv_by_name("FIT_CONVERGED");
	v->udv_undef = FALSE;
	Ginteger(&v->udv_value, 0);

    /* MAIN FIT LOOP: do the regression iteration */

    /* HBB 981118: initialize new variable 'user_break' */
    user_stop = FALSE;

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


	if (ctrlc_flag) {
	    show_fit(iter, chisq, last_chisq, a, lambda, STANDARD);
	    ctrlc_flag = FALSE;
	    if (!fit_interrupt())	/* handle keys */
		break;
	}
	if (res == BETTER) {
	    iter++;
	    last_chisq = chisq;
	}
	if ((res = marquardt(a, C, &chisq, &lambda)) == BETTER)
	    if (!fit_quiet)
	        show_fit(iter, chisq, last_chisq, a, lambda, STANDARD);
    } while ((res != ML_ERROR)
	     && (lambda < MAX_LAMBDA)
	     && ((maxiter == 0) || (iter <= maxiter))
	     && (res == WORSE
		 || ((chisq > NEARLY_ZERO)
		     ? ((last_chisq - chisq) / chisq)
		     : (last_chisq - chisq)) > epsilon
	     )
	);

    /* fit done */

    /* restore original SIGINT function */
    interrupt_setup();

    /* HBB 970304: the maxiter patch: */
    if ((maxiter > 0) && (iter > maxiter)) {
	Dblf2("\nMaximum iteration count (%d) reached. Fit stopped.\n", maxiter);
    } else if (user_stop) {
	Dblf2("\nThe fit was stopped by the user after %d iterations.\n", iter);
    } else {
	Dblf2("\nAfter %d iterations the fit converged.\n", iter);
	v = add_udv_by_name("FIT_CONVERGED");
	v->udv_undef = FALSE;
	Ginteger(&v->udv_value, 1);
    }

    Dblf2("final sum of squares of residuals : %g\n", chisq);
    if (chisq > NEARLY_ZERO) {
	Dblf2("rel. change during last iteration : %g\n\n", (chisq - last_chisq) / chisq);
    } else {
	Dblf2("abs. change during last iteration : %g\n\n", (chisq - last_chisq));
    }

    if (res == ML_ERROR)
	Eex("FIT: error occurred during fit");

    /* compute errors in the parameters */

    if (fit_errorvariables)
	/* Set error variable to zero before doing this */
	/* Thus making sure they are created */
	for (i = 0; i < num_params; i++)
	    setvarerr(par_name[i], 0.0);

    if (num_data == num_params) {
	int k;

	Dblf("\nExactly as many data points as there are parameters.\n");
	Dblf("In this degenerate case, all errors are zero by definition.\n\n");
	Dblf("Final set of parameters \n");
	Dblf("======================= \n\n");
	for (k = 0; k < num_params; k++)
	    Dblf3("%-15.15s = %-15g\n", par_name[k], a[k]);
    } else if (chisq < NEARLY_ZERO) {
	int k;

	Dblf("\nHmmmm.... Sum of squared residuals is zero. Can't compute errors.\n\n");
	Dblf("Final set of parameters \n");
	Dblf("======================= \n\n");
	for (k = 0; k < num_params; k++)
	    Dblf3("%-15.15s = %-15g\n", par_name[k], a[k]);
    } else {
	int ndf          = num_data - num_params; 
	double stdfit    = sqrt(chisq/ndf);
	
	Dblf2("degrees of freedom    (FIT_NDF)                        : %d\n", ndf);
	Dblf2("rms of residuals      (FIT_STDFIT) = sqrt(WSSR/ndf)    : %g\n", stdfit);
	Dblf2("variance of residuals (reduced chisquare) = WSSR/ndf   : %g\n\n", chisq/ndf);

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

	/* scale parameter errors based on chisq */
	chisq = sqrt(chisq / (num_data - num_params));
	for (i = 0; i < num_params; i++)
	    dpar[i] *= chisq;

	Dblf("Final set of parameters            Asymptotic Standard Error\n");
	Dblf("=======================            ==========================\n\n");

	for (i = 0; i < num_params; i++) {
	    double temp = (fabs(a[i]) < NEARLY_ZERO)
		? 0.0
		: fabs(100.0 * dpar[i] / a[i]);

	    Dblf6("%-15.15s = %-15g  %-3.3s %-12.4g (%.4g%%)\n",
		  par_name[i], a[i], PLUSMINUS, dpar[i], temp);
	    if (fit_errorvariables)
		setvarerr(par_name[i], dpar[i]);
	}

	Dblf("\n\ncorrelation matrix of the fit parameters:\n\n");
	Dblf("               ");

	for (j = 0; j < num_params; j++)
	    Dblf2("%-6.6s ", par_name[j]);

	Dblf("\n");
	for (i = 0; i < num_params; i++) {
	    Dblf2("%-15.15s", par_name[i]);
	    for (j = 0; j <= i; j++) {
		/* Only print lower triangle of symmetric matrix */
		Dblf2("%6.3f ", covar[i][j]);
	    }
	    Dblf("\n");
	}

	free(dpar);
    }

    /* HBB 990220: re-imported this snippet from older versions. Finally,
     * some user noticed that it *is* necessary, after all. Not even
     * Carsten Grammes himself remembered what it was for... :-(
     * The thing is: the value of the last parameter is not reset to
     * its original one after the derivatives have been calculated
     */
    /* restore last parameter's value (not done by calculate) */
    {
	struct value val;
	Gcomplex(&val, a[num_params - 1], 0.0);
	setvar(par_name[num_params - 1], val);
    }

    /* call destructor for allocated vars */
    lambda = -2;		/* flag value, meaning 'destruct!' */
    (void) marquardt(a, C, &chisq, &lambda);

    free_matr(C);
    return TRUE;
}


/*****************************************************************
    display actual state of the fit
*****************************************************************/
static void
show_fit(
    int i,
    double chisq, double last_chisq,
    double *a, double lambda,
    FILE *device)
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
	fprintf(device, "%-15.15s = %g\n", par_name[k], a[k]);
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
    check for variable identifiers
*****************************************************************/
static TBOOLEAN
is_variable(char *s)
{
    while (*s != '\0') {
	if (!isalnum((unsigned char) *s) && *s != '_')
	    return FALSE;
	s++;
    }
    return TRUE;
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
setvar(char *varname, struct value data)
{
    struct udvt_entry *udv_ptr = add_udv_by_name(varname);
    udv_ptr->udv_value = data;
    udv_ptr->udv_undef = FALSE;
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
	struct value errval;    /* This will hold the gnuplot value created from value*/
	char* pErrValName;      /* The name of the (new) error variable */

	/* Create the variable name by appending _err */
	pErrValName = gp_alloc(strlen(varname)+sizeof(char)+4, "");

	sprintf(pErrValName,"%s_err",varname);
	Gcomplex(&errval, value, 0.0);
	setvar(pErrValName, errval);
	free(pErrValName);
}

/*****************************************************************
    Read INTGR Variable value, return 0 if undefined or wrong type
*****************************************************************/
static int
getivar(const char *varname)
{
    struct udvt_entry *udv_ptr = first_udv;

    while (udv_ptr) {
	if (!strcmp(varname, udv_ptr->udv_name))
	    return udv_ptr->udv_value.type == INTGR
		? udv_ptr->udv_value.v.int_val	/* valid */
		: 0;		/* wrong type */
	udv_ptr = udv_ptr->next_udv;
    }
    return 0;			/* not in table */
}


/*****************************************************************
    Read DOUBLE Variable value, return 0 if undefined or wrong type
   I don't think it's a problem that it's an integer - div
*****************************************************************/
static double
getdvar(const char *varname)
{
    struct udvt_entry *udv_ptr = first_udv;

    for (; udv_ptr; udv_ptr = udv_ptr->next_udv)
	if (strcmp(varname, udv_ptr->udv_name) == 0)
	    return real(&(udv_ptr->udv_value));

    /* get here => not found */
    return 0;
}

/*****************************************************************
   like getdvar, but
   - convert it from integer to real if necessary
   - create it with value INITIAL_VALUE if not found or undefined
*****************************************************************/
static double
createdvar(char *varname, double value)
{
    struct udvt_entry *udv_ptr = first_udv;

    for (; udv_ptr; udv_ptr = udv_ptr->next_udv)
	if (strcmp(varname, udv_ptr->udv_name) == 0) {
	    if (udv_ptr->udv_undef) {
		udv_ptr->udv_undef = 0;
		(void) Gcomplex(&udv_ptr->udv_value, value, 0.0);
	    } else if (udv_ptr->udv_value.type == INTGR) {
		(void) Gcomplex(&udv_ptr->udv_value, (double) udv_ptr->udv_value.v.int_val, 0.0);
	    }
	    return real(&(udv_ptr->udv_value));
	}
    /* get here => not found */

    {
	struct value tempval;
	(void) Gcomplex(&tempval, value, 0.0);
	setvar(varname, tempval);
    }

    return value;
}


/*****************************************************************
    Split Identifier into path and filename
*****************************************************************/
static void
splitpath(char *s, char *p, char *f)
{
    char *tmp = s + strlen(s) - 1;

    while (tmp >= s && *tmp != '\\' && *tmp != '/' && *tmp != ':')
	tmp--;
    /* FIXME HBB 20010121: unsafe! Sizes of 'f' and 'p' are not known.
     * May write past buffer end. */
    strcpy(f, tmp + 1);
    memcpy(p, s, (size_t) (tmp - s + 1));
    p[tmp - s + 1] = NUL;
}


/* argument: char *fn */
#define VALID_FILENAME(fn) ((fn) != NULL && (*fn) != '\0')

/*****************************************************************
    write the actual parameters to start parameter file
*****************************************************************/
void
update(char *pfile, char *npfile)
{
    char fnam[256], path[256], sstr[256], pname[64], tail[127], *s = sstr, *tmp, c;
    char ifilename[256], *ofilename;
    FILE *of, *nf;
    double pval;


    /* update pfile npfile:
       if npfile is a valid file name,
       take pfile as input file and
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

    /* split into path and filename */
    splitpath(ifilename, path, fnam);
    if (!(of = loadpath_fopen(ifilename, "r")))
	Eex2("parameter file %s could not be read", ifilename);

    if (!(nf = fopen(ofilename, "w")))
	Eex2("new parameter file %s could not be created", ofilename);

    while (fgets(s = sstr, sizeof(sstr), of) != NULL) {

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
	if (!is_variable(tmp) || strlen(tmp) > MAX_ID_LEN) {
	    (void) fclose(nf);
	    (void) fclose(of);
	    Eex2("syntax error in parameter file %s", fnam);
	}
	safe_strncpy(pname, tmp, sizeof(pname));
	/* next must be '=' */
	if (c != '=') {
	    tmp = strchr(s, '=');
	    if (tmp == NULL) {
		(void) fclose(nf);
		(void) fclose(of);
		Eex2("syntax error in parameter file %s", fnam);
	    }
	    s = tmp + 1;
	}
	tmp = get_next_word(&s, &c);
	if (!sscanf(tmp, "%lf", &pval)) {
	    (void) fclose(nf);
	    (void) fclose(of);
	    Eex2("syntax error in parameter file %s", fnam);
	}
	if ((tmp = get_next_word(&s, &c)) != NULL) {
	    (void) fclose(nf);
	    (void) fclose(of);
	    Eex2("syntax error in parameter file %s", fnam);
	}
	/* now modify */

	if ((pval = getdvar(pname)) == 0)
	    pval = (double) getivar(pname);

	sprintf(sstr, "%g", pval);
	if (!strchr(sstr, '.') && !strchr(sstr, 'e'))
	    strcat(sstr, ".0");	/* assure CMPLX-type */

	fprintf(nf, "%-15.15s = %-15.15s   %s", pname, sstr, tail);
    }

    if (fclose(nf) || fclose(of))
	Eex("I/O error during update");

}


/*****************************************************************
    Backup a file by renaming it to something useful. Return
    the new name in tofile
*****************************************************************/

/* tofile must point to a char array[] or allocated data. See update() */

static void
backup_file(char *tofile, const char *fromfile)
{
#if defined (WIN32) || defined(MSDOS) || defined(VMS)
    char *tmpn;
#endif

    /* win32 needs to have two attempts at the rename, since it may
     * be running on win32s with msdos 8.3 names
     */

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
#endif


#if defined (WIN32) || defined(MSDOS)

    /* first attempt for msdos. Second attempt for win32s */

    /* Copy only the first 8 characters of the filename, to comply
     * with the restrictions of FAT filesystems. */
    safe_strncpy(tofile, fromfile, 8 + 1);

    while ((tmpn = strchr(tofile, '.')) != NULL)
	*tmpn = '_';

    strcat(tofile, BACKUP_SUFFIX);

    if (rename(fromfile, tofile) == 0)
	return;			/* success */

#endif /* win32 || msdos */

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
    } else if (this_axis->is_timedata) {
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
    } else if (this_axis->is_timedata) {
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
      /* Store the Z axis dummy variable and range (if any) in the
       * axis for the next independent variable.  Save the current
       * values in an otherwise unused axis, so they can be restored
       * later if there are fewer than 5 independent variables. */
      axis_array[SECOND_Z_AXIS] = axis_array[i]; /* copy entire structure */
      dummy_token[6] = dummy_token[i];

      dummy_token[num_ranges] = -1;
      PARSE_NAMED_RANGE(i, dummy_token[num_ranges]); /* FIXME both should be i? */
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
    for (i=0; i<5; i++){
      if (dummy_token[i] >= 0)
	copy_str(c_dummy_var[i], dummy_token[i], MAX_ID_LEN);
      else
	strcpy(c_dummy_var[i], dummy_default[i]);
    }

    func.at = perm_at();	/* parse expression and save action table */
    dummy_func = NULL;

    token2 = c_token;

    /* get filename */
    file_name = try_to_get_string();
    if (!file_name)
	int_error(c_token, "missing filename");

    /* use datafile module to parse the datafile and qualifiers */
    df_set_plot_mode(MODE_QUERY);  /* Does nothing except for binary datafiles */
    columns = df_open(file_name, 7, NULL);	/* up to 7 using specs allowed */
    free(file_name);
    if (columns < 0)
	int_error(NO_CARET,"Can't read data file");
    if (columns == 1)
	int_error(c_token, "Need 2 to 7 using specs");

    num_indep = (columns<3)?1:columns-2;

    /* The following patch was made by Remko Scharroo, 25-Mar-1999
     * We need to check if one of the columns is time data, like
     * in plot2d and plot3d */

    if (X_AXIS.is_timedata) {
	if (columns < 2)
	    int_error(c_token, "Need full using spec for x time data");
    }
    if (Y_AXIS.is_timedata) {
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
    if ((dummy_token[1] >= 0) && (num_indep==1))
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
      axis_array[i] = axis_array[SECOND_Z_AXIS]; /* copy entire structure */
      dummy_token[num_ranges-1] = dummy_token[6];
    }

    /* defer actually reading the data until we have parsed the rest
     * of the line */

    token3 = c_token;

    tmpd = getdvar(FITLIMIT);	/* get epsilon if given explicitly */
    if (tmpd < 1.0 && tmpd > 0.0)
	epsilon = tmpd;

    /* HBB 970304: maxiter patch */
    maxiter = getivar(FITMAXITER);

    /* get startup value for lambda, if given */
    tmpd = getdvar(FITSTARTLAMBDA);

    if (tmpd > 0.0) {
	startup_lambda = tmpd;
	printf("Lambda Start value set: %g\n", startup_lambda);
    }
    /* get lambda up/down factor, if given */
    tmpd = getdvar(FITLAMBDAFACTOR);

    if (tmpd > 0.0) {
	lambda_up_factor = lambda_down_factor = tmpd;
	printf("Lambda scaling factors reset:  %g\n", lambda_up_factor);
    }
    free(fit_script);
    fit_script = NULL;
    if ((tmp = getenv(FITSCRIPT)) != NULL) {
	fit_script = gp_strdup(tmp);
    }

    {
	char *logfile = getfitlogfile();

	if (!log_f && !(log_f = fopen(logfile, "a")))
	    Eex2("could not open log-file %s", logfile);

	if (logfile)
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
	for (i=0; i<num_indep && i<columns-1; i++)
	  fprintf(log_f, "%s:", c_dummy_var[i]);
	fprintf(log_f, (columns<=2) ? "z\n" : "z:s\n");
    }

    /* report all range specs */
    j = FIRST_Z_AXIS;		/* check Z axis first */
    for (i=0; i<=num_indep; i++) {
      if ((axis_array[j].autoscale & AUTOSCALE_BOTH) != AUTOSCALE_BOTH)
	log_axis_restriction(log_f, j, i ? c_dummy_var[i] : "z");
      j=var_order[i];
    }

    max_data = MAX_DATA;
    fit_x = vec(max_data*num_indep); /* start with max. value */
    fit_z = vec(max_data);
    err_data = vec(max_data);
    num_data = 0;

    for (i=0; i<sizeof(skipped)/sizeof(int); i++)
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
      for (i=0; i<6; i++) {
	int j = var_order[i];
	AXIS *this = axis_array + j;
	if (skipped[j]){
	  printf("         Skipped %d points outside range [%s=",
		 skipped[j], i<5 ? c_dummy_var[i] : "z");
	  if (this->autoscale&AUTOSCALE_MIN)
	    printf("*:");
	  else
	    printf("%g:",this->min);
	  if (this->autoscale&AUTOSCALE_MAX)
	    printf("*]\n");
	  else
	    printf("%g]\n",this->max);
	}
      }
      Eex("No data to fit ");
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
	    if (!is_variable(tmp) || strlen(tmp) > MAX_ID_LEN) {
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
		struct value tempval;
		(void) Gcomplex(&tempval, tmp_par, 0.0);
		/* use parname as temp */
		setvar(par_name[num_params], tempval);
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

    /* avoid parameters being equal to zero */
    for (i = 0; i < num_params; i++)
	if (a[i] == 0)
	    a[i] = NEARLY_ZERO;


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
    free(par_name);
    if (func.at) {
	free_at(func.at);		/* release perm. action table */
	func.at = (struct at_type *) NULL;
    }
    safe_strncpy(last_fit_command, gp_input_line, sizeof(last_fit_command));
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
    if (!fit_quiet)
        vfprintf(STANDARD, fmt, args);
    va_end(args);
    VA_START(args, fmt);
    vfprintf(log_f, fmt, args);
# else
    if (!fit_quiet)
        _doprnt(fmt, args, STANDARD);
    _doprnt(fmt, args, log_f);
# endif
    va_end(args);
#else
    if (!fit_quiet)
        fprintf(STANDARD, fmt, a1, a2, a3, a4, a5, a6, a7, a8);
    fprintf(log_f, fmt, a1, a2, a3, a4, a5, a6, a7, a8);
#endif /* VA_START */
}

/* HBB/H.Harders NEW 20020927: make fit log filename exchangeable at
 * run-time, not only by setting an environment variable. */
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
