#ifndef lint
static char *RCSid = "$Id: doc2gih.c,v 3.26 92/03/25 04:53:29 woo Exp Locker: woo $";
#endif

/*
 * doc2gih.c  -- program to convert Gnuplot .DOC format to gnuplot
 * interactive help (.GIH) format.
 *
 * This involves stripping all lines with a leading digit or
 * a leading @, #, or %.
 * Modified by Russell Lang from hlp2ms.c by Thomas Williams 
 *
 * usage:  doc2gih < file.doc > file.gih
 *
 * Original version by David Kotz used the following one line script!
 * sed '/^[0-9@#%]/d' file.doc > file.gih
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
FILE * outfile;
	infile = stdin;
	outfile = stdout;
	if (argc > 3) {
		fprintf(stderr,"Usage: %s infile outfile\n", argv[0]);
		exit(1);
	}
	if (argc >= 2) 
		if ( (infile = fopen(argv[1],"r")) == (FILE *)NULL) {
			fprintf(stderr,"%s: Can't open %s for reading\n",
				argv[0], argv[1]);
			exit(1);
		}
	if (argc == 3)
		if ( (outfile = fopen(argv[2],"w")) == (FILE *)NULL) {
			fprintf(stderr,"%s: Can't open %s for writing\n",
				argv[0], argv[2]);
		}
	
	convert(infile,outfile);
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
		  (void) fputs(line,b); 
		  break;		
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
		  		/* ignore */
		  } else
		    fprintf(stderr, "unknown control code '%c' in column 1, line %d\n",
			    line[0], line_count);
		  break;
	   }
    }
}

