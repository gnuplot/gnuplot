/* GNUPLOT - scanner.c */
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
#include <ctype.h>
#include "plot.h"

#ifdef AMIGA_AC_5
#define O_RDONLY	0
int open(const char * _name, int _mode, ...);
int close(int);
#endif

#ifdef vms

#include stdio
#include descrip
#include errno

#define MAILBOX "PLOT$MAILBOX"
#define pclose(f) fclose(f)

#endif /* vms */


#define isident(c) (isalnum(c) || (c) == '_')

#ifndef STDOUT
#define STDOUT 1
#endif

#define LBRACE '{'
#define RBRACE '}'

#define APPEND_TOKEN {token[t_num].length++; current++;}

#define SCAN_IDENTIFIER while (isident(expression[current + 1]))\
				APPEND_TOKEN

extern struct lexical_unit token[MAX_TOKENS];

static int t_num;	/* number of token I'm working on */

char *strcat(), *strcpy(), *strncpy();

/*
 * scanner() breaks expression[] into lexical units, storing them in token[].
 *   The total number of tokens found is returned as the function value.
 *   Scanning will stop when '\0' is found in expression[], or when token[]
 *     is full.
 *
 *	 Scanning is performed by following rules:
 *
 *	Current char	token should contain
 *     -------------    -----------------------
 *	1.  alpha,_	all following alpha-numerics
 *	2.  digit	0 or more following digits, 0 or 1 decimal point,
 *				0 or more digits, 0 or 1 'e' or 'E',
 *				0 or more digits.
 *	3.  ^,+,-,/	only current char
 *	    %,~,(,)
 *	    [,],;,:,
 *	    ?,comma
 *	4.  &,|,=,*	current char; also next if next is same
 *	5.  !,<,>	current char; also next if next is =
 *	6.  ", '	all chars up until matching quote
 *	7.  #		this token cuts off scanning of the line (DFK).
 *
 *			white space between tokens is ignored
 */
scanner(expression)
char expression[];
{
register int current;	/* index of current char in expression[] */
register int quote;
char brace;

	for (current = t_num = 0;
	    t_num < MAX_TOKENS && expression[current] != '\0';
	    current++) {
again:
		if (isspace(expression[current]))
			continue;						/* skip the whitespace */
		token[t_num].start_index = current;
		token[t_num].length = 1;
		token[t_num].is_token = TRUE;	/* to start with...*/

		if (expression[current] == '`') {
			substitute(&expression[current],MAX_LINE_LEN - current);
			goto again;
		}
		/* allow _ to be the first character of an identifier */
		if (isalpha(expression[current]) || expression[current] == '_') {
			SCAN_IDENTIFIER;
		} else if (isdigit(expression[current]) || expression[current] == '.'){
			token[t_num].is_token = FALSE;
			token[t_num].length = get_num(&expression[current]);
			current += (token[t_num].length - 1);
		} else if (expression[current] == LBRACE) {
			token[t_num].is_token = FALSE;
			token[t_num].l_val.type = CMPLX;
			if ((sscanf(&expression[++current],"%lf , %lf %c",
				&token[t_num].l_val.v.cmplx_val.real,
				&token[t_num].l_val.v.cmplx_val.imag,
				&brace) != 3) || (brace != RBRACE))
					int_error("invalid complex constant",t_num);
			token[t_num].length += 2;
			while (expression[++current] != RBRACE) {
				token[t_num].length++;
				if (expression[current] == '\0')			/* { for vi % */
					int_error("no matching '}'", t_num);
			}
		} else if (expression[current] == '\'' || expression[current] == '\"'){
			token[t_num].length++;
			quote = expression[current];
			while (expression[++current] != quote) {
				if (!expression[current]) {
					expression[current] = quote;
					expression[current+1] = '\0';
					break;
				} else
					token[t_num].length++;
			}
		} else switch (expression[current]) {
		     case '#':		/* DFK: add comments to gnuplot */
		    	  goto endline; /* ignore the rest of the line */
			case '^':
			case '+':
			case '-':
			case '/':
			case '%':
			case '~':
			case '(':
			case ')':
			case '[':
			case ']':
			case ';':
			case ':':
			case '?':
			case ',':
				break;
			case '&':
			case '|':
			case '=':
			case '*':
				if (expression[current] == expression[current + 1])
					APPEND_TOKEN;
				break;
			case '!':
			case '<':
			case '>':
				if (expression[current + 1] == '=')
					APPEND_TOKEN;
				break;
			default:
				int_error("invalid character",t_num);
			}
		++t_num;	/* next token if not white space */
	}

endline:					/* comments jump here to ignore line */

/* Now kludge an extra token which points to '\0' at end of expression[].
   This is useful so printerror() looks nice even if we've fallen off the
   line. */

		token[t_num].start_index = current;
		token[t_num].length = 0;
	return(t_num);
}


get_num(str)
char str[];
{
double atof();
register int count = 0;
long atol();
register long lval;

	token[t_num].is_token = FALSE;
	token[t_num].l_val.type = INT;		/* assume unless . or E found */
	while (isdigit(str[count]))
		count++;
	if (str[count] == '.') {
		token[t_num].l_val.type = CMPLX;
		while (isdigit(str[++count]))	/* swallow up digits until non-digit */
			;
		/* now str[count] is other than a digit */
	}
	if (str[count] == 'e' || str[count] == 'E') {
		token[t_num].l_val.type = CMPLX;
/* modified if statement to allow + sign in exponent
   rjl 26 July 1988 */
		count++;
		if (str[count] == '-' || str[count] == '+')
			count++;
		if (!isdigit(str[count])) {
			token[t_num].start_index += count;
			int_error("expecting exponent",t_num);
		}
		while (isdigit(str[++count]))
			;
	}
	if (token[t_num].l_val.type == INT) {
 		lval = atol(str);
		if ((token[t_num].l_val.v.int_val = lval) != lval)
			int_error("integer overflow; change to floating point",t_num);
	} else {
		token[t_num].l_val.v.cmplx_val.imag = 0.0;
		token[t_num].l_val.v.cmplx_val.real = atof(str);
	}
	return(count);
}


#ifdef MSDOS

#ifdef __ZTC__
substitute(char *str,int max)
#else
substitute()
#endif
{
	int_error("substitution not supported by MS-DOS!",t_num);
}

#else /* MSDOS */
#ifdef AMIGA_LC_5_1
substitute()
{
	int_error("substitution not supported by AmigaDOS!",t_num);
}

#else /* AMIGA_LC_5_1 */

substitute(str,max)			/* substitute output from ` ` */
char *str;
int max;
{
register char *last;
register int i,c;
register FILE *f;
#ifdef AMIGA_AC_5
int fd;
#else
FILE *popen();
#endif
static char pgm[MAX_LINE_LEN+1],output[MAX_LINE_LEN+1];

#ifdef vms
int chan;
static $DESCRIPTOR(pgmdsc,pgm);
static $DESCRIPTOR(lognamedsc,MAILBOX);
#endif /* vms */

	i = 0;
	last = str;
	while (*(++last) != '`') {
		if (*last == '\0')
			int_error("unmatched `",t_num);
		pgm[i++] = *last;
	}
	pgm[i] = '\0';		/* end with null */
	max -= strlen(last);	/* max is now the max length of output sub. */
  
#ifdef vms
  	pgmdsc.dsc$w_length = i;
   	if (!((vaxc$errno = sys$crembx(0,&chan,0,0,0,0,&lognamedsc)) & 1))
   		os_error("sys$crembx failed",NO_CARET);
   
   	if (!((vaxc$errno = lib$spawn(&pgmdsc,0,&lognamedsc,&1)) & 1))
   		os_error("lib$spawn failed",NO_CARET);
   
   	if ((f = fopen(MAILBOX,"r")) == NULL)
   		os_error("mailbox open failed",NO_CARET);
#else /* vms */
#ifdef AMIGA_AC_5
  	if ((fd = open(pgm,"O_RDONLY")) == -1)
#else
  	if ((f = popen(pgm,"r")) == NULL)
#endif
  		os_error("popen failed",NO_CARET);
#endif /* vms */

	i = 0;
	while ((c = getc(f)) != EOF) {
		output[i++] = ((c == '\n') ? ' ' : c);	/* newlines become blanks*/
		if (i == max) {
#ifdef AMIGA_AC_5
			(void) close(fd);
#else
			(void) pclose(f);
#endif
			int_error("substitution overflow", t_num);
		}
	}
#ifdef AMIGA_AC_5
	(void) close(fd);
#else
	(void) pclose(f);
#endif
	if (i + strlen(last) > max)
		int_error("substitution overflowed rest of line", t_num);
	(void) strncpy(output+i,last+1,MAX_LINE_LEN-i);
									/* tack on rest of line to output */
	(void) strcpy(str,output);				/* now replace ` ` with output */
	screen_ok = FALSE;
}
#endif /* AMIGA_LC_5_1 */
#endif /* MS-DOS */
