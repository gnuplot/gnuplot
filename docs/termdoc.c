/*
 * $Id: termdoc.c,v 1.19 1998/04/14 00:17:14 drd Exp $
 *
 */

/* GNUPLOT - termdoc.c */
/*
 * Copyright (C) 1986 - 1993, 1997   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and 
 * that both that copyright notice and this permission notice appear 
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed 
 * as patches to released version.
 *  
 * This software is provided "as is" without express or implied warranty.
 * 
 *
 * AUTHORS
 *
 *  David Denholm - 1996
 */
 
/* this file is included into all the doc2* convertors
 * it provides a replacement for fgets() which inserts all the
 * help from the terminal drivers line by line at the < in the
 * gnuplot.doc file. This way, doc2* dont need to know what's going
 * on, and think it's all coming from one place.
 *
 * Can be compiled as a standalone program to generate the raw
 * .doc test, when compiled with -DTEST
 *
 * Strips comment lines {so none of doc2* need to bother} 
 * but magic comments beginning  C#  are used as markers
 * for line number recording (as c compilers)
 * We set BEGIN_HELP macro to "C#<driver>" as a special marker.
 *
 * Hmm - this is turning more and more into a preprocessor !
 * gnuplot.doc now has multiple top-level entries, but
 * some help systems (eg VMS) cannot tolerate this.
 * As a complete bodge, conditional on SINGLE_TOP_LEVEL,
 * we accept only the first 1, and map any subsequent 1's to 2's
 * At present, this leaves a bogus, empty section called
 * commands, but that's a small price to pay to get it
 * working properly
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ansichek.h"
#include "stdfn.h"

/* may need to something more clever for 64k machines ? */

#define TERM_DRIVER_H   /* a complete lie, but they dont need it ! */
#define TERM_HELP

/* for error reporting, we arrange for each terminals help to start
 *  "C#driver"
 * and end
 *  "C#"
 * requires ansi preprocesser. It's not a big loss if it
 * doesn't work, since it is really for use by doc maintainer.
 */

#ifdef ANSI_C
#define START_HELP(driver) "C#" #driver , 
#define END_HELP(driver)   ,"C#",
#else
#define START_HELP(driver) /*nowt*/
#define END_HELP(driver)   ,
#endif


char *termtext[] = {
#ifdef ALL_TERM_DOC
#include "allterm.h"
#else
#include "term.h"
#endif
NULL
};



/* because we hide details of including terminal drivers,
 * we provide line numbers and file names
 */

int termdoc_lineno;
char termdoc_filename[80];

char *get_line __PROTO((char *buffer, int max, FILE *fp));

char *get_line(buffer,max,fp)
char *buffer;
int max;
FILE *fp;
{
	static int line=-1;  /* not going yet */
	static int level=0; /* terminals are at level 1 - we add this */

	static int save_lineno; /* for saving lineno */

#ifdef SINGLE_TOP_LEVEL
	static int seen_a_one = 0;
#endif

	if (line == -1) {

		/* we are reading from file */

		{
			read_another_line: /* come here if a comment is read */

			if (!fgets(buffer, max, fp))
				return NULL; /* EOF */
			++termdoc_lineno;
			if (buffer[0] == 'C') {
				if (buffer[1] == '#') {
					/* should not happen in gnuplot.doc, but... */ 
					strcpy(termdoc_filename, buffer+2);
					termdoc_filename[strlen(termdoc_filename)-1] = 0;
					termdoc_lineno = 0;
				}
				goto read_another_line; /* skip comments */
			}
		}

#ifdef SINGLE_TOP_LEVEL
		if (buffer[0] == '1')
		{
			if (seen_a_one)
			{
				buffer[0] = '2';
			}
			seen_a_one = 1;
		}
#endif


		if (buffer[0] != '<')
			return buffer; /* the simple case */

		/* prepare to return text from the terminal drivers */
		save_lineno = termdoc_lineno;
		termdoc_lineno = -1; /* dont count the C# */
		level=buffer[1]-'1';
		line=0;
	}

	/* we're sending lines from terminal help */

	/* process and skip comments. Note that the last line
	 * will invariably be a comment !
	 */

	while(termtext[line][0]=='C')
	{
		if (termtext[line][1] == '#') {
			strcpy(termdoc_filename, termtext[line]+2);
			termdoc_lineno = 0;
		}
		++termdoc_lineno;

		if (!termtext[++line])
		{
			/* end of text : need to return a line from
			 * the file. Recursive call is best way out
			 */
			termdoc_lineno = save_lineno;
			line=(-1); /* we've done the last line, so get next line from file */
			return get_line(buffer, max, fp);
		}
	}


	/* termtext[line] is the next line of text.
	 * more efficient to return pointer, but we need to modify it
	 */

	++termdoc_lineno;
	strncpy(buffer, termtext[line], max);
	strncat(buffer, "\n", max);
	if (isdigit(buffer[0]))
		buffer[0] += level;

	if (!termtext[++line]) {
		/* end of terminal help : return to input file next time
		 * last (pseudo-)line in each terminal should be a comment,
		 * so we shouldn't get here, but...
		 */
		termdoc_lineno = save_lineno;
		line=(-1); /* we've done the last line, so get next line from file */
	}

	return buffer;
}


#ifdef TEST
int main()
{
	char line[256];
	while(get_line(line, 256, stdin))
		printf("%s:%d:%s", termdoc_filename, termdoc_lineno, line);
	return 0;
}
#endif

#define fgets(a,b,c) get_line(a,b,c)
