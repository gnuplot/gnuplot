/*
 *
 *    G N U P L O T  --  eval.c
 *
 *  Copyright (C) 1986 Colin Kelley, Thomas Williams
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

extern int c_token,next_value,next_function;
extern struct udft_entry udft[];
extern struct ft_entry ft[];
extern struct vt_entry vt[];
extern struct at_type *curr_at;
extern struct lexical_unit token[];

struct value *integer();



int add_value(t_num)
int t_num;
{
register int i;

	/* check if it's already in the table... */

	for (i = 0; i < next_value; i++) {
		if (equals(t_num,vt[i].vt_name))
			return(i);
	}
	if (next_value == MAX_VALUES)
		int_error("user defined constant space full",NO_CARET);
	copy_str(vt[next_value].vt_name,t_num);
	vt[next_value].vt_value.type = INT;		/* not necessary, but safe! */
	vt[next_value].vt_undef = TRUE;
	return(next_value++);
}


add_action(sf_index,arg)
enum operators sf_index;
struct value *arg;

 /* argument to pass to standard function indexed by sf_index */
{

	if ( curr_at->count >= MAX_AT_LEN ) 
		int_error("action table overflow",NO_CARET);
	curr_at->actions[curr_at->count].index = ((int)sf_index);
	if (arg != (struct value *)0)
		curr_at->actions[curr_at->count].arg = *arg;
	curr_at->count++;
}


int standard(t_num)  /* return standard function index or 0 */
{
register int i;
	for (i = (int)SF_START; ft[i].ft_name != NULL; i++) {
		if (equals(t_num,ft[i].ft_name))
			return(i);
	}
	return(0);
}



int user_defined(t_num)  /* find or add function and return index */
int t_num; /* index to token[] */
{
register int i;
	for (i = 0; i < next_function; i++) {
		if (equals(t_num,udft[i].udft_name))
			return(i);
	}
	if (next_function == MAX_UDFS)
		int_error("user defined function space full",t_num);
	copy_str(udft[next_function].udft_name,t_num);
	udft[next_function].definition[0] = '\0';
	udft[next_function].at.count = 0;
	(void) integer(&udft[next_function].dummy_value, 0);
	return(next_function++);
}

 

execute_at(at_ptr)
struct at_type *at_ptr;
{
register int i;
	for (i = 0; i < at_ptr->count; i++) {
		(*ft[at_ptr->actions[i].index].funct)(&(at_ptr->actions[i].arg));
	}
}

/*

 'ft' is a table containing C functions within this program. 

 An 'action_table' contains pointers to these functions and arguments to be
 passed to them. 

 at_ptr is a pointer to the action table which must be executed (evaluated)

 so the iterated line exectues the function indexed by the at_ptr and 
 passes the argument which is pointed to by the arg_ptr 

*/
