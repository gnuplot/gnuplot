/* GNUPLOT - plot.c */
/*
 * Copyright (C) 1986, 1987, 1990, 1991   Thomas Williams, Colin Kelley
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
 * Send your comments or suggestions to 
 *  pixar!info-gnuplot@sun.com.
 * This is a mailing list; to join it send a note to 
 *  pixar!info-gnuplot-request@sun.com.  
 * Send bug reports to
 *  pixar!bug-gnuplot@sun.com.
 */

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include "plot.h"
#include "setshow.h"
#ifdef MSDOS
#include <io.h>
#endif
#ifdef vms
#include <unixio.h>
#include <smgdef.h>
extern int vms_vkid;
extern smg$create_virtual_keyboard();
unsigned int status[2] = {1, 0};
#endif
#ifdef AMIGA_LC_5_1
#include <proto/dos.h>
#endif

#ifdef __TURBOC__
#include <graphics.h>
#endif

extern char *getenv(),*strcat(),*strcpy(),*strncpy();

extern char input_line[];
extern int c_token;
extern FILE *outfile;
extern int term;

BOOLEAN interactive = TRUE;	/* FALSE if stdin not a terminal */
char *infile_name = NULL;	/* name of command file; NULL if terminal */

#ifndef STDOUT
#define STDOUT 1
#endif

jmp_buf env;

struct value *integer(),*complex();


extern f_push(),f_pushc(),f_pushd1(),f_pushd2(),f_call(),f_call2(),f_lnot(),f_bnot(),f_uminus()
	,f_lor(),f_land(),f_bor(),f_xor(),f_band(),f_eq(),f_ne(),f_gt(),f_lt(),
	f_ge(),f_le(),f_plus(),f_minus(),f_mult(),f_div(),f_mod(),f_power(),
	f_factorial(),f_bool(),f_jump(),f_jumpz(),f_jumpnz(),f_jtern();

extern f_real(),f_imag(),f_arg(),f_conjg(),f_sin(),f_cos(),f_tan(),f_asin(),
	f_acos(),f_atan(),f_sinh(),f_cosh(),f_tanh(),f_int(),f_abs(),f_sgn(),
	f_sqrt(),f_exp(),f_log10(),f_log(),f_besj0(),f_besj1(),f_besy0(),f_besy1(),
#ifdef GAMMA
	f_gamma(),
#endif
	f_floor(),f_ceil();


struct ft_entry ft[] = {	/* built-in function table */

/* internal functions: */
	{"push", f_push},	{"pushc", f_pushc},
	{"pushd1", f_pushd1},	{"pushd2", f_pushd2},
	{"call", f_call},	{"call2", f_call2},	{"lnot", f_lnot},
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
#ifdef GAMMA
 	{"gamma", f_gamma},
#endif
	{"floor", f_floor},	{"ceil", f_ceil},
	{NULL, NULL}
};

static struct udvt_entry udv_pi = {NULL, "pi",FALSE};
									/* first in linked list */
struct udvt_entry *first_udv = &udv_pi;
struct udft_entry *first_udf = NULL;



#ifdef vms

#define HOME "sys$login:"

#else /* vms */
#ifdef MSDOS

#define HOME "GNUPLOT"

#else /* MSDOS */

#if defined(AMIGA_AC_5) || defined(AMIGA_LC_5_1)

#define HOME "GNUPLOT"
#else /* AMIGA */

#define HOME "HOME"

#endif /* AMIGA */
#endif /* MSDOS */
#endif /* vms */

#ifdef unix
#define PLOTRC ".gnuplot"
#else /* unix */
#if defined(AMIGA_AC_5) || defined(AMIGA_LC_5_1)
#define PLOTRC ".gnuplot"
#else /* AMIGA */
#define PLOTRC "gnuplot.ini"
#endif /* AMIGA */
#endif /* unix */

#ifdef __TURBOC__
void tc_interrupt()
#else
#ifdef _CRAY
void inter(an_int)
int an_int;
#else
inter()
#endif
#endif
{
#ifdef MSDOS
#ifdef __TURBOC__
	(void) signal(SIGINT, tc_interrupt);
#else
	void ss_interrupt();
	(void) signal(SIGINT, ss_interrupt);
#endif
#else  /* MSDOS */
	(void) signal(SIGINT, inter);
#endif  /* MSDOS */
	(void) signal(SIGFPE, SIG_DFL);	/* turn off FPE trapping */
	if (term && term_init)
		(*term_tbl[term].text)();	/* hopefully reset text mode */
	(void) fflush(outfile);
	(void) putc('\n',stderr);
	longjmp(env, TRUE);		/* return to prompt */
}


main(argc, argv)
	int argc;
	char **argv;
{
/* Register the Borland Graphics Interface drivers. If they have been */
/* included by the linker.                                            */

#ifdef __TURBOC__
registerfarbgidriver(EGAVGA_driver_far);
registerfarbgidriver(CGA_driver_far);
registerfarbgidriver(Herc_driver_far);
registerfarbgidriver(ATT_driver_far);
#endif
#ifdef X11
     { int n = X11_args(argc, argv); argv += n; argc -= n; }
#endif 

#ifdef apollo
    apollo_pfm_catch();
#endif

	setbuf(stderr,(char *)NULL);
	outfile = stdout;
	(void) complex(&udv_pi.udv_value, Pi, 0.0);

     interactive = FALSE;
     init_terminal();		/* can set term type if it likes */

#ifdef AMIGA_LC_5_1
     if (IsInteractive(Input()) == DOSTRUE) interactive = TRUE;
     else interactive = FALSE;
#else
     interactive = isatty(fileno(stdin));
#endif
     if (argc > 1)
	  interactive = FALSE;

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
	    if (!interactive)
		 done(IO_ERROR);			/* exit on non-interactive error */
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
	    while(TRUE)
		 com_line();
	}

     done(IO_SUCCESS);
}

/* Set up to catch interrupts */
interrupt_setup()
{
#ifdef MSDOS
#ifdef __TURBOC__
		(void) signal(SIGINT, tc_interrupt);	/* go there on interrupt char */
#else
		void ss_interrupt();
		save_stack();				/* work-around for MSC 4.0/MSDOS 3.x bug */
		(void) signal(SIGINT, ss_interrupt);
#endif
#else /* MSDOS */
		(void) signal(SIGINT, inter);	/* go there on interrupt char */
#endif /* MSDOS */
}


/* Look for a gnuplot start-up file */
load_rcfile()
{
    register FILE *plotrc;
    static char home[80];
    static char rcfile[sizeof(PLOTRC)+80];

    /* Look for a gnuplot init file in . or home directory */
#ifdef vms
    (void) strcpy(home,HOME);
#else /* vms */
#if defined(AMIGA_AC_5) || defined(AMIGA_LC_5_1)
    strcpy(home,getenv(HOME));
    {
        int h;
        h = strlen(home) - 1;
        if (h >= 0) {
            if ((home[h] != ':') && (home[h] != '/')) {
                home[h] = '/';
                home[h+1] = '\0';
            }
       	}
    }
#else /* AMIGA */
    (void) strcat(strcpy(home,getenv(HOME)),"/");
#endif /* AMIGA */
#endif /* vms */
#ifdef NOCWDRC
    /* inhibit check of init file in current directory for security reasons */
    {
#else
    (void) strcpy(rcfile, PLOTRC);
    plotrc = fopen(rcfile,"r");
    if (plotrc == (FILE *)NULL) {
#endif
	   (void) sprintf(rcfile, "%s%s", home, PLOTRC);
	   plotrc = fopen(rcfile,"r");
    }
    if (plotrc)
	 load_file(plotrc, rcfile);
}
