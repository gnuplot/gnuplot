/*
 *
 *    G N U P L O T  --  misc.c
 *
 *  Copyright (C) 1986 Thomas Williams, Colin Kelley
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

extern struct curve_points plot[];
extern int c_token,next_value,next_function;
extern struct udft_entry udft[];
extern struct at_type *curr_at;
extern struct ft_entry ft[];
extern struct vt_entry vt[];


show_at(level)
int level;
{
struct at_type *at_address;
int i, j;
struct value *arg;

	at_address = curr_at;
	for (i = 0; i < at_address->count; i++) {
		for (j = 0; j < level; j++)
			(void) putc(' ',stderr);	/* indent */

			/* print name of action instruction */
		fputs(ft[at_address->actions[i].index].ft_name,stderr);
		arg = &(at_address->actions[i].arg);
			/* now print optional argument */

		switch(at_address->actions[i].index) {
		  case (int)PUSH:	fprintf(stderr," (%s)\n",
					  vt[arg->v.int_val].vt_name);
					break;
		  case (int)PUSHC:	(void) putc('(',stderr);
					show_value(stderr,arg);
					fputs(")\n",stderr);
					break;
		  case (int)PUSHD:	fprintf(stderr," (%s dummy)\n",
					  udft[arg->v.int_val].udft_name);
					break;
		  case (int)CALL:	fprintf(stderr," (%s)\n",
					  udft[arg->v.int_val].udft_name);
					curr_at = &udft[arg->v.int_val].at;
					show_at(level+2); /* recurse! */
					curr_at = at_address;
					break;
		  default:
					(void) putc('\n',stderr);
		}
	}
}
