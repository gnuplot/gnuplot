#ifndef lint
static char *RCSid = "$Id: checkdoc.c,v 3.26 1992/03/25 04:53:29 woo Exp woo $";
#endif

/*
 * checkdoc -- check a doc file for correctness of first column. 
 *
 * Prints out lines that have an illegal first character.
 * First character must be space, digit, or ?, @, #, %, 
 * or line must be empty.
 *
 * usage: checkdoc < docfile
 * Modified by Russell Lang from hlp2ms.c by Thomas Williams 
 *
 * Original version by David Kotz used the following one line script!
 * sed -e '/^$/d' -e '/^[ 0-9?@#%]/d' gnuplot.doc
 *
 */

#include <stdio.h>
#include <ctype.h>

#define MAX_LINE_LEN	256
#define TRUE 1
#define FALSE 0

main()
{
	convert(stdin,stdout);
	exit(0);
}

convert(a,b)
	FILE *a,*b;
{
    static char line[MAX_LINE_LEN];

    while (fgets(line,MAX_LINE_LEN,a)) {
	   process_line(line, b);
    }
}

process_line(line, b)
	char *line;
	FILE *b;
{
    switch(line[0]) {		/* control character */
	   case '?': {			/* interactive help entry */
		  break;			/* ignore */
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
	   case '\n':			/* empty text line */
	   case ' ': {			/* normal text line */
		  break;
	   }
	   default: {
		  if (isdigit(line[0])) { /* start of section */
		  		/* ignore */
		  } else
			fputs(line,b);    /* output bad line */
		  break;
	   }
    }
}

