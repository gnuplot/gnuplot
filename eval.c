#ifndef lint
static char *RCSid = "$Id: eval.c,v 1.14 1997/04/10 02:32:50 drd Exp $";
#endif


/* GNUPLOT - eval.c */
/*
 * Copyright (C) 1986 - 1993, 1997   Thomas Williams, Colin Kelley
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
 *	comp.graphics.apps.gnuplot 
 * is identical to the mailing list (they
 * both carry the same set of messages). We prefer that you read the
 * messages through that newsgroup, to subscribing to the mailing list.
 * (If you can read that newsgroup, and are already on the mailing list,
 * please send a message to majordomo@dartmouth.edu, asking to be
 * removed from the mailing list.)
 *
 * The address for mailing to list members is
 *	   info-gnuplot@dartmouth.edu
 * and for mailing administrative requests is 
 *	   majordomo@dartmouth.edu
 * The mailing list for bug reports is 
 *	   bug-gnuplot@dartmouth.edu
 * The list of those interested in beta-test versions is
 *	   info-gnuplot-beta@dartmouth.edu
 */

#include "plot.h"


struct udvt_entry *
add_udv(t_num)  /* find or add value and return pointer */
int t_num;
{
register struct udvt_entry **udv_ptr = &first_udv;

	/* check if it's already in the table... */

	while (*udv_ptr) {
		if (equals(t_num,(*udv_ptr)->udv_name))
			return(*udv_ptr);
		udv_ptr = &((*udv_ptr)->next_udv);
	}

	*udv_ptr = (struct udvt_entry *)
	  gp_alloc((unsigned long)sizeof(struct udvt_entry), "value");
	(*udv_ptr)->next_udv = NULL;
	copy_str((*udv_ptr)->udv_name,t_num, MAX_ID_LEN);
	(*udv_ptr)->udv_value.type = INTGR;	/* not necessary, but safe! */
	(*udv_ptr)->udv_undef = TRUE;
	return(*udv_ptr);
}


struct udft_entry *
add_udf(t_num)  /* find or add function and return pointer */
int t_num; /* index to token[] */
{
register struct udft_entry **udf_ptr = &first_udf;

	int i;
	while (*udf_ptr) {
		if (equals(t_num,(*udf_ptr)->udf_name))
			return(*udf_ptr);
		udf_ptr = &((*udf_ptr)->next_udf);
	}

     /* get here => not found. udf_ptr points at first_udf or
      * next_udf field of last udf
      */

     if (standard(t_num))
       int_warn("Warning : udf shadowed by built-in function of the same name", t_num);

     /* create and return a new udf slot */

     *udf_ptr = (struct udft_entry *)
	  gp_alloc((unsigned long)sizeof(struct udft_entry), "function");
	(*udf_ptr)->next_udf = (struct udft_entry *) NULL;
	(*udf_ptr)->definition = NULL;
	(*udf_ptr)->at = NULL;
	copy_str((*udf_ptr)->udf_name,t_num, MAX_ID_LEN);
	for(i=0; i<MAX_NUM_VAR; i++)
		(void) Ginteger(&((*udf_ptr)->dummy_values[i]), 0);
	return(*udf_ptr);
}


int standard(t_num)  /* return standard function index or 0 */
int t_num;
{
register int i;
	for (i = (int)SF_START; ft[i].f_name != NULL; i++) {
		if (equals(t_num,ft[i].f_name))
			return(i);
	}
	return(0);
}

 

void execute_at(at_ptr)
struct at_type *at_ptr;
{
register int i,index,count,offset;

	count = at_ptr->a_count;
	for (i = 0; i < count;) {
		index = (int)at_ptr->actions[i].index;
		offset = (*ft[index].func)(&(at_ptr->actions[i].arg));
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
