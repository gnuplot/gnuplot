/*
 * $Id: checkdoc.c,v 1.16 1997/07/22 23:24:18 drd Exp $
 *
 */

/* GNUPLOT - checkdoc.c */
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
 */

/*
 * checkdoc -- check a doc file for correctness of first column. 
 *
 * Prints out lines that have an illegal first character.
 * First character must be space, digit, or ?, @, #, %, 
 * or line must be empty.
 *
 * usage: checkdoc [docfile]
 * Modified by Russell Lang from hlp2ms.c by Thomas Williams 
 *
 * Original version by David Kotz used the following one line script!
 * sed -e '/^$/d' -e '/^[ 0-9?@#%]/d' gnuplot.doc
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <ctype.h>

#include "ansichek.h"

#ifndef NO_STDLIB_H
#include <stdlib.h>
#endif

#define MAX_LINE_LEN	256
#define TRUE 1
#define FALSE 0

void convert __PROTO(( FILE *a, FILE *b ));
void process_line __PROTO(( char *line, FILE *b ));

/* checkdoc should check all terminal driver help */
#define ALL_TERM

#include "termdoc.c"

int main(argc,argv)
int argc;
char **argv;
{
FILE * infile;
	infile = stdin;
	if (argc > 2) {
		fprintf(stderr,"Usage: %s [infile]\n", argv[0]);
		exit(1);
	}
	if (argc == 2) 
		if ( (infile = fopen(argv[1],"r")) == (FILE *)NULL) {
			fprintf(stderr,"%s: Can't open %s for reading\n",
				argv[0], argv[1]);
			exit(1);
		}

	convert(infile, stdout);
	return (0);
}

void convert(a,b)
	FILE *a,*b;
{
    static char line[MAX_LINE_LEN];

    while (fgets(line,MAX_LINE_LEN,a)) {
	   process_line(line, b);
    }
}

void process_line(line, b)
	char *line;
	FILE *b;
{
	/* check matching backticks within a paragraph */

	static int count = 0;

	if (line[0] == ' ')
	{	
		char *p = line;

		/* skip/count leading spaces */

		while (*++p == ' ')
			;

		if (*p=='\n')
		{
			/* it is not clear if this is an error, but it is an
			 * inconsistency, so flag it
			 */
			fprintf(b, "spaces-only line %s:%d\n", termdoc_filename, termdoc_lineno);
		}
		else
		{
			/* accumulate count of backticks. Do not check odd-ness
			 * until end of paragraph (non-space in column 1)
			 */
			for ( ; *p ; ++p)
				if (*p == '`')
					++count;
		}
	}
	else
	{
		if (count&1)
		{
			fprintf(b,
				"mismatching backticks before %s:%d\n",
				termdoc_filename, termdoc_lineno);
		}
		count=0;
	}

	if (strchr(line, '\t'))
		fprintf(b, "tab character in line %s:%d\n", termdoc_filename, termdoc_lineno);
	
    switch(line[0]) {		/* control character */
	   case '?': {			/* interactive help entry */
		  break;			/* ignore */
	   }
		case '<' : {      /* term docs */
			break;      /* ignore */
		}
	   case '@': {			/* start/end table */
		  break;			/* ignore */
	   }
	   case '#': {			/* latex table entry */
		  break;			/* ignore */
	   }
	   case '%': {			/* troff table entry */
		  break;			/* ignore */
	   }
	   case '^': {			/* html entry */
		  break;			/* ignore */
	   }
	   case '\n':			/* empty text line */
	   case ' ': {			/* normal text line */
			break;
	   }
	   default: {
		  if (isdigit(line[0])) { /* start of section */
		  		/* ignore */
		  } else
   			/* output bad line */
			fprintf(b, "%s:%d:%s", termdoc_filename, termdoc_lineno, line);
		  break;
	   }
    }
}

