/*
 * $Id: doc2html.c,v 1.10 1998/04/14 00:16:58 drd Exp $
 *
 */

/* GNUPLOT - doc2html.c */

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
 * doc2html.c  -- program to convert Gnuplot .DOC format to 
 *        World Wide Web (WWW) HyperText Markup Language (HTML) format
 *
 * Created by Russell Lang from doc2ipf by Roger Fearick from 
 * doc2rtf by M Castro from doc2gih by Thomas Williams.
 * 1994-11-03
 *
 * usage:  doc2html gnuplot.doc gnuplot.htm
 *
 */

/* note that tables must begin in at least the second column to */
/* be formatted correctly and tabs are forbidden */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ansichek.h"
#include "stdfn.h"
#include "xref.h"

int debug = FALSE;
char title[256];

void convert __PROTO((FILE *, FILE *));
void process_line __PROTO((char *, FILE *));

/* Replase the previous #ifdef */
int single_top_level = 0;

/* We are using the fgets() replacement from termdoc.c */
extern char *get_line __PROTO((char *, int, FILE *));
/* From xref.c */
extern struct LIST *lookup __PROTO((char *));

int main(argc, argv)
int argc;
char **argv;
{
    FILE *infile;
    FILE *outfile;

    if (argv[argc - 1][0] == '-' && argv[argc - 1][1] == 'd') {
	debug = TRUE;
	argc--;
    }
    if ((argc > 3) || (argc == 1)) {
	fprintf(stderr, "Usage: %s infile outfile\n", argv[0]);
	exit(EXIT_FAILURE);
    }
    if ((infile = fopen(argv[1], "r")) == (FILE *) NULL) {
	fprintf(stderr, "%s: Can't open %s for reading\n",
		argv[0], argv[1]);
	exit(EXIT_FAILURE);
    }
    if (argc == 3) {
	if ((outfile = fopen(argv[2], "w")) == (FILE *) NULL) {
	    fprintf(stderr, "%s: Can't open %s for writing\n",
		    argv[0], argv[2]);
	}
	strncpy(title, argv[2], sizeof(title));
    } else {
	outfile = stdout;
	strncpy(title, argv[1], sizeof(title));
    }
    strtok(title, ".");		/* remove type */
    parse(infile);
    convert(infile, outfile);
    exit(EXIT_SUCCESS);
}

void convert(a, b)
FILE *a, *b;
{
    static char line[MAX_LINE_LEN+1];

    /* generate html header */
    fprintf(b, "<HTML>\n\
<HEAD>\n\
<TITLE>%s</TITLE>\n\
</HEAD>\n\
<BODY>\n\
<H1>%s</H1><P>\n", title, title);

    /* process each line of the file */
    while (get_line(line, sizeof(line), a)) {
	process_line(line, b);
    }

    /* close final page and generate trailer */
    fprintf(b, "\n<P><HR>Created automatically by doc2html\n\
</BODY>\n\
</HTML>\n");

    list_free();
}

void process_line(line, b)
char *line;
FILE *b;
{
    static int line_count = 0;
    static char line2[MAX_LINE_LEN+1];
    static int last_line;
    char hyplink1[64];
    char *pt, *tablerow;
    int i, j;
    static int startpage = 1;
    char str[MAX_LINE_LEN+1];
    char topic[MAX_LINE_LEN+1];
    int k, l;
    struct LIST *klist;
    static int tabl = 0;
    static int para = 0;
    static int inquote = FALSE;
    static int inref = FALSE;

    line_count++;

    i = j = 0;

    while (line[i] != NUL) {
	switch (line[i]) {
	case '<':
	    strcpy(&line2[j], "&lt;");
	    j += strlen("&lt.") - 1;
	    break;

	case '>':
	    strcpy(&line2[j], "&gt;");
	    j += strlen("&gt.") - 1;
	    break;

	case '&':
	    strcpy(&line2[j], "&amp;");
	    j += strlen("&amp;") - 1;
	    break;

	case '\r':
	case '\n':
	    break;
	case '`':		/* backquotes mean boldface or link */
	    if ((!inref) && (!inquote)) {
		k = i + 1;	/* index into current string */
		l = 0;		/* index into topic string */
		while ((line[k] != '`') && (line[k] != '\0')) {
		    topic[l] = line[k];
		    k++;
		    l++;
		}
		topic[l] = NUL;
		klist = lookup(topic);
		if (klist && ((k = klist->line) != last_line)) {
		    sprintf(hyplink1, "<A HREF=\042#%d\042>", k);
		    strcpy(line2 + j, hyplink1);
		    j += strlen(hyplink1) - 1;

		    inref = k;
		} else {
		    if (debug && k != last_line)
			fprintf(stderr, "Can't make link for \042%s\042 on line %d\n", topic, line_count);
		    line2[j++] = '<';
		    line2[j++] = 'B';
		    line2[j] = '>';
		    inquote = TRUE;
		}
	    } else {
		if (inquote && inref)
		    fprintf(stderr, "Warning: Reference Quote conflict line %d\n", line_count);
		if (inquote) {
		    line2[j++] = '<';
		    line2[j++] = '/';
		    line2[j++] = 'B';
		    line2[j] = '>';
		    inquote = FALSE;
		}
		if (inref) {
		    /* must be inref */
		    line2[j++] = '<';
		    line2[j++] = '/';
		    line2[j++] = 'A';
		    line2[j] = '>';
		    inref = 0;
		}
	    }
	    break;
	default:
	    line2[j] = line[i];
	}
	i++;
	j++;
	line2[j] = '\0';
    }

    i = 1;

    switch (line[0]) {		/* control character */
    case '?':{			/* interactive help entry */
#ifdef FIXLATER
	    if (intable)
		intablebut = TRUE;
	    fprintf(b, "\n:i1. %s", &(line[1]));	/* index entry */
#endif
	    break;
	}
    case '@':{			/* start/end table */
	    break;		/* ignore */
	}
    case '#':{			/* latex table entry */
	    break;		/* ignore */
	}
    case '^':{			/* external link escape */
	    (void) fputs(line + 1, b);	/* copy directly */
	    break;		/* ignore */
	}
    case '%':{			/* troff table entry */
	    break;		/* ignore */
	}
    case '\n':			/* empty text line */
	if (tabl)
	    fprintf(b, "</PRE>\n");	/* rjl */
	para = 0;
	tabl = 0;
	fprintf(b, "<P>");
	break;
    case ' ':{			/* normal text line */
	    if ((line2[1] == NUL) || (line2[1] == '\n')) {
		fprintf(b, "<P>");
		para = 0;
		tabl = 0;
	    }
	    if (line2[1] == ' ') {
		if (!tabl) {
		    fprintf(b, "<PRE>\n");
		}
		fprintf(b, "%s\n", &line2[1]);
		tabl = 1;
		para = 0;
	    } else {
		if (tabl) {
		    fprintf(b, "</PRE>\n");	/* rjl */
		    fprintf(b, "<P>");	/* rjl */
		}
		tabl = 0;
		if (!para)
		    para = 1;	/* not in para so start one */
		fprintf(b, "%s \n", &line2[1]);
	    }
	    break;
	}
    default:{
	    if (isdigit(line[0])) {	/* start of section */
		if (tabl)
		    fprintf(b, "</PRE>\n");	/* rjl */
		if (startpage) {	/* use the new level 0 */
		    refs(0, b, "<P>\n", "<P>\n", "<A HREF=\042#%d\042>%s</A><BR>\n");
		} else {
		    refs(last_line, b, "<P>\n", "<P>\n", "<A HREF=\042#%d\042>%s</A><BR>\n");
		}
		para = 0;	/* not in a paragraph */
		tabl = 0;
		last_line = line_count;
		startpage = 0;
		if (debug)
		    fprintf(stderr, "%d: %s\n", line_count, &line2[1]);
		klist = lookup(&line2[1]);
		if (klist)
		    k = klist->line;
		else
		    k = -1;
		/* output unique ID and section title */
		fprintf(b, "<HR><A NAME=\042%d\042>\n<H%c>", line_count, line[0] == '1' ? line[0] : line[0] - 1);
		fprintf(b, &(line2[1]));	/* title */
		fprintf(b, "</H%c>\n<P>", line[0] == '1' ? line[0] : line[0] - 1);
	    } else
		fprintf(stderr, "unknown control code '%c' in column 1, line %d\n",
			line[0], line_count);
	    break;
	}
    }
}
