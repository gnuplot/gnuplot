/*
 *
 *    G N U P L O T  --  parse.c
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
#include <setjmp.h>
#include <signal.h>
#include <errno.h>
#include "plot.h"

extern BOOLEAN undefined;

#ifndef vms
extern int errno;
#endif

extern int next_function,c_function /* next available space in udft */;
extern int num_tokens,c_token;
extern struct lexical_unit token[];
extern char dummy_var[];
extern struct at_type *curr_at;

struct value *pop(),*integer(),*complex();

static struct at_type temp_at;
static jmp_buf fpe_env;

#define dummy (struct value *) 0

fpe()
{
	(void) signal(SIGFPE, fpe);
	undefined = TRUE;
	longjmp(fpe_env, TRUE);
}

evaluate_at(at,valptr)
struct at_type *at;
struct value *valptr;
{
	undefined = FALSE;
	errno = 0;
	reset_stack();
	if (setjmp(fpe_env))
		return;				/* just bail out */
	(void) signal(SIGFPE, fpe);	/* catch core dumps on FPEs */

	execute_at(at);

	(void) signal(SIGFPE, SIG_DFL);

	if (errno == EDOM || errno == ERANGE) {
		undefined = TRUE;
	} else {
		(void) pop(valptr);
		check_stack();
	}
}


struct value *
const_express(valptr)
struct value *valptr;
{
register int tkn = c_token;
	if (END_OF_COMMAND)
		int_error("constant expression required",c_token);
	build_at(&temp_at);	/* make a temporary action table */
	evaluate_at(&temp_at,valptr);	/* run it and send answer back */
	if (undefined) {
		int_error("undefined value",tkn);
	}
	return(valptr);
}


build_at(at)	/* build full expressions */
struct at_type *at;
{
	curr_at = at;		/* set global variable */
	curr_at->count = 0;		/* reset action table !!! */
	express();
}


express()  /* full expressions */
{
	xterm();
	xterms();
}

xterm()  /* NEW!  ? : expressions */
{
	aterm();
	aterms();
}


aterm()
{
	bterm();
	bterms();
}


bterm()
{
	cterm();
	cterms();
}


cterm()
{
	dterm();
	dterms();
}


dterm()
{	
	eterm();
	eterms();
}


eterm()
{
	fterm();
	fterms();
}


fterm()
{
	gterm();
	gterms();
}


gterm()
{
	hterm();
	hterms();
}


hterm()
{
	unary(); /* - things */
	iterms(); /* * / % */
}


factor()
{
register int value;
struct value a, real_value;

	if (equals(c_token,"(")) {
		c_token++;
		express();
		if (!equals(c_token,")")) 
			int_error("')' expected",c_token);
		c_token++;
	}
	else if (isnumber(c_token)) {
		convert(&real_value,c_token);
		c_token++;
		add_action(PUSHC, &real_value);
	}
	else if (isletter(c_token)) {
		if ((c_token+1 < num_tokens)  && equals(c_token+1,"(")) {
		value = standard(c_token);
			if (value) {	/* it's a standard function */
				c_token += 2;
				express();
				if (!equals(c_token,")"))
					int_error("')' expected",c_token);
				c_token++;
				add_action(value,dummy);
			}
			else {
				value = user_defined(c_token);
				c_token += 2;
				express();
				if (!equals(c_token,")")) 
					int_error("')' expected",c_token);
				c_token++;
				add_action(CALL,integer(&a,value));
			}
		}
		else {
			if (equals(c_token,dummy_var)) {
				value = c_function;
				c_token++;
				add_action(PUSHD,integer(&a,value));
			}
			else {
				value = add_value(c_token);
				c_token++;
				add_action(PUSH,integer(&a,value));
			}
		}
	} /* end if letter */
	else
		int_error("invalid expression ",c_token);

	/* add action code for ** operator */
	if (equals(c_token,"**")) {
			c_token++;
			unary();
			add_action(POWER,dummy);
	}
}



xterms()
{  /* create action code for ? : expressions */

	while (equals(c_token,"?")) {
		c_token++;
		express();
		if (!equals(c_token,":")) 
			int_error("expecting ':'",c_token);
		c_token++;
		express();
		add_action(TERNIARY,dummy);
	}
}


aterms()
{  /* create action codes for || operator */

	while (equals(c_token,"||")) {
		c_token++;
		aterm();
		add_action(LOR,dummy);
	}
}


bterms()
{ /* create action code for && operator */

	while (equals(c_token,"&&")) {
		c_token++;
		bterm();
		add_action(LAND,dummy);
	}
}


cterms()
{ /* create action code for | operator */

	while (equals(c_token,"|")) {
		c_token++;
		cterm();
		add_action(BOR,dummy);
	}
}


dterms()
{ /* create action code for ^ operator */

	while (equals(c_token,"^")) {
		c_token++;
		dterm();
		add_action(XOR,dummy);
	}
}


eterms()
{ /* create action code for & operator */

	while (equals(c_token,"&")) {
		c_token++;
		eterm();
		add_action(BAND,dummy);
	}
}


fterms()
{ /* create action codes for == and != operators */

	while (TRUE) {
		if (equals(c_token,"==")) {
			c_token++;
			fterm();
			add_action(EQ,dummy);
		}
		else if (equals(c_token,"!=")) {
			c_token++;
			fterm(); 
			add_action(NE,dummy); 
		}
		else break;
	}
}


gterms()
{ /* create action code for < > >= or <= operators */
	
	while (TRUE) {
		/* I hate "else if" statements */
		if (equals(c_token,">")) {
			c_token++;
			gterm();
			add_action(GT,dummy);
		}
		else if (equals(c_token,"<")) {
			c_token++;
			gterm();
			add_action(LT,dummy);
		}		
		else if (equals(c_token,">=")) {
			c_token++;
			gterm();
			add_action(GE,dummy);
		}
		else if (equals(c_token,"<=")) {
			c_token++;
			gterm();
			add_action(LE,dummy);
		}
		else break;
	}

}



hterms()
{ /* create action codes for + and - operators */

	while (TRUE) {
			if (equals(c_token,"+")) {
				c_token++;
				hterm();
				add_action(PLUS,dummy);
			}
			else if (equals(c_token,"-")) {
				c_token++;
				hterm();
				add_action(MINUS,dummy);
			}
			else break;
	}
}


iterms()
{ /* add action code for * / and % operators */

	while (TRUE) {
			if (equals(c_token,"*")) {
				c_token++;
				unary();
				add_action(MULT,dummy);
			}
			else if (equals(c_token,"/")) {
				c_token++;
				unary();
				add_action(DIV,dummy);
			}
			else if (equals(c_token,"%")) {
				c_token++;
				unary();
				add_action(MOD,dummy);
			}
			else break;
	}
}


unary()
{ /* add code for unary operators */
	if (equals(c_token,"!")) {
		c_token++;
		unary();
		add_action(LNOT,dummy);
	}
	else if (equals(c_token,"~")) {
		c_token++;
		unary();
		add_action(BNOT,dummy);
	}
	else if (equals(c_token,"-")) {
		c_token++;
		unary();
		add_action(UMINUS,dummy);
	}
	else 
		factor();
}
