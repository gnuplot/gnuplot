#ifndef lint
static char *RCSid = "$Id: checkdoc.c%v 3.38.2.70 1993/02/08 02:19:29 woo Exp woo $";
#endif


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

#include <stdio.h>
#include <ctype.h>

#define MAX_LINE_LEN	256
#define TRUE 1
#define FALSE 0

main(argc,argv)
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
    static long line_no=0;

    line_no++;

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
			fprintf(b, "%ld:%s", line_no, line);    /* output bad line */
		  break;
	   }
    }
}

