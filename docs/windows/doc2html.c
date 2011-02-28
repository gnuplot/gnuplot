#ifndef lint
static char *RCSid() { return RCSid("$Id: doc2html.c,v 1.2 2011/02/28 11:40:40 markisch Exp $"); }
#endif

/* GNUPLOT - doc2html.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
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
 * doc2html.c  -- program to convert Gnuplot .DOC format to MS windows
 * HTML help (.html) format.
 *
 * Derived from doc2rtf and doc2html (version 3.7.3) by B. Maerkisch
 *
 * usage:  doc2html file.doc file.html file.htc [-d]
 *
 */

/* note that tables must begin in at least the second column to */
/* be formatted correctly and tabs are forbidden */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "syscfg.h"
#include "stdfn.h"
#define MAX_LINE_LEN 10230
#include "doc2x.h"
#include "xref.h"
#include "version.h"

static TBOOLEAN debug = FALSE;

void convert __PROTO((FILE *, FILE *, FILE *));
void process_line __PROTO((char *, FILE *, FILE *));

int
main (int argc, char **argv)
{
    FILE *infile;
    FILE *outfile;
    FILE *contents;
    if (argc == 5 && argv[4][0] == '-' && argv[4][1] == 'd')
	debug = TRUE;

    if (argc != 4 && !debug) {
	fprintf(stderr, "Usage: %s infile outfile contentfile\n", argv[0]);
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
    if ((contents = fopen(argv[3], "w")) == (FILE *) NULL) {
	fprintf(stderr, "%s: Can't open %s for writing\n",
		argv[0], argv[3]);
	fclose(infile);
	fclose(outfile);
	exit(EXIT_FAILURE);
    }
    parse(infile);
    convert(infile, outfile, contents);
    return EXIT_SUCCESS;
}


void
convert(FILE *a, FILE *b, FILE *c)
{
    static char line[MAX_LINE_LEN+1];

    /* generate html header */
    fprintf(b, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");
    fprintf(b, "<html>\n");
    fprintf(b, "<head>\n");
    fprintf(b, "<meta http-equiv=\"Content-type\" content=\"text/html; charset=windows-1252\">\n");
    fprintf(b, "<title>gnuplot help</title>\n");
    fprintf(b, "</head>\n");
    fprintf(b, "<body>\n");
    fprintf(b, "<h1 align=\"center\">gnuplot %s patchlevel %s</h1>\n", gnuplot_version, gnuplot_patchlevel);
	
    /* generate html header */
    fprintf(c, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");
    fprintf(c, "<html>\n");
    fprintf(c, "<head>\n");
    fprintf(c, "<meta http-equiv=\"Content-type\" content=\"text/html; charset=windows-1252\">\n");
    fprintf(c, "<title>gnuplot help contents</title>\n");
    fprintf(c, "</head>\n");
    fprintf(c, "<body>\n");
    fprintf(c, "<ul>\n");

    /* process each line of the file */
    while (get_line(line, sizeof(line), a)) {
	process_line(line, b, c);
    }

   /* close final page and generate trailer */
    fprintf(b, "</BODY>\n");
    fprintf(b, "</HTML>\n");

    /* close final page and generate trailer */
    fprintf(c, "</ul>\n");
    fprintf(c, "</BODY>\n");
    fprintf(c, "</HTML>\n");
    list_free();
}

void
process_line(char *line, FILE *b, FILE *c)
{
    static int line_count = 0;
    static char line2[MAX_LINE_LEN+1];
    static int last_line;

    static int level = 1;
    static TBOOLEAN startpage = TRUE;
    static TBOOLEAN tabl = FALSE;
    static TBOOLEAN skiptable = FALSE;
    static TBOOLEAN forcetable = FALSE;
    static TBOOLEAN para = FALSE;
    static TBOOLEAN inhlink = FALSE;
    static TBOOLEAN inkey = FALSE;
    static TBOOLEAN inquote = FALSE;
    static int inref = 0;
    static int klink = 0;  /* link number counter */

    struct LIST *klist;

    int i, j, k, l;
    line_count++;

    i = 0;
    j = 0;
    while (line[i] != NUL) {
	switch (line[i]) {
	case '<':
	    line2[j++] = '&';
	    line2[j++] = 'l';
	    line2[j++] = 't';
	    line2[j] = ';';
	    break;
	case '>':
	    line2[j++] = '&';
	    line2[j++] = 'g';
	    line2[j++] = 't';
	    line2[j] = ';';
	    break;
	case '&':
	    line2[j++] = '&';
	    line2[j++] = 'a';
	    line2[j++] = 'm';
	    line2[j++] = 'p';
	    line2[j] = ';';
	    break;
	case '\r':
	case '\n':
	    break;
	case '`':		/* backquotes mean boldface or link */
	    if (line[1] == ' ')	/* tabular line */
		line2[j] = line[i];
	    else if ((!inref) && (!inquote)) {
                char topic[MAX_LINE_LEN+1];
		k = i + 1;	/* index into current string */
		l = 0;		/* index into topic string */
		while ((line[k] != '`') && (line[k] != NUL))
		    topic[l++] = line[k++];
		topic[l] = NUL;
		klist = lookup(topic);
		if (klist && (k = klist->line) > 0 && (k != last_line)) {
                    char hyplink1[MAX_LINE_LEN+1];
                    char id[15];
#ifndef HTML_HARDLINKS
                    sprintf(id, "link%i", ++klink);
                    sprintf(hyplink1, "<OBJECT id=%s type=\"application/x-oleobject\" classid=\"clsid:adb880a6-d8ff-11cf-9377-00aa003b7a11\">\n"
                                      "  <PARAM name=\"Command\" value=\"KLink\">\n"
                                      "  <PARAM name=\"Item1\" value=\"\">\n"
                                      "  <PARAM name=\"Item2\" value=\"%s\">\n"
                                      "</OBJECT>\n"
                                      "<a href=\"JavaScript:%s.Click()\">",
                                      id, topic, id);
                    if (debug)
                        fprintf(stderr, "hyper link \"%s\" - %s on line %d\n", topic, id, line_count);
#else
                    if ((klist->line) > 1)
                        sprintf(hyplink1, "<a href=\"loc%d.html\">", klist->line);
                    else
                        sprintf(hyplink1, "<a href=\"wgnuplot.html\">");
                    if (debug)
                        fprintf(stderr, "hyper link \"%s\" - loc%d.html on line %d\n", topic, klist->line, line_count);
#endif
                    strcpy(line2 + j, hyplink1);
		    j += strlen(hyplink1) - 1;

		    inref = k;
		} else {
		    if (debug)
			fprintf(stderr, "Can't make link for \042%s\042 on line %d\n", topic, line_count);
		    line2[j++] = '<';
		    line2[j++] = 'b';
		    line2[j] = '>';
		    inquote = TRUE;
		}
	    } else {
		if (inquote && inref)
		    fprintf(stderr, "Warning: Reference Quote conflict line %d\n", line_count);
		if (inquote) {
		    line2[j++] = '<';
		    line2[j++] = '/';
		    line2[j++] = 'b';
		    line2[j] = '>';
		    inquote = FALSE;
		}
		if (inref) {
		    /* must be inref */
		    line2[j++] = '<';
		    line2[j++] = '/';
		    line2[j++] = 'a';
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
	line2[j] = NUL;
    }

    i = 1;

    if (inkey && !(line[0] == '?' || line[0] == '=')) {
        /* close keyword object */
        fprintf(b, "</OBJECT>\n");
        inkey = FALSE;
    }

    switch (line[0]) {		/* control character */
    case '?':			/* interactive help entry */
    case '=': 			/* latex index entry */
            if ((line2[1] != NUL) && (line2[1] != ' ')) {
                if (!inkey) {
                    /* open keyword object */
                    fprintf(b, "<OBJECT type=\"application/x-oleobject\" classid=\"clsid:1e2a7bd0-dab9-11d0-b93a-00c04fc99f9e\">\n");
                    inkey = TRUE;
                }
                /* add keyword */
                fprintf(b, "  <param name=\"Keyword\" value=\"%s\">\n", &line2[1]);
                if (debug)
                    fprintf(stderr,"keyword defintion: \"%s\" on line %d.\n", line2 + 1, line_count);
            }
	    break;
    case '@':{			/* start/end table */
            skiptable = !skiptable;
            if (!skiptable) forcetable = FALSE;
	    break;
	}
    case '^':{			/* html link escape */
            if ((!inhlink) && (line[3] == 'a') && (line[5] == 'h')) {
                char *str;
                inhlink = TRUE;
                /* remove trailing newline etc */
                str = line + strlen(line) - 1;
                while (*str=='\r' || *str=='\n') *str-- = NUL;
                fprintf(b, "%s", line + 2);
            } else if (inhlink) {
                inhlink = FALSE;
	        fputs(line + 2, b);	/* copy directly */
            } else {
                if (line[2] == '!') { /* hack for function sections */
                    const char magic[] = "<!-- INCLUDE_NEXT_TABLE -->";
                    if (strncmp(line+1, magic, strlen(magic)) == 0)
                        forcetable = TRUE;
                }
                inhlink = FALSE;
	        fputs(line + 1, b);	/* copy directly */
            }
	    break;		/* ignore */
	}
    case 'F':			/* latex embedded figure */
            if (para) fprintf(b, "</p><p align=\"justify\">\n");
            fprintf(b, "<img src=\"%s.png\" alt=\"%s\">\n", line2+1, line2+1);
            if (para) fprintf(b, "</p><p align=\"justify\">\n");
            break;
    case '#':{			/* latex table entry */
	    break;		/* ignore */
	}
    case '%':{			/* troff table entry */
	    break;		/* ignore */
	}
    case '\n':			/* empty text line */
	if (tabl)
	    fprintf(b, "</pre>\n");
	if (para)
	    fprintf(b, "</p>\n");
	para = FALSE;
	tabl = FALSE;
	break;
    case ' ':{			/* normal text line */
            if (skiptable && !forcetable) break; /* break */
	    if ((line2[1] == NUL) || (line2[1] == '\n')) {
		fprintf(b, "\n");
		if (para)
		    fprintf(b, "</p>\n");
	        para = FALSE;
	        tabl = FALSE;
	    } else if (line2[1] == ' ') { /* in table */
                if (inhlink) {
                    int numspaces = 0;
                    while (line2[numspaces+1] == ' ') numspaces++;
                    fprintf(b, "<tt>%s</tt>", line2+1+numspaces);
                } else { 
                    if (!tabl) {
		        if (para)
			    fprintf(b, "</p>\n");
		        fprintf(b, "<pre>\n");
		    }
		    fprintf(b, "%s\n", &line2[1]);
		    para = FALSE;
		    tabl = TRUE;
                }
	    } else {
                if (inhlink) {
                    /* no newline here! */
                    fprintf(b, "%s", &line2[1]);
                } else {
                    if (tabl) {
        	        fprintf(b, "</pre>\n");
		        tabl = 0;
                    } 
                    if (!para) {
		        para = TRUE;	/* not in para so start one */
		        tabl = FALSE;
		        fprintf(b, "<p align=\"justify\">");
		    }
                    fprintf(b, "%s\n", &line2[1]);
                }
	    }
	    break;
	}
    default:{
	    if (isdigit((int)line[0])) {	/* start of section */
                char location[MAX_LINE_LEN+1];
                int newlevel = line[0]-'0';
                char spacer[10];

		if (startpage) {
		} else {
	            if (tabl)
	                fprintf(b, "</pre>\n");
	            if (para)
	                fprintf(b, "</p>\n");
		}
		para = FALSE;	/* not in a paragraph */
		tabl = FALSE;

		if (!startpage)	/* add list of subtopics */
		    refs(last_line, b, "<h3>Subtopics</h3>\n<menu>\n", "</menu>\n", "\t<li><a href=\"loc%d.html\">%s</a></li>\n");

		last_line = line_count;
		fprintf(b, "\n");
		
                /* output unique ID */
		sprintf(location, "loc%d", line_count);

                /* split contents */
                if (!startpage) {
                    fprintf(b, "<OBJECT type=\"application/x-oleobject\" classid=\"clsid:1e2a7bd0-dab9-11d0-b93a-00c04fc99f9e\">\n");
	            fprintf(b, "<param name=\"New HTML file\" value=\"%s.html\">\n", location);
	            fprintf(b, "<param name=\"New HTML title\" value=\"%s\">\n", &line2[2]);
                    fprintf(b, "</OBJECT>\n");
                    fprintf(b, "<h2>%s</h2>\n", &line2[2]);
                }

                /* create toc entry */
                if (newlevel > level) {
                    level = newlevel;
                    memset(spacer, '\t', level);
                    spacer[level] = NUL;
                    fprintf(c, "%s<ul>\n", spacer);
                } else if (newlevel < level) {
                    for ( ; newlevel < level; level--) {
                        memset(spacer, '\t', level);
                        spacer[level] = NUL;
                        fprintf(c, "%s</ul>\n", spacer);
                    }
                }
                level = newlevel;
                memset(spacer, '\t', level);
                spacer[level] = NUL;
                fprintf(c, "%s<li> <OBJECT type=\"text/sitemap\">\n", spacer);
		fprintf(c, "%s  <param name=\"Name\" value=\"%s\">\n", spacer, &line2[2]);
                fprintf(c, "%s  <param name=\"Local\" value=\"%s.html\">\n", spacer, startpage ? "wgnuplot" : location);
		fprintf(c, "%s  </OBJECT>\n", spacer);
                if (debug)
                    fprintf(stderr, "Section \"%s\", Level %i, ID=%s on line %d\n", line2 + 2, level, location, line_count);

                startpage = FALSE;
            } else
		fprintf(stderr, "unknown control code '%c' in column 1, line %d\n",
			line[0], line_count);
	    break;
	}
    }
}
