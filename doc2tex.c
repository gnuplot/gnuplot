#ifndef lint
static char *RCSid = "$Id: doc2tex.c,v 3.26 1992/03/25 04:53:29 woo Exp woo $";
#endif

/*
 * doc2tex.c  -- program to convert Gnuplot .DOC format to LaTeX document
 * Also will work for VMS .HLP files. 
 * Modified by Russell Lang from hlp2ms.c by Thomas Williams 
 * Extended by David Kotz to support quotes ("), backquotes, tables.
 *
 * usage:  doc2tex < file.doc > file.tex
 *
 *   where file.doc is a Gnuplot .DOC file, and file.tex will be an
 *     article document suitable for printing with LaTeX.
 *
 * typical usage for GNUPLOT:
 *
 *   doc2tex < gnuplot.doc > gnuplot.tex 
 *   latex gnuplot.tex ; latex gnuplot.tex
 */

static char rcsid[] = "$Id: doc2tex.c,v 3.26 1992/03/25 04:53:29 woo Exp woo $";

#include <stdio.h>
#include <ctype.h>
#ifdef AMIGA_LC_5_1
#include <string.h>
#endif

#define MAX_NAME_LEN	256
#define MAX_LINE_LEN	256
#define TRUE 1
#define FALSE 0

typedef int boolean;

boolean intable = FALSE;
boolean verb = FALSE;

main()
{
	init(stdout);
	convert(stdin,stdout);
	finish(stdout);
	exit(0);
}


init(b)
FILE *b;
{
	(void) fputs("\\input{titlepage.tex}\n",b);
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
		  if (intable) {
			 (void) fputs("\\hline\n\\end{tabular}\n", b);
			 (void) fputs("\\end{center}\n",b);
			 intable = FALSE;
		  } else {
			 if (verb) {
				(void) fputs("\\end{verbatim}\n",b);
				verb=FALSE;
			 } 
			 (void) fputs("\n\\begin{center}\n", b);
			 (void) fputs("\\begin{tabular}{|ccl|} \\hline\n", b);
			 intable = TRUE;
		  }
		  /* ignore rest of line */
		  break;
	   }
	   case '#': {			/* latex table entry */
		  if (intable)
		    (void) fputs(line+1, b); /* copy directly */
		  else
		    fprintf(stderr, "error: # line found outside of table\n");
		  break;
	   }
	   case '%': {			/* troff table entry */
		  break;			/* ignore */
	   }
	   case '\n':			/* empty text line */
	   case ' ': {			/* normal text line */
		  if (intable)
		    break;		/* ignore while in table */
		  if (line[1] == ' ') {
			 /* verbatim mode */
			 if (!verb) {
				(void) fputs("\\begin{verbatim}\n",b);
				verb=TRUE;
			 }
			 (void) fputs(line+1,b); 
		  } else {
			 if (verb) {
				(void) fputs("\\end{verbatim}\n",b);
				verb=FALSE;
			 } 
			 if (line[0] == '\n')
			   puttex(line,b); /* handle totally blank line */
			 else
			   puttex(line+1,b);
		  }
		  break;
	   }
	   default: {
		  if (isdigit(line[0])) { /* start of section */
			 if (!intable)	/* ignore while in table */
			   section(line, b);
		  } else
		    fprintf(stderr, "unknown control code '%c' in column 1\n", 
				  line[0]);
		  break;
	   }
    }
}

/* process a line with a digit control char */
/* starts a new [sub]section */

section(line, b)
	char *line;
	FILE *b;
{
    static char string[MAX_LINE_LEN];
    int sh_i;

    if (verb) {
	   (void) fputs("\\end{verbatim}\n",b);
	   verb=FALSE;
    } 
#ifdef AMIGA_LC_5_1
    (void) sscanf(line,"%d",&sh_i);
    strcpy(string,strchr(line,' ')+1);
    {
      char *p;
      p = strchr(string,'\n');
      if (p != NULL) *p = '\0';
    }
#else
    (void) sscanf(line,"%d %[^\n]s",&sh_i,string);
#endif
    switch(sh_i)
	 {
		case 1: 
		(void) fprintf(b,"\\section{");
		break;
		case 2: 
		(void) fprintf(b,"\\section{");
		break;
		case 3:
		(void) fprintf(b,"\\subsection{");
		break;
		case 4: 
		(void) fprintf(b,"\\subsubsection{");
		break;
		default:
		case 5: 
		(void) fprintf(b,"\\paragraph{");
		break;
	 }
    if (islower(string[0]))
	 string[0] = toupper(string[0]);
    puttex(string,b);
    (void) fprintf(b,"}\n");
}

/* put text in string str to file while buffering special TeX characters */
puttex(str,file)
FILE *file;
register char *str;
{
register char ch;
static boolean inquote = FALSE;

	 while( (ch = *str++) != '\0') {
		 switch(ch) {
			 case '#':
			 case '$':
			 case '%':
			 case '&':
			 case '_':
			 case '{':
			 case '}':
				 (void) fputc('\\',file);
				 (void) fputc(ch,file);
				 break;
			 case '\\':
				 (void) fputs("$\\backslash$",file);
				 break;
			 case '~':
				 (void) fputs("\\~{\\ }",file);
				 break;
			 case '^':
				 (void) fputs("\\verb+^+",file);
				 break;
			 case '>':
			 case '<':
			 case '|':
				 (void) fputc('$',file);
				 (void) fputc(ch,file);
				 (void) fputc('$',file);
				 break;
			 case '"': 
				 /* peek at next character: if space, end of quote */
				 if (*str == NULL || isspace(*str) || ispunct(*str))
				   (void) fputs("''", file);
				 else
				   (void) fputs("``", file);
				 break;
			 case '`':	/* backquotes mean boldface */
				 if (inquote) {
					fputs("}", file);
					inquote = FALSE;
				 } else {
					fputs("{\\bf ", file);
					inquote = TRUE;
				 }
				 break;
			 default:
				 (void) fputc(ch,file);
				 break;
		 }
	 }
}


finish(b)
FILE *b;
{
	(void) fputs("\\end{document}\n",b);
}
