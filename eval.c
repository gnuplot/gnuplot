#ifndef lint
static char *RCSid = "$Id: eval.c,v 1.2 1998/11/03 12:52:12 lhecking Exp $";
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
    copy_str((*udv_ptr)->udv_name, t_num, MAX_ID_LEN);
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
	int_warn("Warning : udf shadowed by built-in function of the same name", t_num);

    /* create and return a new udf slot */

    *udf_ptr = (struct udft_entry *)
	gp_alloc(sizeof(struct udft_entry), "function");
    (*udf_ptr)->next_udf = (struct udft_entry *) NULL;
    (*udf_ptr)->definition = NULL;
    (*udf_ptr)->at = NULL;
    copy_str((*udf_ptr)->udf_name, t_num, MAX_ID_LEN);
    for (i = 0; i < MAX_NUM_VAR; i++)
	(void) Ginteger(&((*udf_ptr)->dummy_values[i]), 0);
    return (*udf_ptr);
}


int standard(t_num)		/* return standard function index or 0 */
int t_num;
{
    register int i;
    for (i = (int) SF_START; ft[i].f_name != NULL; i++) {
	if (equals(t_num, ft[i].f_name))
	    return (i);
    }
    return (0);
}



void execute_at(at_ptr)
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
