#ifndef lint
static char *RCSid = "$Id: fit.c,v 1.60 1998/06/22 12:24:50 ddenholm Exp $";
#endif

/*
 *	Nonlinear least squares fit according to the
 *	Marquardt-Levenberg-algorithm
 *
 *	added as Patch to Gnuplot (v3.2 and higher)
 *	by Carsten Grammes
 *	Experimental Physics, University of Saarbruecken, Germany
 *
 *	Internet address: cagr@rz.uni-sb.de
 *
 *	Copyright of this module:  1993, 1998  Carsten Grammes
 *
 *	Permission to use, copy, and distribute this software and its
 *	documentation for any purpose with or without fee is hereby granted,
 *	provided that the above copyright notice appear in all copies and
 *	that both that copyright notice and this permission notice appear
 *	in supporting documentation.
 *
 *	This software is provided "as is" without express or implied warranty.
 *
 *	930726:     Recoding of the Unix-like raw console I/O routines by:
 *		    Michele Marziani (marziani@ferrara.infn.it)
 * drd: start unitialised variables at 1 rather than NEARLY_ZERO
 *  (fit is more likely to converge if started from 1 than 1e-30 ?)
 *
 * HBB (Broeker@physik.rwth-aachen.de) : fit didn't calculate the errors
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
 */


#define FIT_MAIN

#include <math.h>
#include <ctype.h>
#include <signal.h>

#if defined(MSDOS) || defined(DOS386)	     /* non-blocking IO stuff */
#include <io.h>
#include <conio.h>
#include <dos.h>
#if (DJGPP==2) /* HBB: for unlink() */
#include <unistd.h>
#endif
#else
#ifdef OSK
#include <stdio.h>
#else
#ifndef VMS
#include <fcntl.h>
#endif	/* VMS */
#endif /* OSK */
#endif	/* DOS */

#if defined(ATARI) || defined(MTOS)
#include <osbind.h>
#include <stdio.h>
#include <time.h>
#define getchx() Crawcin()
int kbhit(void);
#endif

#define STANDARD    stderr	/* compatible with gnuplot philosophy */

#define BACKUP_SUFFIX ".old"

#include "plot.h"
#include "type.h"               /* own types */
#include "matrix.h"
#include "fit.h"
#include "setshow.h"   /* for load_range */
#include "alloc.h"


/* access external global variables  (ought to make a globals.h someday) */

extern struct udft_entry *dummy_func;
extern char dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];
extern char c_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];
extern int c_token;
extern int df_datum, df_line_number;
   

enum marq_res { OK, ERROR, BETTER, WORSE };
typedef enum marq_res marq_res_t;

#ifdef INFINITY
#undef INFINITY
#endif
#define INFINITY    1e30
#define NEARLY_ZERO 1e-30
#define INITIAL_VALUE 1.0  /* create new variables with this value (was NEARLY_ZERO) */
#define DELTA	    0.001
#define MAX_DATA    2048
#define MAX_PARAMS  32
#define MAX_LAMBDA  1e20
#define MAX_VALUES_PER_LINE	32
#define MAX_VARLEN  32
#define START_LAMBDA 0.01
#if defined(MSDOS) || defined(OS2) || defined(DOS386)
#define PLUSMINUS   "\xF1"                      /* plusminus sign */
#else
#define PLUSMINUS   "+/-"
#endif

static double epsilon	   = 1e-5;		/* convergence criteria */
static int maxiter         = 0;	    /* HBB 970304: maxiter patch */

static char fit_script[127];
static char logfile[128]    = "fit.log";
static char *FIXED	    = "# FIXED";
static char *GNUFITLOG	    = "FIT_LOG";
static char *FITLIMIT	    = "FIT_LIMIT";
static char *FITMAXITER     = "FIT_MAXITER"; /* HBB 970304: maxiter patch */
static char *FITSCRIPT	    = "FIT_SCRIPT";
static char *DEFAULT_CMD    = "replot";         /* if no fitscript spec. */


static FILE	    *log_f = NULL;

static word	    num_data,
		    num_params;
static int	    columns; /* HBB: file-global now, for use in regress() */
static double	    *fit_x,
		    *fit_y,
		    *fit_z,
		    *err_data,
		    *a;
static boolean	    ctrlc_flag = FALSE;

static struct udft_entry func;

typedef char fixstr[MAX_VARLEN+1];
static fixstr	    *par_name;


/*****************************************************************
			 internal Prototypes
*****************************************************************/

static RETSIGTYPE ctrlc_handle __PROTO((int an_int));
static void ctrlc_setup __PROTO((void));
static marq_res_t marquardt __PROTO((double a[], double **alpha, double *chisq,
			        double *lambda, double *varianz));
static boolean analyze __PROTO((double a[], double **alpha, double beta[],
			    double *chisq, double *varianz));
static void calculate __PROTO((double *yfunc, double **dyda, double a[]));
static void call_gnuplot __PROTO((double *par, double *data));
static void show_fit __PROTO((int i, double chisq, double last_chisq, double *a,
		          double lambda, FILE *device));
static boolean regress __PROTO((double a[]));
#if 0 /* not used */
static int scan_num_entries __PROTO((char *s, double vlist[]));
#endif
static boolean is_variable __PROTO((char *s));
static void copy_max __PROTO((char *d, char *s, int n));
static double getdvar __PROTO((char *varname));
static double createdvar __PROTO((char *varname, double value));
static void splitpath __PROTO((char *s, char *p, char *f));
static void backup_file __PROTO((char *, const char *));
#if defined(MSDOS) || defined(DOS386)
static void subst __PROTO((char *s, char from, char to));
#endif
static boolean fit_interrupt __PROTO((void));
static boolean is_empty __PROTO((char *s));

/*****************************************************************
    Useful macros
    We avoid any use of varargs/stdargs (not good style but portable)
*****************************************************************/
#define Dblf(a) 	{fprintf (STANDARD,a); fprintf (log_f,a);}
#define Dblf2(a,b)	{fprintf (STANDARD,a,b); fprintf (log_f,a,b);}
#define Dblf3(a,b,c)    {fprintf (STANDARD,a,b,c); fprintf (log_f,a,b,c);}
#define Dblf5(a,b,c,d,e) \
		{fprintf (STANDARD,a,b,c,d,e); fprintf (log_f,a,b,c,d,e);}
#define Dblf6(a,b,c,d,e,f) \
                {fprintf (STANDARD,a,b,c,d,e,f); fprintf (log_f,a,b,c,d,e,f);}

/*****************************************************************
    This is called when a SIGINT occurs during fit
*****************************************************************/

static RETSIGTYPE ctrlc_handle (an_int)
int an_int;
{
#ifdef OS2
    (void) signal(an_int, SIG_ACK);
#else
    /* reinstall signal handler (necessary on SysV) */
    (void) signal(SIGINT, (sigfunc)ctrlc_handle);
#endif
    ctrlc_flag = TRUE;
}


/*****************************************************************
    setup the ctrl_c signal handler
*****************************************************************/
static void ctrlc_setup()
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
#if (defined(__EMX__) || !defined(MSDOS) && !defined(DOS386)) && !defined(ATARI) && !defined(MTOS)
	(void) signal(SIGINT, (sigfunc)ctrlc_handle);
#endif
}


/*****************************************************************
    getch that handles also function keys etc.
*****************************************************************/
#if defined(MSDOS) || defined(DOS386)
/* HBB 980317: added a prototype... */
int getchx __PROTO((void));

int getchx ()
{
    register int c = getch();
    if ( !c  ||  c == 0xE0 ) {
	c <<= 8;
	c |= getch ();
    }
    return c;
}
#endif

/*****************************************************************
    in case of fatal errors
*****************************************************************/
void error_ex ()
{
    char *sp;

    strncpy (fitbuf, "         ", 9);         /* start after GNUPLOT> */
    sp = strchr (fitbuf, '\0');
    while ( *--sp == '\n' )
	;
    strcpy (sp+1, "\n\n");              /* terminate with exactly 2 newlines */
    fprintf (STANDARD, fitbuf);
      if ( log_f ) {
	fprintf (log_f, "BREAK: %s", fitbuf);
  	fclose (log_f);
  	log_f = NULL;
      }
    if ( func.at ) {
	free (func.at); 		    /* release perm. action table */
	func.at = (struct at_type *) NULL;
    }

    /* restore original SIGINT function */
#if !defined(ATARI) && !defined(MTOS)
    interrupt_setup ();
#endif

    bail_to_command_line();
}


/*****************************************************************
    Marquardt's nonlinear least squares fit
*****************************************************************/
static marq_res_t marquardt (a, alpha, chisq, lambda, varianz)
double a[];
double **alpha;
double *chisq;
double *lambda;
double *varianz;
{
    int 	    j;
    static double   *da,	    /* delta-step of the parameter */
		    *temp_a,	    /* temptative new params set   */
		    **one_da,
		    *beta,
		    *tmp_beta,
		    **tmp_alpha,
		    **covar,
		    old_chisq;

    /* Initialization when lambda < 0 */

    if ( *lambda < 0 ) {	/* Get first chi-square check */
	one_da	    = matr (num_params, 1);
	temp_a	    = vec (num_params);
	beta	    = vec (num_params);
	tmp_beta    = vec (num_params);
	da	    = vec (num_params);
	covar	    = matr (num_params, num_params);
	tmp_alpha   = matr (num_params, num_params);

	*lambda  = -*lambda;	  /* make lambda positive */
	return analyze (a, alpha, beta, chisq, varianz) ? OK : ERROR;
    }
    old_chisq = *chisq;

    for ( j=0 ; j<num_params ; j++ ) {
	memcpy (covar[j], alpha[j], num_params*sizeof(double));
	covar[j][j] *= 1 + *lambda;
	one_da[j][0] = beta [j];
    }

    solve (covar, num_params, one_da, 1);	/* Equation solution */

    for ( j=0 ; j<num_params ; j++ )
	da[j] = one_da[j][0];			/* changes in paramss */

    /* once converged, free dynamic allocated vars */

    if ( *lambda == 0.0 ) {
	free (beta);
	free (tmp_beta);
	free (da);
	free (temp_a);
	free_matr (one_da, num_params);
	free_matr (tmp_alpha, num_params);
	free_matr (covar, num_params);
        return OK;
    }

    /* check if trial did ameliorate sum of squares */

    for ( j=0 ; j<num_params ; j++ ) {
	temp_a[j] = a[j] + da[j];
	tmp_beta[j] = beta [j];
	memcpy (tmp_alpha[j], alpha[j], num_params*sizeof(double));
    }

    if ( !analyze (temp_a, tmp_alpha, tmp_beta, chisq, varianz) )
	return ERROR;

    if ( *chisq < old_chisq ) {     /* Success, accept new solution */
	if ( *lambda > 1e-20 )
	    *lambda /= 10;
	old_chisq = *chisq;
	for ( j=0 ; j<num_params ; j++ ) {
	    memcpy (alpha[j], tmp_alpha[j], num_params*sizeof(double));
	    beta[j] = tmp_beta[j];
	    a[j] = temp_a[j];
	}
	return BETTER;
    }
    else {			    /* failure, increase lambda and return */
	*lambda *= 10;
	*chisq = old_chisq;
	return WORSE;
    }
}


/*****************************************************************
    compute chi-square and numeric derivations
*****************************************************************/
static boolean analyze (a, alpha, beta, chisq, varianz)
double a[];
double **alpha;
double beta[];
double *chisq;
double *varianz;
{

/*
 *  used by marquardt to evaluate the linearized fitting matrix alpha
 *  and vector beta
 */

    word    k, j, i;
    double  wt, sig2i, dz, **dzda, *zfunc;
#if !defined(ATARI) && !defined(MTOS)
    double *edp, *zp, *fp, **dr, **ar, *dc, *bp, *ac;
#endif

    zfunc = vec (num_data);
    dzda = matr (num_data, num_params);

    /* initialize alpha beta */
    for ( j=0 ; j<num_params ; j++ ) {
	for ( k=0 ; k<=j ; k++ )
	    alpha[j][k] = 0;
	beta[j] = 0;
    }

    *chisq = *varianz = 0;
    calculate (zfunc, dzda, a);

#if !defined(ATARI) && !defined(MTOS)
    edp = err_data;
    zp = fit_z;
    fp = zfunc;
    dr = dzda;

    /* Summation loop over all data */
    for ( i=0 ; i<num_data ; i++, dr++ ) {
	/* The original code read: sig2i = 1/(*edp * *edp++); */
	/* There are some compilers that evaluate the operation */
	/* from right to left, although it is an error to do so. */
	/* Hence the following modification: */
	sig2i = 1/(*edp * *edp);
	edp++;
	dz = *zp++ - *fp++;
	*varianz += dz*dz;
	ar = alpha;
        dc = *dr;
	bp = beta;
	for ( j=0 ; j<num_params ; j++ ) {
	    wt = *dc++ * sig2i;
	    ac = *ar++;
            for ( k=0 ; k<=j ; k++ )
		*ac++ += wt * (*dr)[k];
	    *bp++ += dz * wt;
	}
	*chisq += dz*dz*sig2i;		    /* and find chi-square */
    }
#else
     /* Summation loop over all data */
    for ( i=0 ; i<num_data ; i++) {
        sig2i = 1/(err_data[i] * err_data[i]);
        dz = fit_z[i] - zfunc[i];
	*varianz += dz*dz;
        for ( j=0 ; j<num_params ; j++ ) {
	    wt = dzda[i][j] * sig2i;
            for ( k=0 ; k<=j ; k++ )
		alpha[j][k] += wt * dzda[i][k];
            beta[j] += dz * wt;
	}
	*chisq += dz*dz*sig2i;		    /* and find chi-square */
    }
#endif
    *varianz /= num_data;
    for ( j=1 ; j<num_params ; j++ )	    /* fill in the symmetric side */
	for ( k=0 ; k<=j-1 ; k++ )
	    alpha[k][j] = alpha [j][k];
    free (zfunc);
    free_matr (dzda, num_data);
    return TRUE;
}


/*****************************************************************
    compute function values and partial derivatives of chi-square
*****************************************************************/
static void calculate (zfunc, dzda, a)
double *zfunc;
double **dzda;
double a[];
{
    word    k, p;
    double  tmp_a;
    double  *tmp_low,
	    *tmp_high,
	    *tmp_pars;

    tmp_low = vec (num_data);	      /* numeric derivations */
    tmp_high = vec (num_data);
    tmp_pars = vec (num_params);

    /* first function values */

    call_gnuplot (a, zfunc);

    /* then derivatives */

    for ( p=0 ; p<num_params ; p++ )
	tmp_pars[p] = a[p];
    for ( p=0 ; p<num_params ; p++ ) {
	tmp_pars[p] = tmp_a = fabs(a[p]) < NEARLY_ZERO ? NEARLY_ZERO : a[p];
/*
 *  the more exact method costs double execution time and is therefore
 *  commented out here. Change if you like!
 */
/*	  tmp_pars[p] *= 1-DELTA;
	call_gnuplot (tmp_pars, tmp_low);  */

	tmp_pars[p] = tmp_a * (1+DELTA);
	call_gnuplot (tmp_pars, tmp_high);
	for ( k=0 ; k<num_data ; k++ )

/*            dyda[k][p] = (tmp_high[k] - tmp_low[k])/(2*tmp_a*DELTA); */

	    dzda[k][p] = (tmp_high[k] - zfunc[k])/(tmp_a*DELTA);
        tmp_pars[p] = a[p];
    }

    free (tmp_low);
    free (tmp_high);
    free (tmp_pars);
}


/*****************************************************************
    call internal gnuplot functions
*****************************************************************/
static void call_gnuplot (par, data)
double *par;
double *data;
{
    word    i;
    struct value v;

    /* set parameters first */
    for ( i=0 ; i<num_params ; i++ ) {
	Gcomplex (&v, par[i], 0.0);
	setvar (par_name[i], v);
    }

    for (i = 0; i < num_data; i++) {
	/* calculate fit-function value */
	(void) Gcomplex (&func.dummy_values[0], fit_x[i], 0.0);
	(void) Gcomplex (&func.dummy_values[1], fit_y[i], 0.0);
	evaluate_at (func.at,&v);
	if ( undefined )
	    Eex ("Undefined value during function evaluation")
	data[i] = real(&v);
    }
}


/*****************************************************************
    handle user interrupts during fit
*****************************************************************/
static boolean fit_interrupt ()
{
    while ( TRUE ) {
	fprintf (STANDARD, "\n\n(S)top fit, (C)ontinue, (E)xecute:  ");
	switch (getc(stdin)) {

	case EOF :
	case 's' :
	case 'S' :  fprintf (STANDARD, "Stop.");
		    return FALSE;

	case 'c' :
	case 'C' :  fprintf (STANDARD, "Continue.");
		    return TRUE;

	case 'e' :
	case 'E' :  {
			int i;
			struct value v;
			char *tmp;

			tmp = *fit_script ? fit_script : DEFAULT_CMD;
			fprintf (STANDARD, "executing: %s", tmp);
			/* set parameters visible to gnuplot */
			for ( i=0 ; i<num_params ; i++ ) {
			    Gcomplex (&v, a[i], 0.0);
			    setvar (par_name[i], v);
			}
			sprintf (input_line, tmp);
			(void) do_line ();
		    }
	}
    }
}


/*****************************************************************
    frame routine for the marquardt-fit
*****************************************************************/
static boolean regress (a)
double a[];
{
    double	**alpha,
		chisq,
		last_chisq,
		varianz,
		lambda;
    word         iter; /* iteration count */
    marq_res_t	res;

    chisq   = last_chisq = INFINITY;
    alpha   = matr (num_params, num_params);
    lambda  = -START_LAMBDA;			/* use sign as flag */
    iter = 0;					/* iteration counter  */

    /* ctrlc now serves as Hotkey */
    ctrlc_setup ();

    /* Initialize internal variables and 1st chi-square check */
    if ( (res = marquardt (a, alpha, &chisq, &lambda, &varianz)) == ERROR )
	Eex ("FIT: error occured during fit")
    res = BETTER;

    Dblf ("\nInitial set of free parameters:\n")
    show_fit (iter, chisq, chisq, a, lambda, STANDARD);
    show_fit (iter, chisq, chisq, a, lambda, log_f);



    /* MAIN FIT LOOP: do the regression iteration */

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
 */
#if ((defined(MSDOS) || defined(DOS386)) && !defined(__EMX__)) || defined(ATARI) || defined(MTOS)
 	if ( kbhit () ) {
	    do {getchx ();} while ( kbhit() );
	    ctrlc_flag = TRUE;
	}
#endif

	if ( ctrlc_flag ) {
	    show_fit (iter, chisq, last_chisq, a, lambda, STANDARD);
	    ctrlc_flag = FALSE;
	    if ( !fit_interrupt () )	       /* handle keys */
		break;
	}

	if ( res == BETTER ) {
            iter++;
            last_chisq = chisq;
	}
	
        if ( (res = marquardt (a, alpha, &chisq, &lambda, &varianz)) == BETTER )
	    show_fit (iter, chisq, last_chisq, a, lambda, STANDARD);
	    
    }
    while ( res != ERROR  &&  lambda < MAX_LAMBDA  &&
            ((maxiter == 0) || (iter <= maxiter)) && /* HBB 970304: maxiter patch */
	    (res == WORSE  ||  ( chisq > NEARLY_ZERO ? (last_chisq-chisq)/chisq : (last_chisq-chisq)) > epsilon) );

    /* fit done */

#if !defined(ATARI) && !defined(MTOS)
    /* restore original SIGINT function */
    interrupt_setup ();
#endif

    /* HBB 970304: the maxiter patch: */
    if ((maxiter>0)&&(iter>maxiter)) {
      Dblf2 ("\nMaximum iteration count (%d) reached. Fit stopped.\n", maxiter)
    } else {
      Dblf2 ("\nAfter %d iterations the fit converged.\n", iter)
    }

    Dblf2 ("final sum of squares residuals    : %g\n", chisq)
    
    if (chisq > NEARLY_ZERO) {
	Dblf2 ("rel. change during last iteration : %g\n\n", (chisq-last_chisq)/chisq);
    } else {
	Dblf2 ("abs. change during last iteration : %g\n\n", (chisq-last_chisq))
    }

    if ( res == ERROR )
	Eex ("FIT: error occured during fit")

    /* compute errors in the parameters */

    if (num_data==num_params) {
    	
      word i;
      Dblf("\nExactly as many data points as there are parameters.\n");
      Dblf("In this degenerate case, all errors are zero by definition.\n\n");
      Dblf ("Final set of parameters \n");
      Dblf ("======================= \n\n");
      for ( i=0 ; i<num_params; i++ ) 
        Dblf3 ("%-15.15s = %-15g\n", par_name[i], a[i]);
        
    } else if (varianz < NEARLY_ZERO) {
    	
      word i;
      Dblf("\nHmmmm.... Sum of squared residuals is zero. Can't compute errors.\n\n");
      Dblf ("Final set of parameters \n");
      Dblf ("======================= \n\n");
      for ( i=0 ; i<num_params; i++ ) 
        Dblf3 ("%-15.15s = %-15g\n", par_name[i], a[i]);
        
    } else {

      double **covar, **correl, *dpar;
      word i,j;

      for ( i=0 ; i<num_params; i++ )
	for ( j=0 ; j<num_params; j++ )
	  if (columns <= 2)      /* HBB: if no errors were given:  */
	    alpha[i][j] /= varianz;
	  else                   /* HBB: but if they *were*: */
	    alpha[i][j] /= chisq / (num_data - num_params);
	 
      /* get covariance-, Korrelations- and Kurvature-Matrix */
      /* and errors in the parameters			  */

      covar   = matr (num_params, num_params);
      correl  = matr (num_params, num_params);
      dpar = vec (num_params);

      inverse (alpha, covar, num_params);

      Dblf ("Final set of parameters            68.3%% confidence interval (one at a time)\n")
      Dblf ("=======================            =========================================\n\n")
      for ( i=0 ; i<num_params; i++ ) {
      	double t1, t2;
        if (covar[i][i]<=0.0)     /* HBB: prevent floating point exception later on */
	  Eex("Calculation error: non-positive diagonal element in covar. matrix");
	dpar[i] = sqrt (covar[i][i]);
        t1 = fabs(a[i]);
        t2 = fabs(100.0*dpar[i]/a[i]);
	Dblf6 ("%-15.15s = %-15g  %-3.3s %-15g (%g%%)\n", par_name[i], a[i],
	       PLUSMINUS, dpar[i], (t1 < NEARLY_ZERO)? 0 : t2)
      }

      Dblf ("\n\ncorrelation matrix of the fit parameters:\n\n")
      Dblf ("               ")
      for ( j=0 ; j<num_params ; j++ )
	Dblf2 ("%-6.6s ", par_name[j])
      Dblf ("\n")

      for ( i=0 ; i<num_params; i++ ) {
	Dblf2 ("%-15.15s", par_name[i])
	for ( j=0 ; j<num_params; j++ ) {
	    correl[i][j] = covar[i][j] / (dpar[i]*dpar[j]);
	    Dblf2 ("%6.3f ", correl[i][j])
        }
	Dblf ("\n")
      }
      
      free_matr (covar, num_params);
      free_matr (correl, num_params);
      free (dpar);
      
    }    /* num_data>num_params && varianz>0 */

    /* restore last parameter's value (not done by calculate) */
    {
      struct value val;
      Gcomplex (&val, a[num_params-1], 0.0);
      setvar (par_name[num_params-1], val);
    }

    
    /* call destruktor for allocated vars */
    lambda = 0;
    marquardt (a, alpha, &chisq, &lambda, &varianz);

    free_matr (alpha, num_params);

    return TRUE;
}


/*****************************************************************
    display actual state of the fit
*****************************************************************/
static void show_fit (i, chisq, last_chisq, a, lambda, device)
int i;
double chisq;
double last_chisq;
double *a;
double lambda;
FILE *device;
{
    int k;

    fprintf (device, "\n\n\
Iteration %d\n\
chisquare : %-15g   relative deltachi2 : %g\n\
deltachi2 : %-15g   limit for stopping : %g\n\
lambda	  : %g\n\nactual set of parameters\n\n",
i, chisq, chisq > NEARLY_ZERO ? (chisq - last_chisq)/chisq : 0.0, chisq - last_chisq, epsilon, lambda);
    for ( k=0 ; k<num_params ; k++ )
	fprintf (device, "%-15.15s = %g\n", par_name[k], a[k]);
}



/*****************************************************************
    is_empty: check for valid string entries
*****************************************************************/
static boolean is_empty (s)
char *s;
{
    while ( *s == ' '  ||  *s == '\t'  ||  *s == '\n' )
	s++;
    return (boolean)( *s == '#'  ||  *s == '\0' );
}


/*****************************************************************
    get next word of a multi-word string, advance pointer
*****************************************************************/
char *get_next_word (s, subst)
char **s;
char *subst;
{
    char *tmp = *s;

    while ( *tmp==' ' || *tmp=='\t' || *tmp=='=' )
	tmp++;
    if ( *tmp=='\n' || *tmp=='\0' )                    /* not found */
	return NULL;
    if ( (*s = strpbrk (tmp, " =\t\n")) == NULL )
	*s = tmp + strlen(tmp);
    *subst = **s;
    *(*s)++ = '\0';
    return tmp;
}



/*****************************************************************
    get valid numerical entries
*****************************************************************/
#if 0 /* not used */
static int scan_num_entries (s, vlist)
char *s;
double vlist[];
{
    int num = 0;
    char c, *tmp;

    while ( (tmp = get_next_word (&s, &c)) != NULL
					&&  num <= MAX_VALUES_PER_LINE )
	if ( !sscanf (tmp, "%lf", &vlist[++num]) )
	    Eex ("syntax error in data file")
    return num;
}
#endif

/*****************************************************************
    check for variable identifiers
*****************************************************************/
static boolean is_variable (s)
char *s;
{
    while ( *s ) {
	if ( !isalnum(*s) && *s!='_' )
	    return FALSE;
	s++;
    }
    return TRUE;
}


/*****************************************************************
    strcpy but max n chars
*****************************************************************/
static void copy_max (d, s, n)
char *d;
char *s;
int n;
{
    strncpy (d, s, n);
    if ( strlen(s) >= n )
	d[n-1] = '\0';
}


/*****************************************************************
    portable implementation of strnicmp (hopefully)
*****************************************************************/

#ifdef NEED_STRNICMP
int strnicmp (s1, s2, n)
char *s1;
char *s2;
int n;
{
    char c1,c2;

    if(n==0) return 0;

    do {
	c1 = *s1++; if(islower(c1)) c1=toupper(c1);
	c2 = *s2++; if(islower(c2)) c2=toupper(c2);
    } while(c1==c2 && c1 && c2 && --n>0);

    if(n==0 || c1==c2) return 0;
    if(c1=='\0' || c1<c2) return 1;
    return -1;
}
#endif


/*****************************************************************
    first time settings
*****************************************************************/
void init_fit ()
{
    func.at = (struct at_type *) NULL;      /* need to parse 1 time */
}


/*****************************************************************
	    Set a GNUPLOT user-defined variable
******************************************************************/

void setvar (varname, data)
char *varname;
struct value data;
{
    register struct udvt_entry *udv_ptr = first_udv,
			       *last = first_udv;

    /* check if it's already in the table... */

    while (udv_ptr) {
	    last = udv_ptr;
	    if ( !strcmp (varname, udv_ptr->udv_name))
		break;
	    udv_ptr = udv_ptr->next_udv;
    }

    if ( !udv_ptr ) {			    /* generate new entry */
	last->next_udv = udv_ptr = (struct udvt_entry *)
	  gp_alloc ((unsigned int)sizeof(struct udvt_entry), "fit setvar");
	udv_ptr->next_udv = NULL;
    }
    copy_max (udv_ptr->udv_name, varname, sizeof(udv_ptr->udv_name));
    udv_ptr->udv_value = data;
    udv_ptr->udv_undef = FALSE;
}



/*****************************************************************
    Read INTGR Variable value, return 0 if undefined or wrong type
*****************************************************************/
int getivar (varname)
char *varname;
{
    register struct udvt_entry *udv_ptr = first_udv;

    while (udv_ptr) {
	    if ( !strcmp (varname, udv_ptr->udv_name))
		return udv_ptr->udv_value.type == INTGR
		       ? udv_ptr->udv_value.v.int_val	/* valid */
		       : 0;				/* wrong type */
            udv_ptr = udv_ptr->next_udv;
    }
    return 0;						/* not in table */
}


/*****************************************************************
    Read DOUBLE Variable value, return 0 if undefined or wrong type
   I dont think it's a problem that it's an integer - div
*****************************************************************/
static double getdvar (varname)
char *varname;
{
    register struct udvt_entry *udv_ptr = first_udv;

    for ( ; udv_ptr ; udv_ptr=udv_ptr->next_udv)
	    if ( strcmp (varname, udv_ptr->udv_name)==0)
	      return real(&(udv_ptr->udv_value));

    /* get here => not found */
    return 0;
}

/* like getdvar, but
 * - convert it from integer to real if necessary
 * - create it with value INITIAL_VALUE if not found or undefined
 */
static double createdvar(varname,value)
char *varname;
double value;
{
    register struct udvt_entry *udv_ptr = first_udv;

    for ( ; udv_ptr ; udv_ptr=udv_ptr->next_udv)
	    if ( strcmp (varname, udv_ptr->udv_name)==0) {
	    	if (udv_ptr->udv_undef) {
	    		udv_ptr->udv_undef=0;
	    		Gcomplex(&udv_ptr->udv_value, value, 0.0);
	    	} else if (udv_ptr->udv_value.type==INTGR) {
	    		Gcomplex(&udv_ptr->udv_value, (double)udv_ptr->udv_value.v.int_val, 0.0);
	    	}
		   return real(&(udv_ptr->udv_value));
		 }

    /* get here => not found */

    {
        struct value a;
        Gcomplex(&a, value, 0.0);
        setvar(varname, a);
    }
    
    return value;
}
	

/*****************************************************************
    Split Identifier into path and filename
*****************************************************************/
static void splitpath (s, p, f)
char *s;
char *p;
char *f;
{
    register char *tmp;
    tmp = s + strlen(s) - 1;
    while ( *tmp != '\\'  &&  *tmp != '/'  &&  *tmp != ':'  &&  tmp-s >= 0 )
	tmp--;
    strcpy (f, tmp+1);
    strncpy (p, s, tmp-s+1);
    p[tmp-s+1] = '\0';
}


#if defined(MSDOS) || defined(DOS386)
/*****************************************************************
    Character substitution
*****************************************************************/
#ifdef PROTOTYPES
static void subst (char *s, char from, char to)
#else
static void subst (s, from, to)
char *s;
char from;
char to;
#endif
{
    while ( *s ) {
	if ( *s == from )
	    *s = to;
	s++;
    }
}
#endif


/*****************************************************************
    write the actual parameters to start parameter file
*****************************************************************/
void update (pfile, npfile)
char *pfile, *npfile;
{
    char	fnam[256],
		path[256],
		sstr[256],
		pname[64],
		tail[127],
		*s = sstr,
		*tmp,
		c;
    char        ifilename[256], *ofilename;
    FILE        *of,
		*nf;
    double	pval;


    /* update pfile npfile:
       if npfile is a valid file name,
       take pfile as input file and
       npfile as output file
     */
	if (VALID_FILENAME(npfile)) {
		strncpy (ifilename, pfile, sizeof ifilename);
		ofilename = npfile;
	} else {
#ifdef VMS
		/* filesystem will keep original as previous version */
		strncpy (ifilename, pfile, sizeof ifilename);
#else
		backup_file (ifilename, pfile); /* will Eex if it fails */
#endif
		ofilename = pfile;
	}

	/* split into path and filename */
	splitpath (ifilename, path, fnam);
	if ( !(of = fopen (ifilename, "r")) )
		Eex2 ("parameter file %s could not be read", ifilename)

	if ( !(nf = fopen (ofilename, "w")) ) 
		Eex2 ("new parameter file %s could not be created", ofilename)

	while ( fgets (s = sstr, sizeof(sstr), of) != NULL ) {

		if ( is_empty(s) ) {
			fprintf (nf, s);				/* preserve comments */
            continue;
        }
        if ( (tmp = strchr (s, '#')) != NULL ) {
	    copy_max (tail, tmp, sizeof(tail));
	    *tmp = '\0';
	}
	else
	    strcpy (tail, "\n");
	tmp = get_next_word (&s, &c);
	if ( !is_variable (tmp)  ||  strlen(tmp) > MAX_VARLEN ) {
	    fclose (nf);
	    fclose (of);
	    Eex2 ("syntax error in parameter file %s", fnam)
	}
	copy_max (pname, tmp, sizeof(pname));
	/* next must be '=' */
	if ( c != '=' ) {
	    tmp = strchr (s, '=');
	    if ( tmp == NULL ) {
	        fclose (nf);
		fclose (of);
		Eex2 ("syntax error in parameter file %s", fnam)
	    }
	    s = tmp+1;
	}
	tmp = get_next_word (&s, &c);
	if ( !sscanf (tmp, "%lf", &pval) ) {
	    fclose (nf);
	    fclose (of);
	    Eex2 ("syntax error in parameter file %s", fnam)
	}
	if ( (tmp = get_next_word (&s, &c)) != NULL ) {
	    fclose (nf);
	    fclose (of);
	    Eex2 ("syntax error in parameter file %s", fnam)
	}

        /* now modify */

	if ( (pval = getdvar (pname)) == 0 )
	    pval = (double) getivar (pname);

	sprintf (sstr, "%g", pval);
	if ( !strchr (sstr, '.')  &&  !strchr (sstr, 'e') )
	    strcat (sstr, ".0");                /* assure CMPLX-type */
	fprintf (nf, "%-15.15s = %-15.15s   %s", pname, sstr, tail);
    }
    if ( fclose (nf) || fclose (of) )
	Eex ("I/O error during update")

}


/*****************************************************************
    Backup a file by renaming it to something useful. Return
    the new name in tofile
*****************************************************************/
static void
backup_file (tofile, fromfile)
char *tofile;
const char *fromfile;
/* tofile must point to a char array[] or allocated data. See update() */
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
    while ( (tmpn = strchr (tofile, '.') ) != NULL)
        *tmpn = '_';
#endif /*VMS*/

    strcat (tofile, BACKUP_SUFFIX);

	if (rename(fromfile, tofile) == 0)
		return; /* hurrah */
#endif


#if defined (WIN32) || defined(MSDOS)

	/* first attempt for msdos. Second attempt for win32s */
		
	/* beware : strncpy is very dangerous since it does not guarantee
	 * to terminate the string. Copy up to 8 characters. If exactly
	 * chars were copied, the string is not terminated. If the
	 * source string was shorter than 8 chars, no harm is done
	 * (here) by writing to offset 8.
	 */
	strncpy(tofile, fromfile, 8);
	tofile[8] = 0;
	
   while ( (tmpn = strchr (tofile, '.') ) != NULL)
        *tmpn = '_';

    strcat (tofile, BACKUP_SUFFIX);

    if (rename(fromfile, tofile) == 0)
    	return;  /* success */

#endif /* win32 || msdos */

	/* get here => rename failed. */
	Eex3("Could not rename file %s to %s", fromfile, tofile);
}


/*****************************************************************
    Interface to the classic gnuplot-software
*****************************************************************/

void do_fit ()
{
    int autorange_x=3, autorange_y=3;  /* yes */
    double min_x, max_x;  /* range to fit */
    double min_y, max_y;  /* range to fit */
    int dummy_x=-1, dummy_y=-1; /* eg  fit [u=...] [v=...] */

    int 	i;
    double	v[4];
    double tmpd;
    time_t	timer;
    int token1, token2, token3;
    char *tmp;

    /* first look for a restricted x fit range... */

    if (equals(c_token, "[")) {
	c_token++;
	if (isletter(c_token)) {
	    if (equals(c_token + 1, "=")) {
		dummy_x = c_token;
		c_token += 2;
	    }
			/* else parse it as a xmin expression */
	}
	autorange_x = load_range(FIRST_X_AXIS,&min_x, &max_x, autorange_x);
	if (!equals(c_token, "]"))
		int_error("']' expected", c_token);
	c_token++;
    }

    /* ... and y */

    if (equals(c_token, "[")) {
	c_token++;
	if (isletter(c_token)) {
	    if (equals(c_token + 1, "=")) {
		dummy_y = c_token;
		c_token += 2;
	    }
	    /* else parse it as a ymin expression */
	}
	autorange_y = load_range(FIRST_Y_AXIS,&min_y, &max_y, autorange_y);
	if (!equals(c_token, "]"))
		int_error("']' expected", c_token);
	c_token++;
    }


/* now compile the function */

    token1 = c_token;

    if (func.at) {
	free(func.at);
	func.at=NULL;  /* in case perm_at() does int_error */
    }

    dummy_func = &func;

    
    /* set the dummy variable names */
    
    if (dummy_x >= 0)
	copy_str(c_dummy_var[0], dummy_x, MAX_ID_LEN);
    else
	strcpy(c_dummy_var[0], dummy_var[0]);

    if (dummy_y >= 0)
	copy_str(c_dummy_var[0], dummy_y, MAX_ID_LEN);
    else
	strcpy(c_dummy_var[1], dummy_var[1]);
	
    func.at = perm_at();
    dummy_func = NULL;

    token2 = c_token;


    
/* use datafile module to parse the datafile and qualifiers */

    columns = df_open(4);  /* up to 4 using specs allowed */
    if (columns==1)
	int_error("Need 2 to 4 using specs", c_token);
    /* defer actually reading the data until we have parsed the rest of the line */
    
	token3 = c_token;

    tmpd = getdvar (FITLIMIT);		  /* get epsilon if given explicitly */
    if ( tmpd < 1.0  &&  tmpd > 0.0 )
        epsilon = tmpd;

    /* HBB 970304: maxiter patch */
    maxiter = getivar (FITMAXITER);

    *fit_script = '\0';
    if ( (tmp = getenv (FITSCRIPT)) != NULL )
	strcpy (fit_script, tmp);

    tmp = getenv (GNUFITLOG);		  /* open logfile */
    if ( tmp != NULL ) {
        char *tmp2 = &tmp[strlen(tmp)-1];
        if ( *tmp2 == '/'  ||  *tmp2 == '\\' )
            sprintf (logfile, "%s%s", tmp, logfile);
        else
            strcpy (logfile, tmp);
    }
    if ( !log_f && /* div */ !(log_f = fopen (logfile, "a")) )
	Eex2 ("could not open log-file %s", logfile)
    fprintf (log_f, "\n\n*******************************************************************************\n");
    time (&timer);
    fprintf (log_f, "%s\n\n", ctime (&timer));
	{	char line[MAX_LINE_LEN];
		capture(line, token2,token3-1,MAX_LINE_LEN);
			fprintf (log_f, "FIT:    data read from %s\n", line);
	}

    fit_x = vec (MAX_DATA);		 /* start with max. value */
    fit_y = vec (MAX_DATA);
    fit_z = vec (MAX_DATA);

/* first read in experimental data */

    err_data = vec (MAX_DATA);
    num_data = 0;

	while ( (i=df_readline(v, 4)) != EOF ) {

		if ( num_data==MAX_DATA ) {
			df_close();
			Eex2 ("max. # of datapoints %d exceeded", MAX_DATA);
		}

		switch (i)
		{
			case DF_UNDEFINED:
			case DF_FIRST_BLANK:
			case DF_SECOND_BLANK:
				continue;
			case 0:
				Eex2("bad data on line %d", df_line_number);
				break;
			case 1:  /* only z provided */
				v[2] = v[0];
				v[0] = df_datum;
				break;
			case 2: /* x and z */
				v[2] = v[1];
				break;

			/* only if the explicitly asked for 4 columns do we
			 * do a 3d fit. (We can get here if they didn't
			 * specify a using spec, and the file has 4 columns
			 */
			case 4:  /* x, y, z, error */
				if (columns == 4)
					break;
				/* else fall through */

			case 3: /* x, z, error */
				v[3] = v[2]; /* error */
				v[2] = v[1]; /* z */
				break;
				
		}

		/* skip this point if it is out of range */
		if ( !(autorange_x&1) && (v[0] < min_x))
			continue;
		if ( !(autorange_x&2) && (v[0] > max_x))
			continue;
		if ( !(autorange_y&1) && (v[1] < min_y))
			continue;
		if ( !(autorange_y&2) && (v[1] > max_y))
			continue;

		fit_x[num_data] = v[0];
		fit_y[num_data] = v[1];
		fit_z[num_data] = v[2];

		/* we only use error if _explicitly_ asked for by a
		 * using spec
		 */
		err_data[num_data++] = (columns > 2) ? v[3] : 1;
	}
	df_close();

	if (num_data <= 1)
		Eex("No data to fit ");
	
    /* now resize fields */
    redim_vec (&fit_x, num_data);
    redim_vec (&fit_y, num_data);
    redim_vec (&fit_z, num_data);
    redim_vec (&err_data, num_data);
    
    fprintf (log_f, "        #datapoints = %d\n", num_data);

    if ( columns < 3 )
	fprintf (log_f, "        y-errors assumed equally distributed\n\n");

	{	char line[MAX_LINE_LEN];
		capture(line, token1, token2-1,MAX_LINE_LEN);		
		fprintf (log_f, "function used for fitting: %s\n", line);
	}

    /* read in parameters */

	if (!equals(c_token, "via"))
		int_error("Need via and either parameter list or file", c_token);

    a = vec (MAX_PARAMS);
    par_name = (fixstr *) gp_alloc ((MAX_PARAMS+1)*sizeof(fixstr), "fit param");
    num_params = 0;

	if (isstring(++c_token)) {
		boolean fixed;
		double	tmp_par;
		char c, *s;
		char sstr[MAX_LINE_LEN];
		FILE *f;

		quote_str(sstr, c_token++, MAX_LINE_LEN);

		/* get parameters and values out of file and ignore fixed ones */

		fprintf (log_f, "take parameters from file: %s\n\n", sstr);
		if ( !(f = fopen (sstr, "r")) )
			Eex2 ("could not read parameter-file %s", sstr);
		while ( TRUE ) {
			if ( !fgets (s = sstr, sizeof(sstr), f) )		/* EOF found */
				break;
			if ( (tmp = strstr (s, FIXED)) != NULL ) {	      /* ignore fixed params */
				*tmp = '\0';
				fprintf (log_f, "FIXED:  %s\n", s);
				fixed = TRUE;
			}
			else
				fixed = FALSE;
			if ( (tmp = strchr (s, '#')) != NULL )
				*tmp = '\0';
			if ( is_empty(s) )
				continue;
			tmp = get_next_word (&s, &c);
			if ( !is_variable (tmp)  ||  strlen(tmp) > MAX_VARLEN ) {
				fclose (f);
				Eex ("syntax error in parameter file");
			}
			
			copy_max (par_name[num_params], tmp, sizeof(fixstr));
			/* next must be '=' */
			if ( c != '=' ) {
				tmp = strchr (s, '=');
				if ( tmp == NULL ) {
					fclose (f);
					Eex ("syntax error in parameter file");
				}
				s = tmp+1;
			}
			tmp = get_next_word (&s, &c);
			if ( sscanf (tmp, "%lf", &tmp_par) != 1 ) {
				fclose (f);
				Eex ("syntax error in parameter file");
			}
			
			/* make fixed params visible to GNUPLOT */
			if ( fixed ) {
				struct value val;
				Gcomplex (&val, tmp_par, 0.0);
				setvar (par_name[num_params], val);   /* use parname as temp */
			}
			else {
				if (num_params>=MAX_PARAMS) {
/* HBB: maybe it would be better to realloc a and v instead ? */
					fclose(f);
					Eex("too many parameters to fit");
				}
				a[num_params++] = tmp_par;
			}
			
			if ( (tmp = get_next_word (&s, &c)) != NULL ) {
				fclose (f);
				Eex ("syntax error in parameter file");
			}
		}
		fclose (f);
	} else { /* not a string after via */
		fprintf (log_f, "use actual parameter values\n\n");
		do {
			if (!isletter(c_token))
				Eex ("no parameter specified");
			capture(par_name[num_params], c_token, c_token, sizeof(par_name[0]));
			if (num_params >= MAX_PARAMS) 
				Eex ("too many parameters to fit");
			/* create variable if it doesn't exist */
			a[num_params] = createdvar (par_name[num_params], INITIAL_VALUE);
			++num_params;
		}	while (equals(++c_token, ",") && ++c_token);
	}
    redim_vec (&a, num_params);
    par_name = (fixstr *) gp_realloc (par_name, (num_params+1)*sizeof(fixstr), "fit param");

    if (num_data < num_params)
	Eex("Number of data points smaller than number of parameters");
	
    /* avoid parameters being equal to zero */
    for ( i=0 ; i<num_params ; i++ )
	if ( a[i] == 0 )
	    a[i] = NEARLY_ZERO;

    regress (a);

    fclose (log_f);
    log_f = NULL;
    free (fit_x);
    free (fit_y);
    free (fit_z);
    free (err_data);
    free (a);
    free (par_name);
    if ( func.at ) {
        free (func.at);                     /* release perm. action table */
        func.at = (struct at_type *) NULL;
    }

}

#if defined(ATARI) || defined(MTOS)
int kbhit()
{
	fd_set rfds;
	struct timeval timeout;
	
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	rfds = (fd_set)(1L << fileno(stdin));
	return((select(0, &rfds, NULL, NULL, &timeout) > 0) ? 1 : 0);
}
#endif


