/* GNUPLOT - util.c */
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

#include <ctype.h>
#include <setjmp.h>
#include <stdio.h>
#include <errno.h>
#include "plot.h"

BOOLEAN screen_ok;
	/* TRUE if command just typed; becomes FALSE whenever we
		send some other output to screen.  If FALSE, the command line
		will be echoed to the screen before the ^ error message. */

#ifndef vms
#ifndef __ZTC__
extern int errno;
extern int sys_nerr;
extern char *sys_errlist[];
#endif
#endif /* vms */

extern char input_line[];
extern struct lexical_unit token[];
extern jmp_buf env;	/* from plot.c */
extern int inline_num;		/* from command.c */
extern BOOLEAN interactive;	/* from plot.c */
extern char *infile_name;	/* from plot.c */

extern char *strchr();

#ifndef AMIGA_AC_5
extern double sqrt(), atan2();
#endif

/*
 * chr_in_str() compares the characters in the string of token number t_num
 * with c, and returns TRUE if a match was found.
 */
chr_in_str(t_num, c)
int t_num;
char c;
{
register int i;

	if (!token[t_num].is_token)
		return(FALSE);				/* must be a value--can't be equal */
	for (i = 0; i < token[t_num].length; i++) {
		if (input_line[token[t_num].start_index+i] == c)
			return(TRUE);
		}
	return FALSE;
}


/*
 * equals() compares string value of token number t_num with str[], and
 *   returns TRUE if they are identical.
 */
equals(t_num, str)
int t_num;
char *str;
{
register int i;

	if (!token[t_num].is_token)
		return(FALSE);				/* must be a value--can't be equal */
	for (i = 0; i < token[t_num].length; i++) {
		if (input_line[token[t_num].start_index+i] != str[i])
			return(FALSE);
		}
	/* now return TRUE if at end of str[], FALSE if not */
	return(str[i] == '\0');
}



/*
 * almost_equals() compares string value of token number t_num with str[], and
 *   returns TRUE if they are identical up to the first $ in str[].
 */
almost_equals(t_num, str)
int t_num;
char *str;
{
register int i;
register int after = 0;
register start = token[t_num].start_index;
register length = token[t_num].length;

	if (!token[t_num].is_token)
		return(FALSE);				/* must be a value--can't be equal */
	for (i = 0; i < length + after; i++) {
		if (str[i] != input_line[start + i]) {
			if (str[i] != '$')
				return(FALSE);
			else {
				after = 1;
				start--;	/* back up token ptr */
				}
			}
		}

	/* i now beyond end of token string */

	return(after || str[i] == '$' || str[i] == '\0');
}



isstring(t_num)
int t_num;
{
	
	return(token[t_num].is_token &&
		   (input_line[token[t_num].start_index] == '\'' ||
		   input_line[token[t_num].start_index] == '\"'));
}


isnumber(t_num)
int t_num;
{
	return(!token[t_num].is_token);
}


isletter(t_num)
int t_num;
{
	return(token[t_num].is_token &&
			(isalpha(input_line[token[t_num].start_index])));
}


/*
 * is_definition() returns TRUE if the next tokens are of the form
 *   identifier =
 *		-or-
 *   identifier ( identifer ) =
 */
is_definition(t_num)
int t_num;
{
	return (isletter(t_num) &&
			(equals(t_num+1,"=") ||			/* variable */
			(equals(t_num+1,"(") &&		/* function */
			 isletter(t_num+2)   &&
			 equals(t_num+3,")") &&
			 equals(t_num+4,"=") ) ||
			(equals(t_num+1,"(") &&		/* function with */
			 isletter(t_num+2)   &&		/* two variables */
			 equals(t_num+3,",") &&
			 isletter(t_num+4)   &&
			 equals(t_num+5,")") &&
			 equals(t_num+6,"=") )
		));
}



/*
 * copy_str() copies the string in token number t_num into str, appending
 *   a null.  No more than MAX_ID_LEN chars are copied.
 */
copy_str(str, t_num)
char str[];
int t_num;
{
register int i = 0;
register int start = token[t_num].start_index;
register int count;

	if ((count = token[t_num].length) > MAX_ID_LEN)
		count = MAX_ID_LEN;
	do {
		str[i++] = input_line[start++];
		} while (i != count);
	str[i] = '\0';
}


/*
 * quote_str() does the same thing as copy_str, except it ignores the
 *   quotes at both ends.  This seems redundant, but is done for
 *   efficency.
 */
quote_str(str, t_num)
char str[];
int t_num;
{
register int i = 0;
register int start = token[t_num].start_index + 1;
register int count;

	if ((count = token[t_num].length - 2) > MAX_ID_LEN)
		count = MAX_ID_LEN;
	if (count>0) {
		do {
			str[i++] = input_line[start++];
			} while (i != count);
	}
	str[i] = '\0';
}


/*
 * quotel_str() does the same thing as quote_str, except it uses
 * MAX_LINE_LEN instead of MAX_ID_LEN. 
 */ 
quotel_str(str, t_num) 
char str[]; 
int t_num; 
{
register int i = 0;
register int start = token[t_num].start_index + 1;
register int count;

	if ((count = token[t_num].length - 2) > MAX_LINE_LEN)
		count = MAX_LINE_LEN;
	if (count>0) {
		do {
			str[i++] = input_line[start++];
			} while (i != count);
	}
	str[i] = '\0';
}


/*
 *	capture() copies into str[] the part of input_line[] which lies between
 *	the begining of token[start] and end of token[end].
 */
capture(str,start,end)
char str[];
int start,end;
{
register int i,e;

	e = token[end].start_index + token[end].length;
	for (i = token[start].start_index; i < e && input_line[i] != '\0'; i++)
		*str++ = input_line[i];
	*str = '\0';
}


/*
 *	m_capture() is similar to capture(), but it mallocs storage for the
 *  string.
 */
m_capture(str,start,end)
char **str;
int start,end;
{
register int i,e;
register char *s;

	if (*str)		/* previous pointer to malloc'd memory there */
		free(*str);
	e = token[end].start_index + token[end].length;
	*str = alloc((unsigned int)(e - token[start].start_index + 1), "string");
     s = *str;
     for (i = token[start].start_index; i < e && input_line[i] != '\0'; i++)
	  *s++ = input_line[i];
     *s = '\0';
}


/*
 *	m_quote_capture() is similar to m_capture(), but it removes
	quotes from either end if the string.
 */
m_quote_capture(str,start,end)
char **str;
int start,end;
{
register int i,e;
register char *s;

	if (*str)		/* previous pointer to malloc'd memory there */
		free(*str);
	e = token[end].start_index + token[end].length-1;
	*str = alloc((unsigned int)(e - token[start].start_index + 1), "string");
     s = *str;
    for (i = token[start].start_index + 1; i < e && input_line[i] != '\0'; i++)
	 *s++ = input_line[i];
    *s = '\0';
}


convert(val_ptr, t_num)
struct value *val_ptr;
int t_num;
{
	*val_ptr = token[t_num].l_val;
}

static char *num_to_str(r)
double r;
{
	static i = 0;
	static char s[4][20];
	int j = i++;

	if ( i > 3 ) i = 0;

	sprintf( s[j], "%g", r );
	if ( strchr( s[j], '.' ) == NULL &&
	     strchr( s[j], 'e' ) == NULL &&
	     strchr( s[j], 'E' ) == NULL )
		strcat( s[j], ".0" );

	return s[j];
} 

disp_value(fp,val)
FILE *fp;
struct value *val;
{
	switch(val->type) {
		case INT:
			fprintf(fp,"%d",val->v.int_val);
			break;
		case CMPLX:
			if (val->v.cmplx_val.imag != 0.0 )
				fprintf(fp,"{%s, %s}",
					num_to_str(val->v.cmplx_val.real),
					num_to_str(val->v.cmplx_val.imag));
			else
				fprintf(fp,"%s",
					num_to_str(val->v.cmplx_val.real));
			break;
		default:
			int_error("unknown type in disp_value()",NO_CARET);
	}
}


double
real(val)		/* returns the real part of val */
struct value *val;
{
	switch(val->type) {
		case INT:
			return((double) val->v.int_val);
		case CMPLX:
			return(val->v.cmplx_val.real);
	}
	int_error("unknown type in real()",NO_CARET);
	/* NOTREACHED */
	return((double)0.0);
}


double
imag(val)		/* returns the imag part of val */
struct value *val;
{
	switch(val->type) {
		case INT:
			return(0.0);
		case CMPLX:
			return(val->v.cmplx_val.imag);
	}
	int_error("unknown type in imag()",NO_CARET);
	/* NOTREACHED */
	return((double)0.0);
}



double
magnitude(val)		/* returns the magnitude of val */
struct value *val;
{
	switch(val->type) {
		case INT:
			return((double) abs(val->v.int_val));
		case CMPLX:
			return(sqrt(val->v.cmplx_val.real*
				    val->v.cmplx_val.real +
				    val->v.cmplx_val.imag*
				    val->v.cmplx_val.imag));
	}
	int_error("unknown type in magnitude()",NO_CARET);
	/* NOTREACHED */
	return((double)0.0);
}



double
angle(val)		/* returns the angle of val */
struct value *val;
{
	switch(val->type) {
		case INT:
			return((val->v.int_val > 0) ? 0.0 : Pi);
		case CMPLX:
			if (val->v.cmplx_val.imag == 0.0) {
				if (val->v.cmplx_val.real >= 0.0)
					return(0.0);
				else
					return(Pi);
			}
			return(atan2(val->v.cmplx_val.imag,
				     val->v.cmplx_val.real));
	}
	int_error("unknown type in angle()",NO_CARET);
	/* NOTREACHED */
	return((double)0.0);
}


struct value *
complex(a,realpart,imagpart)
struct value *a;
double realpart, imagpart;
{
	a->type = CMPLX;
	a->v.cmplx_val.real = realpart;
	a->v.cmplx_val.imag = imagpart;
	return(a);
}


struct value *
integer(a,i)
struct value *a;
int i;
{
	a->type = INT;
	a->v.int_val = i;
	return(a);
}



os_error(str,t_num)
char str[];
int t_num;
{
#ifdef vms
static status[2] = {1, 0};		/* 1 is count of error msgs */
#endif

register int i;

	/* reprint line if screen has been written to */

	if (t_num != NO_CARET) {		/* put caret under error */
		if (!screen_ok)
			fprintf(stderr,"\n%s%s\n", PROMPT, input_line);

		for (i = 0; i < sizeof(PROMPT) - 1; i++)
			(void) putc(' ',stderr);
		for (i = 0; i < token[t_num].start_index; i++) {
			(void) putc((input_line[i] == '\t') ? '\t' : ' ',stderr);
			}
		(void) putc('^',stderr);
		(void) putc('\n',stderr);
	}

	for (i = 0; i < sizeof(PROMPT) - 1; i++)
		(void) putc(' ',stderr);
	fprintf(stderr,"%s\n",str);

	for (i = 0; i < sizeof(PROMPT) - 1; i++)
		(void) putc(' ',stderr);
     if (!interactive)
	  if (infile_name != NULL)
	    fprintf(stderr,"\"%s\", line %d: ", infile_name, inline_num);
	  else
	    fprintf(stderr,"line %d: ", inline_num);


#ifdef vms
	status[1] = vaxc$errno;
	sys$putmsg(status);
	(void) putc('\n',stderr);
#else
#ifdef __ZTC__
	fprintf(stderr,"error number %d\n\n",errno);
#else
	if (errno >= sys_nerr)
		fprintf(stderr, "unknown errno %d\n\n", errno);
	else
		fprintf(stderr,"(%s)\n\n",sys_errlist[errno]);
#endif
#endif

	longjmp(env, TRUE);	/* bail out to command line */
}


int_error(str,t_num)
char str[];
int t_num;
{
register int i;

	/* reprint line if screen has been written to */

	if (t_num != NO_CARET) {		/* put caret under error */
		if (!screen_ok)
			fprintf(stderr,"\n%s%s\n", PROMPT, input_line);

		for (i = 0; i < sizeof(PROMPT) - 1; i++)
			(void) putc(' ',stderr);
		for (i = 0; i < token[t_num].start_index; i++) {
			(void) putc((input_line[i] == '\t') ? '\t' : ' ',stderr);
			}
		(void) putc('^',stderr);
		(void) putc('\n',stderr);
	}

	for (i = 0; i < sizeof(PROMPT) - 1; i++)
		(void) putc(' ',stderr);
     if (!interactive)
	  if (infile_name != NULL)
	    fprintf(stderr,"\"%s\", line %d: ", infile_name, inline_num);
	  else
	    fprintf(stderr,"line %d: ", inline_num);
     fprintf(stderr,"%s\n\n", str);

	longjmp(env, TRUE);	/* bail out to command line */
}

/* Lower-case the given string (DFK) */
/* Done in place. */
void
lower_case(s)
     char *s;
{
  register char *p = s;

  while (*p != '\0') {
    if (isupper(*p))
	 *p = tolower(*p);
    p++;
  }
}

/* Squash spaces in the given string (DFK) */
/* That is, reduce all multiple white-space chars to single spaces */
/* Done in place. */
void
squash_spaces(s)
     char *s;
{
  register char *r = s;		/* reading point */
  register char *w = s;		/* writing point */
  BOOLEAN space = FALSE;		/* TRUE if we've already copied a space */

  for (w = r = s; *r != '\0'; r++) {
	 if (isspace(*r)) {
		/* white space; only copy if we haven't just copied a space */
		if (!space) {
		    space = TRUE;
		    *w++ = ' ';
		}				/* else ignore multiple spaces */
	 } else {
		/* non-space character; copy it and clear flag */
		*w++ = *r;
		space = FALSE;
	 }
  }
  *w = '\0';				/* null terminate string */
}

