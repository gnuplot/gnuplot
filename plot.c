/*
 *
 *    G N U P L O T  --  plot.c
 *
 *  Copyright (C) 1986, 1987  Thomas Williams, Colin Kelley
 *
 *  You may use this code as you wish if credit is given and this message
 *  is retained.
 *
 *  Please e-mail any useful additions to vu-vlsi!plot so they may be
 *  included in later releases.
 *
 *  This file should be edited with 4-column tabs!  (:set ts=4 sw=4 in vi)
 */

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include "plot.h"

char *getenv(),*strcat(),*strcpy(),*strncpy();

extern char input_line[];
extern FILE *outfile;
extern int term;
extern struct termentry term_tbl[];

#ifndef STDOUT
#define STDOUT 1
#endif

jmp_buf env;

struct value *integer(),*complex();


extern f_push(),f_pushc(),f_pushd(),f_call(),f_lnot(),f_bnot(),f_uminus()
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
	{"push", f_push},	{"pushc", f_pushc},	{"pushd", f_pushd},
	{"call", f_call},	{"lnot", f_lnot},	{"bnot", f_bnot},
	{"uminus", f_uminus},					{"lor", f_lor},
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

#define HOME "HOME"

#endif /* MSDOS */
#endif /* vms */

#ifdef unix
#define PLOTRC ".gnuplot"
#else
#define PLOTRC "gnuplot.ini"
#endif

interrupt()
{
#ifdef MSDOS
	void ss_interrupt();
	(void) signal(SIGINT, ss_interrupt);
#else
	(void) signal(SIGINT, interrupt);
#endif
	(void) signal(SIGFPE, SIG_DFL);	/* turn off FPE trapping */
	if (term)
		(*term_tbl[term].text)();	/* hopefully reset text mode */
	(void) fflush(outfile);
	(void) putc('\n',stderr);
	longjmp(env, TRUE);		/* return to prompt */
}


main()
{
register FILE *plotrc;
register char *gnuterm;
static char home[sizeof(PLOTRC)+80];

	setbuf(stderr,(char *)NULL);
	outfile = fdopen(dup(STDOUT),"w");
	(void) complex(&udv_pi.udv_value, Pi, 0.0);

	show_version();

/* thanks to osupyr!alden (Dave Alden) for the GNUTERM code */

	if (!(gnuterm = getenv("GNUTERM")))
		gnuterm = TERM;
	(void) strcat(input_line,gnuterm); /* input_line already has "set term " */

	if (!setjmp(env))				/* come back here from printerror() */
		do_line();

	if (!setjmp(env)) {
#ifdef MSDOS
		void ss_interrupt();
		save_stack();				/* work-around for MSC 4.0/MSDOS 3.x bug */
		(void) signal(SIGINT, ss_interrupt);
#else /* MSDOS */
		(void) signal(SIGINT, interrupt);	/* go there on interrupt char */
#endif /* MSDOS */
		if (!(plotrc = (fopen(PLOTRC,"r")))) {
#ifdef vms
			(void) strncpy(home,HOME,sizeof(home));
			plotrc = fopen(strcat(home,PLOTRC),"r");
#else /* vms */
			(void) strcat(strncpy(home,getenv(HOME),sizeof(home)),"/");
			plotrc = fopen(strcat(home,PLOTRC),"r");
#endif /* vms */
		}
		if (plotrc)
			load_file(plotrc);
	}

loop:	com_line();
		goto loop;
}
