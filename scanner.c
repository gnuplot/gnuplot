/*
 *
 *    G N U P L O T  --  scanner.c
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
#include <ctype.h>
#include "plot.h"

extern BOOLEAN screen_ok;

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
 *		Current char	token should contain
 *     -------------    -----------------------
 *		1.  alpha		all following alpha-numerics
 *		2.  digit		0 or more following digits, 0 or 1 decimal point,
 *						  0 or more digits, 0 or 1 'e' or 'E',
 *						  0 or more digits.
 *		3.  ^,+,-,/		only current char
 *		    %,~,(,)
 *		    [,],;,:,
 *		    ?,comma
 *		4.  &,|,=,*		current char; also next if next is same
 *		5.  !,<,>		current char; also next if next is =
 *		6.  ", '		all chars up until matching quote
 *
 *		white space between tokens is ignored
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
		if (isalpha(expression[current])) {
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
		if (str[++count] == '-')
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

substitute()
{
	int_error("substitution not supported by MS-DOS!",t_num);
}

#else /* MSDOS */

substitute(str,max)			/* substitute output from ` ` */
char *str;
int max;
{
register char *last;
register int i,c;
register FILE *f;
FILE *popen();
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
  	if ((f = popen(pgm,"r")) == NULL)
  		os_error("popen failed",NO_CARET);
#endif /* vms */

	i = 0;
	while ((c = getc(f)) != EOF) {
		output[i++] = ((c == '\n') ? ' ' : c);	/* newlines become blanks*/
		if (i == max) {
			(void) pclose(f);
			int_error("substitution overflow", t_num);
		}
	}
	(void) pclose(f);
	if (i + strlen(last) > max)
		int_error("substitution overflowed rest of line", t_num);
	(void) strncpy(output+i,last+1,MAX_LINE_LEN-i);
									/* tack on rest of line to output */
	(void) strcpy(str,output);				/* now replace ` ` with output */
	screen_ok = FALSE;
}
#endif /* MS-DOS */
