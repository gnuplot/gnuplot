/*
 *
 *    G N U P L O T  --  eval.c
 *
 *  Copyright (C) 1986, 1987  Colin Kelley, Thomas Williams
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
#include "plot.h"

char *malloc();

extern int c_token;
extern struct ft_entry ft[];
extern struct udvt_entry *first_udv;
extern struct udft_entry *first_udf;
extern struct at_type at;
extern struct lexical_unit token[];

struct value *integer();



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

	if (!(*udv_ptr = (struct udvt_entry *)
		malloc((unsigned int)sizeof(struct udvt_entry))))
			int_error("not enought memory for value",t_num);
	(*udv_ptr)->next_udv = NULL;
	copy_str((*udv_ptr)->udv_name,t_num);
	(*udv_ptr)->udv_value.type = INT;	/* not necessary, but safe! */
	(*udv_ptr)->udv_undef = TRUE;
	return(*udv_ptr);
}


struct udft_entry *
add_udf(t_num)  /* find or add function and return pointer */
int t_num; /* index to token[] */
{
register struct udft_entry **udf_ptr = &first_udf;

	while (*udf_ptr) {
		if (equals(t_num,(*udf_ptr)->udf_name))
			return(*udf_ptr);
		udf_ptr = &((*udf_ptr)->next_udf);
	}
	if (!(*udf_ptr = (struct udft_entry *)
		malloc((unsigned int)sizeof(struct udft_entry))))
			int_error("not enought memory for function",t_num);
	(*udf_ptr)->next_udf = (struct udft_entry *) NULL;
	(*udf_ptr)->definition = NULL;
	(*udf_ptr)->at = NULL;
	copy_str((*udf_ptr)->udf_name,t_num);
	(void) integer(&((*udf_ptr)->dummy_value), 0);
	return(*udf_ptr);
}


union argument *
add_action(sf_index)
enum operators sf_index;		/* index of p-code function */
{
	if (at.a_count >= MAX_AT_LEN)
		int_error("action table overflow",NO_CARET);
	at.actions[at.a_count].index = sf_index;
	return(&(at.actions[at.a_count++].arg));
}


int standard(t_num)  /* return standard function index or 0 */
{
register int i;
	for (i = (int)SF_START; ft[i].f_name != NULL; i++) {
		if (equals(t_num,ft[i].f_name))
			return(i);
	}
	return(0);
}

 

execute_at(at_ptr)
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
