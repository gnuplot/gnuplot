#ifndef lint
static char *RCSid = "$Id: doc2hlp.c,v 3.26 1992/03/25 04:53:29 woo Exp woo $";
#endif

/*
 * doc2hlp.c  -- program to convert Gnuplot .DOC format to 
 * VMS help (.HLP) format.
 *
 * This involves stripping all lines with a leading ?,
 * @, #, or %.
 * Modified by Russell Lang from hlp2ms.c by Thomas Williams 
 *
 * usage:  doc2hlp < file.doc > file.hlp
 *
 * Original version by David Kotz used the following one line script!
 * sed '/^[?@#%]/d' file.doc > file.hlp
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
    static int line_count = 0;

    line_count++;

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
		  (void) fputs(line,b); 
		  break;
	   }
	   default: {
		  if (isdigit(line[0])) { /* start of section */
			(void) fputs(line,b); 
		  } else
		    fprintf(stderr, "unknown control code '%c' in column 1, line %d\n", 
				  line[0], line_count);
		  break;
	   }
    }
}
