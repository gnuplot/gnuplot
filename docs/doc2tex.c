/*
 * $Id: doc2tex.c,v 1.21 1998/06/18 14:59:12 ddenholm Exp $
 *
 */

/* GNUPLOT - doc2tex.c */

/*[
 * Copyright 1986 - 1993, 1998   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the complete modified source code.  Modifications are to
 * be distributed as patches to the released version.  Permission to
 * distribute binaries produced by compiling modified sources is granted,
 * provided you
 *   1. distribute the corresponding source modifications from the
 *    released version in the form of a patch file along with the binaries,
 *   2. add special version identification to distinguish your version
 *    in addition to the base release version number,
 *   3. provide your name and address as the primary contact for the
 *    support of your modified version, and
 *   4. retain our contact information in regard to use of the base
 *    software.
 * Permission to distribute the released version of the source code along
 * with corresponding source modifications in the form of a patch file is
 * granted with same provisions 2 through 4 for binary distributions.
 *
 * This software is provided "as is" without express or implied warranty
 * to the extent permitted by applicable law.
]*/

/*
 * doc2tex.c  -- program to convert Gnuplot .DOC format to LaTeX document
 * Also will work for VMS .HLP files. 
 * Modified by Russell Lang from hlp2ms.c by Thomas Williams 
 * Extended by David Kotz to support quotes ("), backquotes, tables.
 * Extended by Jens Emmerich to handle '_', '---', paired single
 * quotes. Changed "-handling. Added pre/post-verbatim hooks.
 *
 *
 * usage:  doc2tex [file.doc [file.tex]]
 *
 *   where file.doc is a Gnuplot .DOC file, and file.tex will be an
 *     article document suitable for printing with LaTeX.
 *
 * typical usage for GNUPLOT:
 *
 *   doc2tex gnuplot.doc gnuplot.tex 
 *   latex gnuplot.tex ; latex gnuplot.tex
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ansichek.h"
#include "stdfn.h"


#define MAX_NAME_LEN	256
#define MAX_LINE_LEN	256
#define TRUE 1
#define FALSE 0

void init __PROTO(( FILE *b ));
void convert __PROTO(( FILE *a, FILE *b ));
void process_line __PROTO(( char *line, FILE *b ));
void section __PROTO(( char *line, FILE *b ));
void puttex __PROTO(( char *str, FILE *file ));
void finish __PROTO(( FILE *b ));


typedef int boolean;

boolean intable = FALSE;
boolean verb = FALSE;

#ifndef ALL_TERM_DOC
#define ALL_TERM_DOC
#endif
#define TERM_DRIVER_H
#include "termdoc.c"

int
main(argc,argv)
	int argc;
	char **argv;
{
	FILE * infile;
	FILE * outfile;

	infile = stdin;
	outfile = stdout;
	if (argc > 3) {
		fprintf(stderr,"Usage: %s [infile [outfile]]\n", argv[0]);
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
	
	init(outfile);
	convert(infile,outfile);
	finish(outfile);
	exit(0);
}


void init(b)
	FILE *b;
{
	(void) fputs("\\input{titlepag.tex}\n",b);
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
 				(void) fputs("\\postverbatim\n",b);
				verb=FALSE;
			 } 
			 (void) fputs("\n\\begin{center}\n", b);
			 /* moved to gnuplot.doc by RCC
                         (void) fputs("\\begin{tabular}{|ccl|} \\hline\n", b);
                         */
			 intable = TRUE;
		  }
		  /* ignore rest of line */
		  break;
	   }
	   case '#': {			/* latex table entry */
		  if (intable)
		    (void) fputs(line+1, b); /* copy directly */
		  else{
		    fprintf(stderr, "warning: # line found outside of table\n");
			fprintf(stderr, "%s\n",line+1);}

		  break;
	   }
	   case '^': {			/* external link escape */
		  break;			/* ignore */
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
 				(void) fputs("\\preverbatim\n",b); 
				(void) fputs("\\begin{verbatim}\n",b);
				verb=TRUE;
			 }
			 (void) fputs(line+2,b); 
		  } else {
			 if (verb) {
				(void) fputs("\\end{verbatim}\n",b);
 				(void) fputs("\\postverbatim\n",b);
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

void section(line, b)
	char *line;
	FILE *b;
{
    static char string[MAX_LINE_LEN];
    int sh_i;

    if (verb) {
	   (void) fputs("\\end{verbatim}\n",b);
	   (void) fputs("\\postverbatim\n",b);
	   verb=FALSE;
    } 
    (void) sscanf(line,"%d %[^\n]s",&sh_i,string);
    switch(sh_i)
	 {
		case 1: 
		(void) fprintf(b,"\\part{");
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
		case 5: 
		(void) fprintf(b,"\\subsubsubsection{");
		break;
		default:
		case 6: 
		(void) fprintf(b,"\\paragraph{");
		break;
	 }
    if (islower(string[0]))
	 string[0] = toupper(string[0]);
    puttex(string,b);
    (void) fprintf(b,"}\n");
}

/* put text in string str to file while buffering special TeX characters */
void puttex(str,file)
	FILE *file;
	register char *str;
{
	register char ch;
	static boolean inquote = FALSE;
        int i;

	 while( (ch = *str++) != '\0') {
		 switch(ch) {
			 case '#':
			 case '$':
			 case '%':
			 case '&':
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
			         (void) fputs("{\\tt\"}", file);
			         break;
		         case '\'':
			         if (*str == '\'') {
				     (void) fputs("{'\\,'}", file);
				     str++;
				 } else {
				     (void) fputc(ch, file);
				 }
				 break;
		         case '-':
			         if ((*str == '-') && (*(str+1) == '-')) {
				     (void) fputs(" --- ", file);
				     str += 2;
				 } else {
				     (void) fputc(ch, file);
				 }
				 break;
			 case '`':	/* backquotes mean boldface */
				 if (inquote) {
					(void) fputs("}", file);
					inquote = FALSE;
				 } else {
					(void) fputs("{\\bf ", file);
					inquote = TRUE;
				 }
				 break;
		         case '_':      /* emphasised text ? */
			     for(i=0; isalpha(*(str+i)); i++) {};
                             if((i>0)&&(*(str+i)=='_')&&isspace(*(str+i+1))) {
				 (void) fputs("{\\em ", file);
				 for(; *str != '_'; str++) {
				     (void) fputc(*str, file);
				 }
				 str++;
				 (void) fputs("\\/}", file);
			     } else {
				 (void) fputs("{\\tt\\_}", file);
			     }
			 break;
		         default:
				 (void) fputc(ch,file);
				 break;
		 }
	 }
}


void finish(b)
	FILE *b;
{
	(void) fputs("\\end{document}\n",b);
}

