#ifndef lint
static char *RCSid() { return RCSid("$Id: gpltconv.c,v 1.2 1999/08/24 11:22:04 lhecking Exp $"); }
#endif

/* gnuplot - gpltconv.c */

/*
 * Copyright (C) 1999  Lars Hecking
 *
 * This code is in the public domain.
 *
 */

/*
 *
 * Convert old gnuplot syntax to gnuplot 4.0 command syntax. This program
 * was provided because some platforms do not have the tools necessary to
 * perform processing of text files.
 *
 * Usage: gpltconv [infile [outfile]]
 *
 * If no file names are present, gpltconv filters from stdin to stdout.
 * If one file name is present, gpltconv filters from that file to stdout.
 *
 * The conversions are (so far):
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
 * TODO: Make conversion both ways. Detect old/new syntax automagically
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GNUPLOT_COMMENT '#'

/* translation buffer; should be big enough for everyone ...
 * we don't use dynamic memory to keep it nice and simple */
static char tbuf[4*BUFSIZ];

/* strings to be changed */
static const char *instrings[] =
{
    "set data style ",
    "set function style ",
    "set linestyle ",
    "set no"
};

/* replacements */
static const char *outstrings[] =
{
    "set style data ",
    "set style function ",
    "set style line ",
    "unset "
};

#define N_REPL (int)(sizeof(instrings)/sizeof(instrings[0]))

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

    /* if two filenames are specified, read from from the first
     * and write to the second */
    if (argc == 3) {
	if ((outfile = fopen(argv[2], "w")) == (FILE *) NULL) {
	    fprintf(stderr, "%s: Can't open %s for writing\n",
		    argv[0], argv[2]);
	    (void)fclose(infile);
	    exit(EXIT_FAILURE);
	}
    }

    /* loop over input file */
    while (fgets(tbuf,(int)sizeof(tbuf),infile) != NULL) {
	/* skip processing if line is commented */
	if (*tbuf != GNUPLOT_COMMENT) {
	    char *q;
	    int i;
	    /* look for original strings */
	    for (i = 0; i < N_REPL; i++) {
		/* there may be more than one occurrence per line */
		while((q = strstr(tbuf,instrings[i])) != NULL) {
		    size_t inlen = strlen(instrings[i]),
			outlen = strlen(outstrings[i]);

		    if (inlen != outlen)
			memmove(q + outlen, q + inlen, strlen(q) - inlen + 1);

		    strncpy(q,outstrings[i],outlen);
		}
	    }
	}
	(void)fputs(tbuf,outfile);
    }

    (void)fclose(infile);
    (void)fclose(outfile);

    exit(EXIT_SUCCESS);
}
