/*
 *
 *    gnutex/gnuplot translator  --  plot.c
 *
 * By David Kotz, 1990.
 * Department of Computer Science, Duke University, Durham, NC 27706.
 * Mail to dfk@cs.duke.edu.
 */

#include <stdio.h>
#include <setjmp.h>
#include <signal.h>
#include "plot.h"

extern FILE *infile;
extern FILE *outfile;

jmp_buf env;

struct value stack[STACK_DEPTH];

struct lexical_unit token[MAX_TOKENS];

struct value *integer(),*complex();

extern f_push(),f_pushc(),f_pushd(),f_call(),f_terniary(),f_lnot(),f_bnot(),
	f_uminus(),f_lor(),f_land(),f_bor(),f_xor(),f_band(),f_eq(),f_ne(),
	f_gt(),f_lt(),f_ge(),f_le(),f_plus(),f_minus(),f_mult(),f_div(),
	f_mod(),f_power();

extern f_real(),f_imag(),f_arg(),f_conjg(),f_sin(),f_cos(),f_tan(),f_asin(),
	f_acos(),f_atan(),f_sinh(),f_cosh(),f_tanh(),f_int(),f_abs(),f_sgn(),
	f_sqrt(),f_exp(),f_log10(),f_log(),f_besj0(),f_besj1(),f_besy0(),f_besy1(),
	f_floor(),f_ceil();
	

struct ft_entry ft[] = {	/* built-in function table */

/* internal functions: */
	{"push", f_push},	{"pushc", f_pushc},	{"pushd", f_pushd},
	{"call", f_call},	{"?:", f_terniary},	{"lnot", f_lnot},
	{"bnot", f_bnot},	{"uminus", f_uminus},	{"lor", f_lor},
	{"land", f_land},	{"bor", f_bor},		{"xor", f_xor},
	{"band", f_band},	{"eq", f_eq},		{"ne", f_ne},
	{"gt", f_gt},		{"lt", f_lt},		{"ge", f_ge},
	{"le", f_le},		{"plus", f_plus},	{"minus", f_minus},
	{"mult", f_mult},	{"div", f_div},		{"mod", f_mod},
	{"power", f_power},

/* standard functions: */
	{"real", f_real},	{"imag", f_imag},	{"arg", f_arg},
	{"conjg", f_conjg}, {"sin", f_sin},		{"cos", f_cos},
	{"tan", f_tan},		{"asin", f_asin},	{"acos", f_acos},
	{"atan", f_atan},	{"sinh", f_sinh},	{"cosh", f_cosh},
	{"tanh", f_tanh},	{"int", f_int},		{"abs", f_abs},
	{"sgn", f_sgn},		{"sqrt", f_sqrt},	{"exp", f_exp},
	{"log10", f_log10},	{"log", f_log},		{"besj0", f_besj0},
	{"besj1", f_besj1},	{"besy0", f_besy0},	{"besy1", f_besy1},
	{"floor", f_floor},	{"ceil", f_ceil}, 	{NULL, NULL}
};

struct udft_entry udft[MAX_UDFS+1];

struct vt_entry vt[MAX_VALUES] = {
	{"pi"},			{"xmin"},		{"xmax"},
	{"ymin"}, 		{"ymax"},		{"autoscale"}
};

struct st_entry st[MAX_STYLES] = {
    /* include the fixed styles by default */
    /* These must match the positions in enum PLOT_STYLE */
    {"lines"},	 	{"points"},		{"impulses"},		{"linespoints"},
    {"dots"}
};
int next_style = FIXED_STYLES+1;

catch()					/* interrupts */
{
	(void) signal(SIGFPE, SIG_DFL);	/* turn off FPE trapping */
	(void) fflush(outfile);
	(void) putc('\n',stderr);
	longjmp(env, TRUE);		/* return to prompt */
}


main(argc, argv)
	int argc;
	char **argv;
{
    setbuf(stderr,NULL);

    if (argc == 3) {
	   infile = fopen(argv[1], "r");
	   if (infile == (FILE *)NULL) {
		  fprintf(stderr, "Cannot open '%s' for input\n", argv[1]);
		  exit(1);
	   }
	   outfile = fopen(argv[2], "w");
	   if (outfile == (FILE *)NULL) {
		  fprintf(stderr, "Cannot open '%s' for output\n", argv[2]);
		  exit(1);
	   }
    } else {
	   fprintf(stderr, "usage: gnut2p infile outfile\n");
	   exit(2);
    }

    (void) complex(&vt[(int)C_PI].vt_value, Pi, 0.0);

    setjmp(env);

    /* setting to some of the old defaults */
    fprintf(outfile, "set noclip one; set noclip two; set clip points\n");

    while(TRUE)
	 com_line();
}
