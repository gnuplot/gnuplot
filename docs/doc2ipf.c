#ifndef lint
static char *RCSid() { return RCSid("$Id: doc2ipf.c,v 1.14 2002/12/17 20:50:49 lhecking Exp $"); }
#endif

/* GNUPLOT - doc2ipf.c */

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
 * doc2ipf.c  -- program to convert Gnuplot .DOC format to OS/2
 * ipfc  (.inf/.hlp) format.
 *
 * Modified by Roger Fearick from doc2rtf by M Castro 
 *
 * usage:  doc2ipf gnuplot.doc gnuplot.ipf
 *
 */

/* note that tables must begin in at least the second column to */
/* be formatted correctly and tabs are forbidden */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "syscfg.h"
#include "stdfn.h"

#define MAX_LINE_LEN 1023

#include "doc2x.h"
#include "xref.h"

#define MAX_COL 6

/* From xref.c */
extern void *xmalloc __PROTO((size_t));

void convert __PROTO((FILE *, FILE *));
void process_line __PROTO((char *, FILE *));

/* malloc's are not being checked ! */

struct TABENTRY {		/* may have MAX_COL column tables */
    struct TABENTRY *next;
    char col[MAX_COL][256];
};

struct TABENTRY table;
struct TABENTRY *tableins = &table;
int tablecols = 0;
int tablewidth[MAX_COL] = {0, 0, 0, 0, 0, 0};	/* there must be the correct
	                        			   number of zeroes here */
int tablelines = 0;

#define TmpFileName "doc2ipf.tmp"
static TBOOLEAN debug = FALSE;


int
main (int argc, char **argv)
{
    FILE *infile;
    FILE *outfile;
    if (argc == 4 && argv[3][0] == '-' && argv[3][1] == 'd')
	debug = TRUE;

    if (argc != 3 && !debug) {
	fprintf(stderr, "Usage: %s infile outfile\n", argv[0]);
	exit(EXIT_FAILURE);
    }
    if ((infile = fopen(argv[1], "r")) == (FILE *) NULL) {
	fprintf(stderr, "%s: Can't open %s for reading\n",
		argv[0], argv[1]);
	exit(EXIT_FAILURE);
    }
    if ((outfile = fopen(argv[2], "w")) == (FILE *) NULL) {
	fprintf(stderr, "%s: Can't open %s for writing\n",
		argv[0], argv[2]);
	fclose(infile);
	exit(EXIT_FAILURE);
    }
    parse(infile);
    convert(infile, outfile);
    return EXIT_SUCCESS;
}

void
convert(FILE *a, FILE *b)
{
    static char line[MAX_LINE_LEN+1];

    /* generate ipf header */
    fprintf(b, ":userdoc.\n:prolog.\n");
    fprintf(b, ":title.GNUPLOT\n");
    fprintf(b, ":docprof toc=12345.\n:eprolog.\n");

    /* process each line of the file */
    while (get_line(line, sizeof(line), a)) {
	process_line(line, b);
    }

    /* close final page and generate trailer */
    fprintf(b, "\n:euserdoc.\n");

    list_free();
}

void
process_line(char *line, FILE *b)
{
    static int line_count = 0;
    static char line2[MAX_LINE_LEN+1];
    static int last_line;
    char hyplink1[64];
    char *pt, *tablerow;
    int i;
    int j;
    static int startpage = 1;
    char str[MAX_LINE_LEN+1];
    char topic[MAX_LINE_LEN+1];
    int k, l;
    static int tabl = 0;
    static int para = 0;
    static int inquote = FALSE;
    static int inref = FALSE;
    static int intable = FALSE;
    static int intablebut = FALSE;
    static int introffheader = FALSE;
    static char tablechar = '@';
    static FILE *bo = NULL, *bt = NULL;
    static char tabledelim[4] = "%@\n";
    static int nblanks = 0;
    struct LIST *klist;

    line_count++;

    if (debug && introffheader) {
	fprintf(stderr, "%s\n", line);
    }
    if (bo == NULL)
	bo = b;
    i = 0;
    j = 0;
    nblanks = 0;
    while (line[nblanks] == ' ')
	++nblanks;
    while (line[i] != NUL) {
	if (introffheader) {
	    if (line[i] != '\n')
		line2[j] = line[i];
	    else
		line2[j] = NUL;
	} else
	    switch (line[i]) {
	    case '$':
		if (intable && (tablechar != '$') && (line[0] == '%')) {
		    ++i;
		    if (line[i + 1] == '$' || line[i] == 'x' || line[i] == '|') {
			while (line[i] != '$')
			    line2[j++] = line[i++];
			--j;
		    } else {
			while (line[i] != '$')
			    i++;
			if (line[i + 1] == ',')
			    i++;
			if (line[i + 1] == ' ')
			    i++;
			line2[j] = line[++i];
		    }
		} else
		    line2[j] = line[i];
		break;

	    case ':':
		strcpy(&line2[j], "&colon.");
		j += strlen("&colon.") - 1;
		break;

	    case '&':
		/* real hack to solve \&_ in postscript doc tables */
		/* (which are a special case hack anyway. */
		if (j > 0 && line2[j - 1] == '\\') {
		    j -= 2;
		    break;
		}
		strcpy(&line2[j], "&amp.");
		j += strlen("&amp.") - 1;
		break;

	    case '\r':
	    case '\n':
		break;

	    case '`':		/* backquotes mean boldface or link */
		if (nblanks > 7) {
		    line2[j] = line[i];
		    break;
		}
		if ((!inref) && (!inquote)) {
		    k = i + 1;	/* index into current string */
		    l = 0;	/* index into topic string */
		    while ((line[k] != '`') && (line[k] != 0)) {
			topic[l] = line[k];
			k++;
			l++;
		    }
		    topic[l] = 0;
		    klist = lookup(topic);
		    if (klist != NULL && (k = klist->line) > 0) {
			sprintf(hyplink1, ":link reftype=hd res=%d.", k);
			strcpy(line2 + j, hyplink1);
			j += strlen(hyplink1) - 1;

			inref = k;
		    } else {
			if (debug)
			    fprintf(stderr, "Can't make link for \042%s\042 on line %d\n", topic, line_count);
			strcpy(line2 + j, ":hp2.");
			j += 4;
			inquote = TRUE;
		    }
		} else {
		    if (inquote && inref)
			fprintf(stderr, "Warning: Reference Quote conflict line %d\n", line_count);
		    if (inquote) {
			strcpy(line2 + j, ":ehp2.");
			j += 5;
			inquote = FALSE;
		    }
		    if (inref) {
			/* must be inref */
			strcpy(line2 + j, ":elink.");
			j += 6;
			inref = FALSE;
		    }
		}
		break;

	    case '.':
		/* Makes code less readable but fixes warnings like 
		   <..\docs\gnuplot.ipf:6546> Warning 204: Invalid macro [.gnuplot_iris4d]
		   which seems to be triggered by a '.' character in the first column?! */
		strcpy(line2+j, "&per.");
		j += 4;
		break;

	    default:
		line2[j] = line[i];
	    }
	i++;
	j++;
	if ((j >= sizeof(line2))) {
	    fprintf(stderr, "MAX_LINE_LEN exceeded\n");
	    if (inref || inquote)
		fprintf(stderr, "Possible missing link character (`) near above line number\n");
	    abort();
	}
	line2[j] = NUL;
    }

    i = 1;

    switch (line[0]) {		/* control character */
    case '?':{			/* interactive help entry */
	    if (intable)
		intablebut = TRUE;
	    break;
	}
    case '@':{			/* start/end table */
	    intable = !intable;
	    if (intable) {
		tablechar = '@';
		introffheader = FALSE;
		tablelines = 0;
		tablecols = 0;
		tableins = &table;
		for (j = 0; j < MAX_COL; j++)
		    tablewidth[j] = 0;
	    } else {		/* dump table */
		int header = 0;
		intablebut = FALSE;
		tableins = &table;
		fprintf(b, ":table cols=\'");
		for (j = 0; j < MAX_COL; j++)
		    if (tablewidth[j] > 0)
			fprintf(b, " %d", tablewidth[j]);
		fprintf(b, "\'.\n");
		tableins = tableins->next;
		if (tableins->next != NULL)
		    header = (tableins->next->col[0][0] == '_');
		if (header)
		    tableins->next = tableins->next->next;
		while (tableins != NULL) {
		    fprintf(b, ":row.\n");
		    for (j = 0; j < tablecols; j++)
			if (header)
			    fprintf(b, ":c.:hp9.%s:ehp9.\n", tableins->col[j]);
			else
			    fprintf(b, ":c.%s\n", tableins->col[j]);
		    tableins = tableins->next;
		    header = 0;
		}
		fprintf(b, ":etable.\n");
		if (bt != NULL) {
		    rewind(bt);
		    while (get_line(str, sizeof(str), bt))
			fputs(str, b);
		    fclose(bt);
		    remove(TmpFileName);
		    bt = NULL;
		    bo = b;
		}
	    }
	    break;
	}
    case '#':{			/* latex table entry */
	    break;		/* ignore */
	}
    case '%':{			/* troff table entry */
	    if (intable) {
		if (introffheader) {
		    if (debug) {
		       fprintf(stderr, ">%s\n", line2);
		       fprintf(stderr, "tablechar: %c\n", tablechar);
		    }
		    if (line2[1] == '.')
			break;
		    pt = strchr(line2, '(');
		    if (pt != NULL)
			tablechar = *(pt + 1);
		    if (debug) {
		       fprintf(stderr, "tablechar: %c\n", tablechar);
		    }
		    pt = strchr(line2 + 2, '.');
		    if (pt != NULL)
			introffheader = FALSE;
		    break;
		}
		if (line[1] == '.') {	/* ignore troff commands */
		    introffheader = TRUE;
		    break;
		}
		tablerow = line2;
		tableins->next = xmalloc(sizeof(struct TABENTRY));
		tableins = tableins->next;
		tableins->next = NULL;
		j = 0;
		tabledelim[1] = tablechar;
		line2[0] = tablechar;
		while ((pt = strtok(tablerow, tabledelim + 1)) != NULL) {
		    if (*pt != NUL) {	/* ignore null columns */
			/* this fails on format line */
			assert(j < MAX_COL);
			strcpy(tableins->col[j], pt);
			k = strlen(pt);
			if (k > tablewidth[j])
			    tablewidth[j] = k;
			++j;
			tablerow = NULL;
			if (j > tablecols)
			    tablecols = j;
		    }
		}
		while (j < MAX_COL)
		    tableins->col[j++][0] = NUL;
	    }
	    break;		/* ignore */
	}
    case '\n':			/* empty text line */
	para = 0;
	tabl = 0;
	fprintf(bo, ":p.");
	break;
    case ' ':{			/* normal text line */
	    if (intable && !intablebut)
		break;
	    if (intablebut) {	/* indexed items in  table, copy
				   to file after table by saving in
				   a temp file meantime */
		if (bt == NULL) {
		    fflush(bo);
		    bt = fopen(TmpFileName, "w+");
		    if (bt == NULL) {
			fprintf(stderr, "Can't open %s\n", TmpFileName);
		    }
		    else
			bo = bt;
		}
	    }
	    if (intablebut && (bt == NULL))
		break;
	    if ((line2[1] == 0) || (line2[1] == '\n')) {
		fprintf(bo, ":p.");
		para = 0;
	    }
	    if (line2[1] == ' ') {
		if (!tabl)
		    fprintf(bo, ":p.");
		fprintf(bo, "%s", &line2[1]);
		fprintf(bo, "\n.br\n");
		tabl = 1;
		para = 0;
	    } else {
		if (!para) {
		    para = 1;	/* not in para so start one */
		    tabl = 0;
		}
		fprintf(bo, "%s \n", &line2[1]);
	    }
	    fflush(bo);
	    break;
	}
    case '^':
	break;			/* ignore */
    default:{
	    if (isdigit((int)line[0])) {	/* start of section */
		if (intable) {
		    intablebut = TRUE;
		    if (bt == NULL) {
			fflush(bo);
			bt = fopen(TmpFileName, "w+");
			if (bt == NULL) {
			    fprintf(stderr, "Can't open %s\n", TmpFileName);
			}
			else
			    bo = bt;
		    }
		}
		if (startpage)	/* use new level 0 item */
		    refs(0, bo, NULL, NULL, NULL);
		else
		    refs(last_line, bo, NULL, NULL, NULL);
		para = 0;	/* not in a paragraph */
		tabl = 0;
		last_line = line_count;
		startpage = 0;
		if (debug) {
		   fprintf(stderr, "%d: %s\n", line_count, &line2[1]);
		}
		klist = lookup(&line2[2]);
		if (klist != NULL)
		    k = klist->line;
		/*if( k<0 ) fprintf(bo,":h%c.", line[0]=='1'?line[0]:line[0]-1);
		   else */
		fprintf(bo, ":h%c res=%d.", line[0], line_count);
		fprintf(bo, &(line2[1]));	/* title */
		fprintf(bo, "\n:p.");
	    } else
		fprintf(stderr, "unknown control code '%c' in column 1, line %d\n",
			line[0], line_count);
	}
	break;
    }
}
