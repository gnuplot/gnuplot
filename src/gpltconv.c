#ifndef lint
static char *RCSid() { return RCSid("$Id: $"); }
#endif

/* gnuplot - gpltconv.c */

/*
 * Copyright (C) 1999  Lars Hecking
 *
 * The code is released to the public domain.
 *
 */

/*
 *
 * Convert old gnuplot syntax to gnuplot 4.0 command syntax
 *
 * Usage: gpltconv [infile [outfile]]
 *
 * If no file names are present, the program filters from stdin to stdout.
 * If one file name is present, the programs filters from that file to stdout.
 *
 * The conversions are:
 *
 * set data style xxx     -> set style data xxx
 * set function style xxx -> set style function xxx
 * set linestyle xxx      -> set style line xxx
 * set noVariable         -> unset Variable
 *
 * Limitations:
 * It is assumed that all commands and options are
 * separated by exactly one space! If your scripts
 * are in a weird format with spaces and tabs,
 * use the following sed script
 *
 * # space plus tab between the brackets
 * /^[^#]/ s/set[ 	][ 	]*data[ 	][ 	]*style/set style data/g
 * /^[^#]/ s/set[ 	][ 	]*function[ 	][ 	]*style/set style function/g
 * /^[^#]/ s/set[ 	][ 	]*linestyle[ 	][ 	]*style/set style line/g
 * /^[^#]/ s/set[ 	][ 	]*no\([^ 	]*\)/unset \1/g
 *
 * or perl.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GNUPLOT_COMMENT '#'

/* translation buffer; should be big enough for everyone ...
 * we don't use dynamic memory to keep the code simple */
static char tbuf[4096];

/* strings to be replaced */
static char *datastring = "set data style ";
static char *funcstring = "set function style ";
static char *linestring = "set linestyle ";
static char *nostring   = "set no";

int
main(argc, argv)
int argc;
char **argv;
{
    FILE *infile;
    FILE *outfile;

    /* filter if no filenames are given on the command line */
    infile = stdin;
    outfile = stdout;

    /* if more than two filenames are specified: error */
    if (argc > 3) {
	fprintf(stderr, "Usage: %s [infile [outfile]]\n", argv[0]);
	exit(EXIT_FAILURE);
    }

    /* if one filename is specified, read from the file
     * and write to stdout */
    if (argc >= 2) {
	if ((infile = fopen(argv[1], "r")) == (FILE *) NULL) {
	    fprintf(stderr, "%s: Can't open %s for reading\n",
		    argv[0], argv[1]);
	    exit(EXIT_FAILURE);
	}
    }

    /* if two filename are specified, read from from the first
     * and write to the second */
    if (argc == 3) {
	if ((outfile = fopen(argv[2], "w")) == (FILE *) NULL) {
	    fprintf(stderr, "%s: Can't open %s for writing\n",
		    argv[0], argv[2]);
	    exit(EXIT_FAILURE);
	}
    }

    /* loop over input file */
    while (fgets(tbuf,sizeof(tbuf),infile) != NULL) {
	char *q = &tbuf[0];
	int len = strlen(tbuf);

	/* if the line is a comment, or does not contain any of the
	 * strings to be translated, write it out directly and
	 * skip all processing */
	if (*tbuf == GNUPLOT_COMMENT ||
	    (strstr(tbuf,datastring) == NULL &&
	     strstr(tbuf,funcstring) == NULL &&
	     strstr(tbuf,linestring) == NULL &&
	     strstr(tbuf,nostring) == NULL))
	    fputs(tbuf,outfile);
	else {
	    /* loop over input line, may contain several commands */
	    while (q < &tbuf[len]) {

		q = strstr(tbuf,datastring);
		if (q != NULL) {
		    strncpy(q,"set style data ",strlen(datastring));
		    q += strlen(datastring);
		} else {

		    q = strstr(tbuf,funcstring);
		    if (q != NULL) {
			strncpy(q,"set style function ",strlen(funcstring));
			q += strlen(funcstring);
		    } else {

			q = strstr(tbuf,linestring);
			if (q != NULL) {
			    /* the replacement string is one byte longer, so
			     * we shift everything by one byte, including the
			     * match string and the concluding \0 byte */
			    memmove(q+1,q,strlen(q)+1);
			    strncpy(q,"set style line ",strlen(linestring)+1);
			    q += strlen(linestring);
			} else {

			    q = strstr(tbuf,nostring);
			    if (q != NULL) {
				strncpy(q,"unset ",strlen(nostring));
				q += strlen(nostring);
			    } else
				break;
			}
		    }
		}
	    }

	    fputs(tbuf,outfile);
	}
    }


    exit(EXIT_SUCCESS);
}
