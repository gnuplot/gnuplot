#ifndef lint
static char *RCSid = "$Id: plot.c%v 3.50.1.8 1993/07/27 05:37:15 woo Exp $";
#endif


/* GNUPLOT - plot.c */
/*
 * Copyright (C) 1986 - 1993   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and 
 * that both that copyright notice and this permission notice appear 
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed 
 * as patches to released version.
 *  
 * This software is provided "as is" without express or implied warranty.
 * 
 *
 * AUTHORS
 * 
 *   Original Software:
 *     Thomas Williams,  Colin Kelley.
 * 
 *   Gnuplot 2.0 additions:
 *       Russell Lang, Dave Kotz, John Campbell.
 *
 *   Gnuplot 3.0 additions:
 *       Gershon Elber and many others.
 * 
 * There is a mailing list for gnuplot users. Note, however, that the
 * newsgroup 
 *	comp.graphics.gnuplot 
 * is identical to the mailing list (they
 * both carry the same set of messages). We prefer that you read the
 * messages through that newsgroup, to subscribing to the mailing list.
 * (If you can read that newsgroup, and are already on the mailing list,
 * please send a message info-gnuplot-request@dartmouth.edu, asking to be
 * removed from the mailing list.)
 *
 * The address for mailing to list members is
 *	   info-gnuplot@dartmouth.edu
 * and for mailing administrative requests is 
 *	   info-gnuplot-request@dartmouth.edu
 * The mailing list for bug reports is 
 *	   bug-gnuplot@dartmouth.edu
 * The list of those interested in beta-test versions is
 *	   info-gnuplot-beta@dartmouth.edu
 */

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#ifdef XPG3_LOCALE
#include <locale.h>
#endif
#include "plot.h"
#include "setshow.h"
#if defined(MSDOS) || defined(DOS386)
#include <io.h>
#endif
#ifdef vms
#include <unixio.h>
#include <smgdef.h>
extern int vms_vkid;
extern smg$create_virtual_keyboard();
unsigned int status[2] = {1, 0};
#endif
#ifdef AMIGA_SC_6_1
#include <proto/dos.h>
#endif

#ifdef _Windows
#include <windows.h>
#ifndef SIGINT
#define SIGINT 2	/* for MSC */
#endif
#else
# ifdef __TURBOC__
# include <graphics.h>
# endif
#endif

#ifndef AMIGA_SC_6_1
extern char *getenv(),*strcat(),*strcpy(),*strncpy();
#endif /* !AMIGA_SC_6_1 */

extern char input_line[];
extern int c_token;
extern FILE *outfile;
extern int term;

TBOOLEAN interactive = TRUE;	/* FALSE if stdin not a terminal */
TBOOLEAN noinputfiles = TRUE;	/* FALSE if there are script files */
char *infile_name = NULL;	/* name of command file; NULL if terminal */

#ifndef STDOUT
#define STDOUT 1
#endif

#ifdef _Windows
jmp_buf far env;
#else
jmp_buf env;
#endif

struct value *Ginteger(),*Gcomplex();


extern f_push(),f_pushc(),f_pushd1(),f_pushd2(),f_pushd(),f_call(),f_calln(),
	f_lnot(),f_bnot(),f_uminus(),f_lor(),f_land(),f_bor(),f_xor(),
	f_band(),f_eq(),f_ne(),f_gt(),f_lt(),
	f_ge(),f_le(),f_plus(),f_minus(),f_mult(),f_div(),f_mod(),f_power(),
	f_factorial(),f_bool(),f_jump(),f_jumpz(),f_jumpnz(),f_jtern();

extern f_real(),f_imag(),f_arg(),f_conjg(),f_sin(),f_cos(),f_tan(),f_asin(),
	f_acos(),f_atan(),f_sinh(),f_cosh(),f_tanh(),f_int(),f_abs(),f_sgn(),
	f_sqrt(),f_exp(),f_log10(),f_log(),f_besj0(),f_besj1(),f_besy0(),f_besy1(),
	f_erf(), f_erfc(), f_gamma(), f_lgamma(), f_ibeta(), f_igamma(), f_rand(),
	f_floor(),f_ceil(),
	f_normal(), f_inverse_erf(), f_inverse_normal();   /* XXX - JG */


struct ft_entry GPFAR ft[] = {	/* built-in function table */

/* internal functions: */
	{"push", f_push},	{"pushc", f_pushc},
	{"pushd1", f_pushd1},	{"pushd2", f_pushd2},	{"pushd", f_pushd},
	{"call", f_call},	{"calln", f_calln},	{"lnot", f_lnot},
	{"bnot", f_bnot},	{"uminus", f_uminus},	{"lor", f_lor},
	{"land", f_land},	{"bor", f_bor},		{"xor", f_xor},
	{"band", f_band},	{"eq", f_eq},		{"ne", f_ne},
	{"gt", f_gt},		{"lt", f_lt},		{"ge", f_ge},
	{"le", f_le},		{"plus", f_plus},	{"minus", f_minus},
	{"mult", f_mult},	{"div", f_div},		{"mod", f_mod},
	{"power", f_power}, {"factorial", f_factorial},
	{"bool", f_bool},	{"jump", f_jump},	{"jumpz", f_jumpz},
	{"jumpnz",f_jumpnz},{"jtern", f_jtern},

/* standard functions: */
	{"real", f_real},	{"imag", f_imag},	{"arg", f_arg},
	{"conjg", f_conjg}, {"sin", f_sin},		{"cos", f_cos},
	{"tan", f_tan},		{"asin", f_asin},	{"acos", f_acos},
	{"atan", f_atan},	{"sinh", f_sinh},	{"cosh", f_cosh},
	{"tanh", f_tanh},	{"int", f_int},		{"abs", f_abs},
	{"sgn", f_sgn},		{"sqrt", f_sqrt},	{"exp", f_exp},
	{"log10", f_log10},	{"log", f_log},		{"besj0", f_besj0},
	{"besj1", f_besj1},	{"besy0", f_besy0},	{"besy1", f_besy1},
        {"erf", f_erf},         {"erfc", f_erfc},       {"gamma", f_gamma},     {"lgamma", f_lgamma},
        {"ibeta", f_ibeta},     {"igamma", f_igamma},   {"rand", f_rand},
        {"floor", f_floor},     {"ceil", f_ceil},

    {"norm",        f_normal},              /* XXX-JG */
    {"inverf",      f_inverse_erf},         /* XXX-JG */
    {"invnorm",     f_inverse_normal},      /* XXX-JG */

	{NULL, NULL}
};

static struct udvt_entry udv_pi = {NULL, "pi",FALSE};
									/* first in linked list */
struct udvt_entry *first_udv = &udv_pi;
struct udft_entry *first_udf = NULL;



#ifdef vms

#define HOME "sys$login:"

#else /* vms */
#if defined(MSDOS) ||  defined(AMIGA_AC_5) || defined(AMIGA_SC_6_1) || defined(ATARI) || defined(OS2) || defined(_Windows) || defined(DOS386)

#define HOME "GNUPLOT"

#else /* MSDOS || AMIGA || ATARI || OS2 || _Windows || defined(DOS386)*/

#define HOME "HOME"

#endif /* MSDOS || AMIGA || ATARI || OS2 || _Windows || defined(DOS386)*/
#endif /* vms */

#if defined(unix) || defined(AMIGA_AC_5) || defined(AMIGA_SC_6_1)
#define PLOTRC ".gnuplot"
#else /* AMIGA || unix */
#define PLOTRC "gnuplot.ini"
#endif /* AMIGA || unix */

#if defined (__TURBOC__) || defined (__PUREC__)
void tc_interrupt()
#else
#ifdef __ZTC__
void ztc_interrupt()
#else
#if defined( _CRAY ) || defined( sgi ) || defined( __alpha )
void inter(an_int)
int an_int;
#else
#if defined( NEXT ) || defined( OS2 ) || defined( VMS )
void inter(int an_int)
#else
#ifdef sgi
void inter(int sig, int code, struct sigcontext *sc)
#else
#if defined(SOLARIS)
void inter()
#else
inter()
#endif
#endif
#endif
#endif
#endif
#endif
{
#if defined (MSDOS) || defined(_Windows) || (defined (ATARI) && defined(__PUREC__)) || defined(DOS386)
#if defined (__TURBOC__) || defined (__PUREC__)
#ifndef DOSX286
	(void) signal(SIGINT, tc_interrupt);
#endif
#else
#ifdef __ZTC__
   (void) signal(SIGINT, ztc_interrupt);
#else
#ifdef __EMX__
	(void) signal(SIGINT, (void *)inter);
#else
#ifdef DJGPP
	(void) signal(SIGINT, (SignalHandler)inter);
#else
#if defined __MSC__
	(void) signal(SIGINT, inter);
#endif	/* __MSC__ */

#endif	/* DJGPP */
#endif  /* __EMX__ */
#endif	/* ZTC */
#endif  /* __TURBOC__ */

#else  /* MSDOS */
#ifdef OS2
	(void) signal(an_int, SIG_ACK);
#else
	(void) signal(SIGINT, inter);
#endif  /* OS2 */
#endif  /* MSDOS */
#ifndef DOSX286
	(void) signal(SIGFPE, SIG_DFL);	/* turn off FPE trapping */
#endif
	if (term && term_init)
		(*term_tbl[term].text)();	/* hopefully reset text mode */
	(void) fflush(outfile);
	(void) putc('\n',stderr);
	longjmp(env, TRUE);		/* return to prompt */
}


#ifdef _Windows
gnu_main(argc, argv)
#else
main(argc, argv)
#endif
	int argc;
	char **argv;
{
#ifdef XPG3_LOCALE
	(void) setlocale(LC_CTYPE, "");
#endif
/* Register the Borland Graphics Interface drivers. If they have been */
/* included by the linker.                                            */

#ifndef DOSX286
#ifndef _Windows
#if defined (__TURBOC__) && defined (MSDOS)
registerfarbgidriver(EGAVGA_driver_far);
registerfarbgidriver(CGA_driver_far);
registerfarbgidriver(Herc_driver_far);
registerfarbgidriver(ATT_driver_far);
# endif
#endif
#endif
#ifdef X11
     { int n = X11_args(argc, argv); argv += n; argc -= n; }
#endif 

#ifdef apollo
    apollo_pfm_catch();
#endif

	setbuf(stderr,(char *)NULL);
#ifdef UNIX
	setlinebuf(stdout);
#endif
	outfile = stdout;
	(void) Gcomplex(&udv_pi.udv_value, Pi, 0.0);

     interactive = FALSE;
     init_terminal();		/* can set term type if it likes */

#ifdef AMIGA_SC_6_1
     if (IsInteractive(Input()) == DOSTRUE) interactive = TRUE;
     else interactive = FALSE;
#else
#if defined(__MSC__) && defined(_Windows)
     interactive = TRUE;
#else
     interactive = isatty(fileno(stdin));
#endif
#endif
     if (argc > 1)
	  interactive = noinputfiles = FALSE;
     else
	  noinputfiles = TRUE;

     if (interactive)
	  show_version();
#ifdef vms   /* initialise screen management routines for command recall */
          if (status[1] = smg$create_virtual_keyboard(&vms_vkid) != SS$_NORMAL)
               done(status[1]);
#endif

	if (!setjmp(env)) {
	    /* first time */
	    interrupt_setup();
	    load_rcfile();

	    if (interactive && term != 0)	/* not unknown */
		 fprintf(stderr, "\nTerminal type set to '%s'\n", 
			    term_tbl[term].name);
	} else {	
	    /* come back here from int_error() */
	    load_file_error();	/* if we were in load_file(), cleanup */
#ifdef _Windows
	SetCursor(LoadCursor((HINSTANCE)NULL, IDC_ARROW));
#endif
#ifdef vms
	    /* after catching interrupt */
	    /* VAX stuffs up stdout on SIGINT while writing to stdout,
		  so reopen stdout. */
	    if (outfile == stdout) {
		   if ( (stdout = freopen("SYS$OUTPUT","w",stdout))  == NULL) {
			  /* couldn't reopen it so try opening it instead */
			  if ( (stdout = fopen("SYS$OUTPUT","w"))  == NULL) {
				 /* don't use int_error here - causes infinite loop! */
				 fprintf(stderr,"Error opening SYS$OUTPUT as stdout\n");
			  }
		   }
		   outfile = stdout;
	    }
#endif					/* VMS */
	    if (!interactive && !noinputfiles) {
			if (term && term_init)
				(*term_tbl[term].reset)();
#ifdef vms
			vms_reset();
#endif
			return(IO_ERROR);	/* exit on non-interactive error */
 		}
	}

     if (argc > 1) {
	    /* load filenames given as arguments */
	    while (--argc > 0) {
		   ++argv;
		   c_token = NO_CARET; /* in case of file not found */
		   load_file(fopen(*argv,"r"), *argv);	
	    }
	} else {
	    /* take commands from stdin */
	    while(!com_line());
	}

	if (term && term_init)
		(*term_tbl[term].reset)();
#ifdef vms
	vms_reset();
#endif
    return(IO_SUCCESS);
}

#if defined(ATARI) && defined(__PUREC__)
#include <math.h>
int purec_matherr(struct exception *e)
{	char *c;
	switch (e->type) {
	    case DOMAIN:    c = "domain error"; break;
	    case SING  :    c = "argument singularity"; break;
	    case OVERFLOW:  c = "overflow range"; break;
	    case UNDERFLOW: c = "underflow range"; break;
	    default:		c = "(unknown error"; break;
	}
	fprintf(stderr, "math exception : %s\n", c);
	fprintf(stderr, "    name : %s\n", e->name);
	fprintf(stderr, "    arg 1: %e\n", e->arg1);
	fprintf(stderr, "    arg 2: %e\n", e->arg2);
	fprintf(stderr, "    ret  : %e\n", e->retval);
	return 1;
}
#endif

/* Set up to catch interrupts */
interrupt_setup()
{
#if defined (MSDOS) || defined(_Windows) || (defined (ATARI) && defined(__PUREC__)) || defined(DOS386)
#ifdef __PUREC__
	setmatherr(purec_matherr);
#endif
#if defined (__TURBOC__) || defined (__PUREC__)
#if !defined(DOSX286) && !defined(BROKEN_SIGINT)
		(void) signal(SIGINT, tc_interrupt);	/* go there on interrupt char */
#endif
#else
#ifdef __ZTC__
        (void) signal(SIGINT, ztc_interrupt);
#else
#ifdef __EMX__
		(void) signal(SIGINT, (void *)inter);	/* go there on interrupt char */
#else
#ifdef DJGPP
		(void) signal(SIGINT, (SignalHandler)inter);	/* go there on interrupt char */
#else
               (void) signal(SIGINT, inter);
#endif
#endif
#endif
#endif
#else /* MSDOS */
		(void) signal(SIGINT, inter);	/* go there on interrupt char */
#endif /* MSDOS */
}


/* Look for a gnuplot start-up file */
load_rcfile()
{
    register FILE *plotrc;
    char home[80]; 
    char rcfile[sizeof(PLOTRC)+80];

    /* Look for a gnuplot init file in . or home directory */
#ifdef vms
    (void) strcpy(home,HOME);
#else /* vms */
    char *tmp_home=getenv(HOME);
    char *p;	/* points to last char in home path, or to \0, if none */
    char c='\0';/* character that should be added, or \0, if none */


    if(tmp_home) {
    	strcpy(home,tmp_home);
	if( strlen(home) ) p = &home[strlen(home)-1];
	else		   p = home;
#if defined(MSDOS) || defined(ATARI) || defined( OS2 ) || defined(_Windows) || defined(DOS386)
	if( *p!='\\' && *p!='\0' ) c='\\';
#else
#if defined(AMIGA_AC_5)
	if( *p!='/' && *p!=':' && *p!='\0' ) c='/';
#else /* that leaves unix */
	c='/';
#endif
#endif
	if(c) {
	    if(*p) p++;
	    *p++=c;
	    *p='\0';
	}
    }
#endif /* vms */

#ifdef NOCWDRC
    /* inhibit check of init file in current directory for security reasons */
    {
#else
    (void) strcpy(rcfile, PLOTRC);
    plotrc = fopen(rcfile,"r");
    if (plotrc == (FILE *)NULL) {
#endif
#ifndef vms
	if( tmp_home ) {
#endif
	   (void) sprintf(rcfile, "%s%s", home, PLOTRC);
	   plotrc = fopen(rcfile,"r");
#ifndef vms
	} else
	    plotrc=NULL;
#endif
    }
    if (plotrc)
	 load_file(plotrc, rcfile);
}
