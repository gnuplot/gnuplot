/* GNUPLOT - parse.c */
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
#include <errno.h>
#include <math.h>
#include "plot.h"

#ifndef vms
#ifndef __ZTC__
extern int errno;
#endif
#endif

extern int num_tokens,c_token;
extern struct lexical_unit token[];
extern char c_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];	/* name of current dummy vars */
extern struct udft_entry *dummy_func;	/* pointer to dummy variable's func */

struct value *pop(),*integer(),*complex();
struct at_type *temp_at(), *perm_at();
struct udft_entry *add_udf();
struct udvt_entry *add_udv();
union argument *add_action();

struct at_type at;
static jmp_buf fpe_env;

#define dummy (struct value *) 0

#ifdef __TURBOC__
void fpe()
#else
#if defined( __ZTC__ ) || defined( _CRAY )
void fpe(an_int)
int an_int;
#else
#ifdef NEXT
void fpe(int an_int)
#else
fpe()
#endif
#endif /* __ZTC__ || _CRAY */
#endif
{
#ifdef PC	/* thanks to lotto@wjh12.UUCP for telling us about this  */
	_fpreset();
#endif
	(void) signal(SIGFPE, fpe);
	undefined = TRUE;
	longjmp(fpe_env, TRUE);
}


#ifdef apollo
#include <apollo/base.h>
#include <apollo/pfm.h>
#include <apollo/fault.h>

/*
  On an Apollo, the OS can signal a couple errors that are not mapped
  into SIGFPE, namely signalling NaN and branch on an unordered
  comparison.  I suppose there are others, but none of these are documented,
  so I handle them as they arise.

  Anyway, we need to catch these faults and signal SIGFPE.
*/

pfm_$fh_func_val_t apollo_sigfpe(pfm_$fault_rec_t& fault_rec)
{
    kill(getpid(), SIGFPE);
    return pfm_$continue_fault_handling;
}

apollo_pfm_catch()
{
    status_$t status;
    pfm_$establish_fault_handler(fault_$fp_bsun, pfm_$fh_backstop,
				 apollo_sigfpe, &status);
    pfm_$establish_fault_handler(fault_$fp_sig_nan, pfm_$fh_backstop,
				 apollo_sigfpe, &status);
}
#endif


evaluate_at(at_ptr,val_ptr)
struct at_type *at_ptr;
struct value *val_ptr;
{
	double temp, real();

	undefined = FALSE;
	errno = 0;
	reset_stack();
	if (setjmp(fpe_env))
		return;				/* just bail out */
	(void) signal(SIGFPE, fpe);	/* catch core dumps on FPEs */

	execute_at(at_ptr);

	(void) signal(SIGFPE, SIG_DFL);

	if (errno == EDOM || errno == ERANGE) {
		undefined = TRUE;
	} else {
		(void) pop(val_ptr);
		check_stack();
	}
/* At least one machine (ATT 3b1) computes Inf without a SIGFPE */
	temp = real(val_ptr);
	if (temp > VERYLARGE || temp < -VERYLARGE) {
		undefined = TRUE;
	}
}


struct value *
const_express(valptr)
struct value *valptr;
{
register int tkn = c_token;
	if (END_OF_COMMAND)
		int_error("constant expression required",c_token);
	evaluate_at(temp_at(),valptr);	/* run it and send answer back */
	if (undefined) {
		int_error("undefined value",tkn);
	}
	return(valptr);
}


struct at_type *
temp_at()	/* build a static action table and return its pointer */
{
	at.a_count = 0;		/* reset action table !!! */
	express();
	return(&at);
}


/* build an action table, put it in dynamic memory, and return its pointer */

struct at_type *
perm_at()
{
register struct at_type *at_ptr;
register unsigned int len;

	(void) temp_at();
	len = sizeof(struct at_type) -
		(MAX_AT_LEN - at.a_count)*sizeof(struct at_entry);
	at_ptr = (struct at_type *) alloc(len, "action table");
     (void) memcpy(at_ptr,&at,len);
	return(at_ptr);
}


#ifdef NOCOPY
/*
 * cheap and slow version of memcpy() in case you don't have one
 */
memcpy(dest,src,len)
char *dest,*src;
unsigned int len;
{
	while (len--)
		*dest++ = *src++;
}
#endif /* NOCOPY */


express()  /* full expressions */
{
	xterm();
	xterms();
}

xterm()  /* ? : expressions */
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

	if (equals(c_token,"(")) {
		c_token++;
		express();
		if (!equals(c_token,")"))
			int_error("')' expected",c_token);
		c_token++;
	}
	else if (isnumber(c_token)) {
		convert(&(add_action(PUSHC)->v_arg),c_token);
		c_token++;
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
				(void) add_action(value);
			}
			else {
				int call_type = (int )CALL;
				value = c_token;
				c_token += 2;
				express();
				if (equals(c_token, ",")) {
					c_token += 1;
					express();
					call_type = (int )CALL2;
				}
				if (!equals(c_token,")"))
					int_error("')' expected",c_token);
				c_token++;
				add_action(call_type)->udf_arg = add_udf(value);
			}
		}
		else {
			if (equals(c_token,c_dummy_var[0])) {
			    c_token++;
			    add_action(PUSHD1)->udf_arg = dummy_func;
			}
			else if (equals(c_token,c_dummy_var[1])) {
			    c_token++;
			    add_action(PUSHD2)->udf_arg = dummy_func;
			}
			else {
			    add_action(PUSH)->udv_arg = add_udv(c_token);
			    c_token++;
			}
		}
	} /* end if letter */
	else
		int_error("invalid expression ",c_token);

	/* add action code for ! (factorial) operator */
	while (equals(c_token,"!")) {
		c_token++;
		(void) add_action(FACTORIAL);
	}
	/* add action code for ** operator */
	if (equals(c_token,"**")) {
			c_token++;
			unary();
			(void) add_action(POWER);
	}

}



xterms()
{  /* create action code for ? : expressions */

	if (equals(c_token,"?")) {
		register int savepc1, savepc2;
		register union argument *argptr1,*argptr2;
		c_token++;
		savepc1 = at.a_count;
		argptr1 = add_action(JTERN);
		express();
		if (!equals(c_token,":"))
			int_error("expecting ':'",c_token);
		c_token++;
		savepc2 = at.a_count;
		argptr2 = add_action(JUMP);
		argptr1->j_arg = at.a_count - savepc1;
		express();
		argptr2->j_arg = at.a_count - savepc2;
	}
}


aterms()
{  /* create action codes for || operator */

	while (equals(c_token,"||")) {
		register int savepc;
		register union argument *argptr;
		c_token++;
		savepc = at.a_count;
		argptr = add_action(JUMPNZ);	/* short-circuit if already TRUE */
		aterm();
		argptr->j_arg = at.a_count - savepc;/* offset for jump */
#if defined(AMIGA_LC_5_1) || defined(AMIGA_AC_5)
		(void) add_action(ABOOL);
#else
		(void) add_action(BOOL);
#endif
	}
}


bterms()
{ /* create action code for && operator */

	while (equals(c_token,"&&")) {
		register int savepc;
		register union argument *argptr;
		c_token++;
		savepc = at.a_count;
		argptr = add_action(JUMPZ);	/* short-circuit if already FALSE */
		bterm();
		argptr->j_arg = at.a_count - savepc;/* offset for jump */
#if defined(AMIGA_LC_5_1) || defined(AMIGA_AC_5)
		(void) add_action(ABOOL);
#else
		(void) add_action(BOOL);
#endif
	}
}


cterms()
{ /* create action code for | operator */

	while (equals(c_token,"|")) {
		c_token++;
		cterm();
		(void) add_action(BOR);
	}
}


dterms()
{ /* create action code for ^ operator */

	while (equals(c_token,"^")) {
		c_token++;
		dterm();
		(void) add_action(XOR);
	}
}


eterms()
{ /* create action code for & operator */

	while (equals(c_token,"&")) {
		c_token++;
		eterm();
		(void) add_action(BAND);
	}
}


fterms()
{ /* create action codes for == and != operators */

	while (TRUE) {
		if (equals(c_token,"==")) {
			c_token++;
			fterm();
			(void) add_action(EQ);
		}
		else if (equals(c_token,"!=")) {
			c_token++;
			fterm();
			(void) add_action(NE);
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
			(void) add_action(GT);
		}
		else if (equals(c_token,"<")) {
			c_token++;
			gterm();
			(void) add_action(LT);
		}		
		else if (equals(c_token,">=")) {
			c_token++;
			gterm();
			(void) add_action(GE);
		}
		else if (equals(c_token,"<=")) {
			c_token++;
			gterm();
			(void) add_action(LE);
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
				(void) add_action(PLUS);
			}
			else if (equals(c_token,"-")) {
				c_token++;
				hterm();
				(void) add_action(MINUS);
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
				(void) add_action(MULT);
			}
			else if (equals(c_token,"/")) {
				c_token++;
				unary();
				(void) add_action(DIV);
			}
			else if (equals(c_token,"%")) {
				c_token++;
				unary();
				(void) add_action(MOD);
			}
			else break;
	}
}


unary()
{ /* add code for unary operators */
	if (equals(c_token,"!")) {
		c_token++;
		unary();
		(void) add_action(LNOT);
	}
	else if (equals(c_token,"~")) {
		c_token++;
		unary();
		(void) add_action(BNOT);
	}
	else if (equals(c_token,"-")) {
		c_token++;
		unary();
		(void) add_action(UMINUS);
	}
	else
		factor();
}
