#ifndef lint
static char *RCSid() { return RCSid("$Id: eval.c,v 1.7 1999/10/01 14:54:30 lhecking Exp $"); }
#endif

/* GNUPLOT - eval.c */

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


#include "plot.h"
#include "alloc.h"
#include "datafile.h"
#include "eval.h"
#include "internal.h"
#include "parse.h"
#include "specfun.h"
#include "standard.h"
#include "util.h"

/* HBB 990829: the following was moved here, from plot.c, where is
 * wasn't used, anyway... */

struct udvt_entry udv_pi = { NULL, "pi", FALSE };
/* first in linked list */
struct udvt_entry *first_udv = &udv_pi;
struct udft_entry *first_udf = NULL;

/* The table of built-in functions */
struct ft_entry GPFAR ft[] =
{
    /* internal functions: */
    {"push", (FUNC_PTR) f_push},
    {"pushc", (FUNC_PTR) f_pushc},
    {"pushd1", (FUNC_PTR) f_pushd1},
    {"pushd2", (FUNC_PTR) f_pushd2},
    {"pushd", (FUNC_PTR) f_pushd},
    {"call", (FUNC_PTR) f_call},
    {"calln", (FUNC_PTR) f_calln},
    {"lnot", (FUNC_PTR) f_lnot},
    {"bnot", (FUNC_PTR) f_bnot},
    {"uminus", (FUNC_PTR) f_uminus},
    {"lor", (FUNC_PTR) f_lor},
    {"land", (FUNC_PTR) f_land},
    {"bor", (FUNC_PTR) f_bor},
    {"xor", (FUNC_PTR) f_xor},
    {"band", (FUNC_PTR) f_band},
    {"eq", (FUNC_PTR) f_eq},
    {"ne", (FUNC_PTR) f_ne},
    {"gt", (FUNC_PTR) f_gt},
    {"lt", (FUNC_PTR) f_lt},
    {"ge", (FUNC_PTR) f_ge},
    {"le", (FUNC_PTR) f_le},
    {"plus", (FUNC_PTR) f_plus},
    {"minus", (FUNC_PTR) f_minus},
    {"mult", (FUNC_PTR) f_mult},
    {"div", (FUNC_PTR) f_div},
    {"mod", (FUNC_PTR) f_mod},
    {"power", (FUNC_PTR) f_power},
    {"factorial", (FUNC_PTR) f_factorial},
    {"bool", (FUNC_PTR) f_bool},
    {"dollars", (FUNC_PTR) f_dollars},	/* for using extension */
    {"jump", (FUNC_PTR) f_jump},
    {"jumpz", (FUNC_PTR) f_jumpz},
    {"jumpnz", (FUNC_PTR) f_jumpnz},
    {"jtern", (FUNC_PTR) f_jtern},

/* standard functions: */
    {"real", (FUNC_PTR) f_real},
    {"imag", (FUNC_PTR) f_imag},
    {"arg", (FUNC_PTR) f_arg},
    {"conjg", (FUNC_PTR) f_conjg},
    {"sin", (FUNC_PTR) f_sin},
    {"cos", (FUNC_PTR) f_cos},
    {"tan", (FUNC_PTR) f_tan},
    {"asin", (FUNC_PTR) f_asin},
    {"acos", (FUNC_PTR) f_acos},
    {"atan", (FUNC_PTR) f_atan},
    {"atan2", (FUNC_PTR) f_atan2},
    {"sinh", (FUNC_PTR) f_sinh},
    {"cosh", (FUNC_PTR) f_cosh},
    {"tanh", (FUNC_PTR) f_tanh},
    {"int", (FUNC_PTR) f_int},
    {"abs", (FUNC_PTR) f_abs},
    {"sgn", (FUNC_PTR) f_sgn},
    {"sqrt", (FUNC_PTR) f_sqrt},
    {"exp", (FUNC_PTR) f_exp},
    {"log10", (FUNC_PTR) f_log10},
    {"log", (FUNC_PTR) f_log},
    {"besj0", (FUNC_PTR) f_besj0},
    {"besj1", (FUNC_PTR) f_besj1},
    {"besy0", (FUNC_PTR) f_besy0},
    {"besy1", (FUNC_PTR) f_besy1},
    {"erf", (FUNC_PTR) f_erf},
    {"erfc", (FUNC_PTR) f_erfc},
    {"gamma", (FUNC_PTR) f_gamma},
    {"lgamma", (FUNC_PTR) f_lgamma},
    {"ibeta", (FUNC_PTR) f_ibeta},
    {"igamma", (FUNC_PTR) f_igamma},
    {"rand", (FUNC_PTR) f_rand},
    {"floor", (FUNC_PTR) f_floor},
    {"ceil", (FUNC_PTR) f_ceil},

    {"norm", (FUNC_PTR) f_normal},	/* XXX-JG */
    {"inverf", (FUNC_PTR) f_inverse_erf},	/* XXX-JG */
    {"invnorm", (FUNC_PTR) f_inverse_normal},	/* XXX-JG */
    {"asinh", (FUNC_PTR) f_asinh},
    {"acosh", (FUNC_PTR) f_acosh},
    {"atanh", (FUNC_PTR) f_atanh},

    {"column", (FUNC_PTR) f_column},	/* for using */
    {"valid", (FUNC_PTR) f_valid},	/* for using */
    {"timecolumn", (FUNC_PTR) f_timecolumn},	/* for using */

    {"tm_sec", (FUNC_PTR) f_tmsec},	/* for timeseries */
    {"tm_min", (FUNC_PTR) f_tmmin},	/* for timeseries */
    {"tm_hour", (FUNC_PTR) f_tmhour},	/* for timeseries */
    {"tm_mday", (FUNC_PTR) f_tmmday},	/* for timeseries */
    {"tm_mon", (FUNC_PTR) f_tmmon},	/* for timeseries */
    {"tm_year", (FUNC_PTR) f_tmyear},	/* for timeseries */
    {"tm_wday", (FUNC_PTR) f_tmwday},	/* for timeseries */
    {"tm_yday", (FUNC_PTR) f_tmyday},	/* for timeseries */

    {NULL, NULL}
};


struct udvt_entry *
add_udv(t_num)			/* find or add value and return pointer */
int t_num;
{
    register struct udvt_entry **udv_ptr = &first_udv;

    /* check if it's already in the table... */

    while (*udv_ptr) {
	if (equals(t_num, (*udv_ptr)->udv_name))
	    return (*udv_ptr);
	udv_ptr = &((*udv_ptr)->next_udv);
    }

    *udv_ptr = (struct udvt_entry *)
	gp_alloc(sizeof(struct udvt_entry), "value");
    (*udv_ptr)->next_udv = NULL;
    (*udv_ptr)->udv_name = gp_alloc (token_len(t_num)+1, "user var");
    copy_str((*udv_ptr)->udv_name, t_num, token_len(t_num)+1);
    (*udv_ptr)->udv_value.type = INTGR;		/* not necessary, but safe! */
    (*udv_ptr)->udv_undef = TRUE;
    return (*udv_ptr);
}


struct udft_entry *
add_udf(t_num)			/* find or add function and return pointer */
int t_num;			/* index to token[] */
{
    register struct udft_entry **udf_ptr = &first_udf;

    int i;
    while (*udf_ptr) {
	if (equals(t_num, (*udf_ptr)->udf_name))
	    return (*udf_ptr);
	udf_ptr = &((*udf_ptr)->next_udf);
    }

    /* get here => not found. udf_ptr points at first_udf or
     * next_udf field of last udf
     */

    if (standard(t_num))
	int_warn(t_num, "Warning : udf shadowed by built-in function of the same name");

    /* create and return a new udf slot */

    *udf_ptr = (struct udft_entry *)
	gp_alloc(sizeof(struct udft_entry), "function");
    (*udf_ptr)->next_udf = (struct udft_entry *) NULL;
    (*udf_ptr)->definition = NULL;
    (*udf_ptr)->at = NULL;
    (*udf_ptr)->udf_name = gp_alloc (token_len(t_num)+1, "user func");
    copy_str((*udf_ptr)->udf_name, t_num, token_len(t_num)+1);
    for (i = 0; i < MAX_NUM_VAR; i++)
	(void) Ginteger(&((*udf_ptr)->dummy_values[i]), 0);
    return (*udf_ptr);
}


int
standard(t_num)		/* return standard function index or 0 */
int t_num;
{
    register int i;
    for (i = (int) SF_START; ft[i].f_name != NULL; i++) {
	if (equals(t_num, ft[i].f_name))
	    return (i);
    }
    return (0);
}



void
execute_at(at_ptr)
struct at_type *at_ptr;
{
    register int i, index, count, offset;

    count = at_ptr->a_count;
    for (i = 0; i < count;) {
	index = (int) at_ptr->actions[i].index;
	offset = (*ft[index].func) (&(at_ptr->actions[i].arg));
	if (is_jump(index))
	    i += offset;
	else
	    i++;
    }
}

/*

   'ft' is a table containing C functions within this program. 

   An 'action_table' contains pointers to these functions and arguments to be
   passed to them. 

   at_ptr is a pointer to the action table which must be executed (evaluated)

   so the iterated line exectues the function indexed by the at_ptr and 
   passes the address of the argument which is pointed to by the arg_ptr 

 */
