#ifndef lint
static char *RCSid = "$Id: doc2ms.c,v 3.26 1992/03/25 04:53:29 woo Exp woo $";
#endif

/*
 * doc2ms.c  -- program to convert Gnuplot .DOC format to *roff -ms document
 * From hlp2ms by Thomas Williams 
 *
 * Modified by Russell Lang, 2nd October 1989
 * to make vms help level 1 and 2 create the same ms section level.
 *
 * Modified to become doc2ms by David Kotz (David.Kotz@Dartmouth.edu) 12/89
 * Added table and backquote support.
 *
 * usage:  doc2ms < file.doc > file.ms
 *
 *   where file.doc is a VMS .DOC file, and file.ms will be a [nt]roff
 *     document suitable for printing with nroff -ms or troff -ms
 *
 * typical usage for GNUPLOT:
 *
 *   doc2ms < gnuplot.doc | troff -ms
 */

static char rcsid[] = "$Id: doc2ms.c,v 3.26 1992/03/25 04:53:29 woo Exp woo $";

#include <stdio.h>
#include <ctype.h>
#ifdef AMIGA_LC_5_1
#include <string.h>
#endif

#define MAX_NAME_LEN	256
#define MAX_LINE_LEN	256
#define LINE_SKIP		3

#define TRUE 1
#define FALSE 0

typedef int boolean;

static boolean intable = FALSE;

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
    /* in nroff, increase line length by 8 and don't adjust lines */
    (void) fputs(".if n \\{.nr LL +8m\n.na \\}\n",b);
    (void) fputs(".nr PO +0.3i\n",b);
    (void) fputs(".so titlepage.ms\n",b);
    (void) fputs(".pn 1\n",b);
    (void) fputs(".bp\n",b);
    (void) fputs(".ta 1.5i 3.0i 4.5i 6.0i 7.5i\n",b);
    (void) fputs("\\&\n.sp 3\n.PP\n",b);
    /* following line commented out by rjl
	  (void) fputs(".so intro\n",b);
	  */
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
			 (void) fputs(".TE\n.KE\n", b);
			 (void) fputs(".EQ\ndelim off\n.EN\n\n",b);
			 intable = FALSE;
		  } else {
			 (void) fputs("\n.EQ\ndelim $$\n.EN\n",b);
			 (void) fputs(".KS\n.TS\ncenter box tab (@) ;\n", b);
			 (void) fputs("c c l .\n", b);
			 intable = TRUE;
		  }
		  /* ignore rest of line */
		  break;
	   }
	   case '#': {			/* latex table entry */
		  break;			/* ignore */
	   }
	   case '%': {			/* troff table entry */
		  if (intable)
		    (void) fputs(line+1, b); /* copy directly */
		  else
		    fprintf(stderr, "error: % line found outside of table\n");
		  break;
	   }
	   case '\n':			/* empty text line */
	   case ' ': {			/* normal text line */
		  if (intable)
		    break;		/* ignore while in table */
		  switch(line[1]) {
			 case ' ': {
				/* verbatim mode */
				fputs(".br\n",b); 
				fputs(line+1,b); 
				fputs(".br\n",b);
				break;
			 }
			 case '\'': {
				fputs("\\&",b);
				putms(line+1,b); 
				break;
			 }
			 default: {
				if (line[0] == '\n')
				  putms(line,b); /* handle totally blank line */
				else
				  putms(line+1,b);
				break;
			 }
			 break;
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
    static int old = 1;

  
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
    
    (void) fprintf(b,".sp %d\n",(sh_i == 1) ? LINE_SKIP : LINE_SKIP-1);
    
    if (sh_i > old) {
	   do
		if (old!=1)	/* this line added by rjl */
		  (void) fputs(".RS\n.IP\n",b);
	   while (++old < sh_i);
    }
    else if (sh_i < old) {
	   do
			   if (sh_i!=1) /* this line added by rjl */
				(void) fputs(".RE\n.br\n",b);
	   while (--old > sh_i);
    }
    
    /* added by dfk to capitalize section headers */
    if (islower(string[0]))
	 string[0] = toupper(string[0]);
    
    /* next 3 lines added by rjl */
    if (sh_i!=1) 
	 (void) fprintf(b,".NH %d\n%s\n.sp 1\n.LP\n",sh_i-1,string);
    else 
	 (void) fprintf(b,".NH %d\n%s\n.sp 1\n.LP\n",sh_i,string);
    old = sh_i;
    
    (void) fputs(".XS\n",b);
    (void) fputs(string,b);
    (void) fputs("\n.XE\n",b);
}

putms(s, file)
	char *s;
	FILE *file;
{
    static boolean inquote = FALSE;

    while (*s != '\0') {
	   switch (*s) {
		  case '`': {		/* backquote -> boldface */
			 if (inquote) {
				fputs("\\fR", file);
				inquote = FALSE;
			 } else {
				fputs("\\fB", file);
				inquote = TRUE;
			 }
			 break;
		  }
		  case '\\': {		/* backslash */
			 fputs("\\\\", file);
			 break;
		  }
		  default: {
			 fputc(*s, file);
			 break;
		  }
	   }
	   s++;
    }
}

finish(b)		/* spit out table of contents */
FILE *b;
{
	(void) fputs(".pn 1\n",b);
	(void) fputs(".ds RH %\n",b);
	(void) fputs(".af % i\n",b);
	(void) fputs(".bp\n.PX\n",b);
}
