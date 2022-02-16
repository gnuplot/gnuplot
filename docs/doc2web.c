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
 * doc2web.c reformat gnuplot documentation into HTML for use as a 
 *		web resource.
 *
 * Derived from windows/doc2html
 * Derived from doc2rtf and doc2html (version 3.7.3) by B. Maerkisch
 *
 * usage:  doc2web file.doc outputdirectory [-d]
 *
 */

/* note that tables must begin in at least the second column to */
/* be formatted correctly and tabs are forbidden */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define CREATE_INDEX

#include "syscfg.h"
#include "stdfn.h"
#define MAX_LINE_LEN 10230
#include "doc2x.h"
#include "xref.h"
#include "version.h"

static char path[PATH_MAX];
static const char name[] = "gnuplot5";
static char *sectionname = "";
static TBOOLEAN collapsing_terminal_docs = FALSE;
static TBOOLEAN processing_title_page = FALSE;
static TBOOLEAN file_has_sidebar = FALSE;

void convert(FILE *, FILE *, FILE *);
void process_line(char *, FILE *, FILE *);
void sidebar(FILE *a, int start);
void header(FILE *a, char * title);
void footer(FILE *a);

int
main (int argc, char **argv)
{
    FILE *infile;
    FILE *outfile;
    FILE *index = NULL;
    char filename[PATH_MAX];
    char *last_char;

    if (argc != 3) {
	fprintf(stderr, "Usage: %s infile outpath\n", argv[0]);
	exit(EXIT_FAILURE);
    }
    if ((infile = fopen(argv[1], "r")) == (FILE *) NULL) {
	fprintf(stderr, "%s: Can't open %s for reading\n",
		argv[0], argv[1]);
	exit(EXIT_FAILURE);
    }
    strcpy(path, argv[2]);
    /* make sure there's a path separator at the end */
    last_char = path + strlen(path) - 1;
    if ((*last_char != DIRSEP1) && (*last_char != DIRSEP2)) {
        *(++last_char) = DIRSEP1;
        *(++last_char) = 0;
    }

    /* Wrap the title page text in the same format as the manual.
     * Assume that if we can create and write the file here,
     * we will be able to do the same for the manual pages later.
     */
    strcpy(filename, path);
    strcat(filename, name);
    strcat(filename, ".html");
    if ((outfile = fopen(filename, "w")) == (FILE *) NULL) {
	fprintf(stderr, "%s: Can't open %s for writing\n",
		argv[0], filename);
	fclose(infile);
	exit(EXIT_FAILURE);
    } else {
	char line[80];
	// fprintf(stderr,"Opening %s for output\n", filename);
	sprintf(line, "gnuplot %s", VERSION_MAJOR);
	header(outfile, line);
	fprintf(outfile, "<h1 align=\"center\">gnuplot %s</h1>\n", VERSION_MAJOR);
	footer(outfile);
    }

#ifdef CREATE_INDEX
    strcpy(filename, path);
    strcat(filename, "index");
    strcat(filename, ".hhk");
    if ((index = fopen(filename, "w")) == (FILE *) NULL) {
	fprintf(stderr, "%s: Can't open %s for writing\n",
		argv[0], filename);
	fclose(infile);
	fclose(outfile);
	exit(EXIT_FAILURE);
    }
#endif

    parse(infile);
    convert(infile, outfile, index);
    return EXIT_SUCCESS;
}


void
header(FILE *a, char * title)
{
    /* generate html header */
    fprintf(a, "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\">\n");
    fprintf(a, "<html>\n");
    fprintf(a, "<head>\n");
    fprintf(a, "<meta http-equiv=\"Content-type\" content=\"text/html; charset=utf-8\">\n");
    fprintf(a, "<link text=\"text/css\" href=\"gnuplot_docs.css\" rel=\"stylesheet\">\n");
    fprintf(a, "<title>%s</title>\n", title);
    fprintf(a, "</head>\n");
    fprintf(a, "<body>\n");
    fputs( 
"<table class=\"center\" style=\"font-size:150%;\" width=\"80%\" >\n"
"<th class=\"center\"><a href=\"gnuplot5.html\">Credits</a></td>\n"
"<th class=\"center\"><a href=\"Overview.html\">Overview</a></td>\n"
"<th class=\"center\"><a href=\"Plotting_Styles.html\">Plotting Styles</a></td>\n"
"<th class=\"center\"><a href=\"Commands.html\">Commands</a></td>\n"
"<th class=\"center\"><a href=\"Terminals.html\">Terminals</a></td>\n"
"</th></table>\n", a);
}


void
footer(FILE *a)
{
    /* done with this HTML output page */
    fprintf(a, "</body>\n");
    fprintf(a, "</html>\n");
}

void
sidebar(FILE *a, int start)
{
    /* wrap the sidebar and the main page content in a table */
    if (start)
	fputs(
"<table><tr><td width=20%>\n"
"    <h1>Index</h1>\n"
"    <object type=\"text/html\" class=\"sidebar\" data=\"index.html\">\n"
"</td><td width=80%>\n", a);
    else
	fprintf(a, "</td></tr></table>\n");
}


/*
 * a is input (normally gnuplot.doc)
 * b is output (the html fragment we are working on)
 * d is index.html (unsorted at this point)
 */
void
convert(FILE *a, FILE *b, FILE *d)
{
    static char line[MAX_LINE_LEN+1];

    /* process each line of the file */
    while (get_line(line, sizeof(line), a)) {
	process_line(line, b, d);
    }

    list_free();
}

void
process_line(char *line, FILE *b, FILE *d)
{
    static int line_count = 0;
    static char line2[MAX_LINE_LEN+1];
    static int last_line;

    static TBOOLEAN startpage = TRUE;
    static TBOOLEAN tabl = FALSE;
    static TBOOLEAN intable = FALSE;
    static TBOOLEAN skiptable = FALSE;
    static TBOOLEAN forcetable = FALSE;
    static TBOOLEAN para = FALSE;
    static TBOOLEAN inhlink = FALSE;
    static TBOOLEAN inquote = FALSE;
    static int inref = 0;
    static int klink = 0;  /* link number counter */
    static char location[PATH_MAX+1] = {0};
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
		/* Do not turn `gnuplot` in to a self-reference hyperlink */
		if (!strcmp(&topic[1],"nuplot"))
		    klist = 0;
		else
		    klist = lookup(topic);
		if (klist && (k = klist->line) > 0 && (k != last_line)) {
                    char hyplink1[MAX_LINE_LEN+1];
		    (void)klink;	/* Otherwise compiler warning about unused variable */
                    /* explicit links */
                    if ((klist->line) > 1)
                        sprintf(hyplink1, "<a href=\"loc%d.html\">", klist->line);
                    else
                        sprintf(hyplink1, "<a href=\"%s.html\">", name);
                    strcpy(line2 + j, hyplink1);
		    j += strlen(hyplink1) - 1;

		    inref = k;
		} else {
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

    switch (line[0]) {		/* control character */
    case '=': 			/* latex index entry */
	    break;
    case '?':			/* interactive help entry */
            if ((line2[1] != NUL) && (line2[1] != ' ') && (line2[1] != '?')) {
#ifdef CREATE_INDEX
		/* Only keep single-word entries */
		if (!strchr( &(line2[1]), ' ' )) {
		    fprintf(d, "<li><a href=\"%s.html\" target=\"_parent\">", location);
		    fprintf(d, " %s </a></li>\n", &line2[1]);
		}
#endif
            }
	    break;
    case '@':{			/* start/end table */
	    skiptable = !skiptable;
	    if (!skiptable) forcetable = FALSE;
	    intable = !intable; /* just for latex list support */
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


	    } else if (processing_title_page && !strncmp(&line[1], "<!-- end", 8)) {
		    /* Reached the end of the title page records.
		     * Jump to approximately where we would have been if there
		     * were no title records.
		     */
		    // fprintf(stderr, "Reached end of Title\n");
		    startpage = TRUE;
		    goto end_of_titlepage;

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
    case 'F':			/* embedded figure */
            if (para) fprintf(b, "</p>");
            fprintf(b, "<p align=\"center\">\n");
            fprintf(b, "<img src=\"%s.svg\" alt=\"%s\">\n", line2+1, line2+1);
            fprintf(b, "</p>");
            if (para) fprintf(b, "<p align=\"justify\">\n");
            break;
    case '#':{			/* latex table entry */
	    if (!intable) {  /* HACK: we just handle itemized lists */
		/* Itemized list outside of table */
		if (line[1] == 's')
		    (void) fputs("<ul>\n", b);
		else if (line[1] == 'e')
		    (void) fputs("</ul>\n", b);
		else if (line[1] == 'b') {
		    /* Bullet */
		    fprintf(b, "<li class=\"shortlist\">%s\n", line2+2);
		}
		else if (line[1] == '#') {
		    /* Continuation of bulleted line */
		    fputs(line2+2, b);
		}
		else if (strncmp(line+1, "TeX", 3) != 0) {
		    if (strchr(line, '\n'))
			*(strchr(line, '\n')) = '\0';
		    fprintf(b, "<li><pre>%s</pre>\n", line + 1);
		}
	    }
	    break;
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
                int newlevel = line[0]-'0';

		if (newlevel == 1) {
		    startpage = TRUE;
		    intable = FALSE;
		    if (!strncmp(&line[2], "Gnuplot", 7)) {
			processing_title_page = TRUE;
			sectionname = "Overview";
		    } else if (!strncmp(&line[2], "Plot", 4))
			sectionname = "Plotting_Styles";
		    else if (!strncmp(&line[2], "Commands", 4))
			sectionname = "Commands";
		    else if (!strncmp(&line[2], "Term", 4))
			sectionname = "Terminals";
		    else if (!strncmp(&line[2], "Bugs", 4))
			sectionname = "Bugs";

		} else {
	            if (tabl)
	                fprintf(b, "</pre>\n");
	            if (para)
	                fprintf(b, "</p>\n");
		}

		para = FALSE;	/* not in a paragraph */
		tabl = FALSE;

                /* output unique ID */
                if (startpage) {
                    strcpy(location, sectionname);
                } else
                    sprintf(location, "loc%d", line_count);

		/* add list of subtopics */
		if (!collapsing_terminal_docs && !processing_title_page) {
		    reftable(last_line, b,
			"<table class=\"center\"><th colspan=3>Subtopics</th><tr><td width=33%><ul>\n",
		    "</ul></td></tr></table>\n",
		    "\t<li><a href=\"loc%d.html\">%s</a></li>\n",
		    14,
		    "</ul></td><td width=33%><ul>\n");
		}
		last_line = line_count;

		if (processing_title_page) {
		    /* Nothing to do here */

		} else if (collapsing_terminal_docs) {
		    collapsing_terminal_docs = FALSE;

		} else {
                    /* close current file and start a new one */
		    char newfile[PATH_MAX] = "";

    end_of_titlepage:

		    /* If we got here via "goto end_of_titlepage"
		     * dummy up the environment that would have been there if
		     * it were not for that detour past the titlepage records.
		     */
		    if (processing_title_page) {
			processing_title_page = FALSE;
			last_line = 1;
		    }

#ifdef CREATE_INDEX
		    if (file_has_sidebar) {
			sidebar(b, 0);
			file_has_sidebar = FALSE;
		    }
#endif
		    footer(b);
		    fclose(b);

                    /* open new file */
		    strcat(newfile,path);
		    strncat(newfile,location,PATH_MAX-strlen(newfile)-6);
		    strcat(newfile,".html");
                    if (!(b = fopen(newfile, "w"))) {
                        fprintf(stderr, "doc2web: Can't open %s for writing\n",
                            newfile);
                        exit(EXIT_FAILURE);
                    }
		    if (startpage) {
			// fprintf(stderr,"Opening %s for output\n",newfile);
			header(b, sectionname);
#ifdef CREATE_INDEX
			sidebar(b, 1);
			file_has_sidebar = TRUE;
#endif
			fprintf(b, "<h1>Gnuplot %s %s</h1>\n", VERSION_MAJOR, sectionname);
		    } else {
			header(b, &line2[2]);
			fprintf(b, "<h2>%s</h2>\n", &line2[2]);
		    }
		}

		if (startpage && !strncmp(sectionname,"Terminals",8)) {
		    collapsing_terminal_docs = TRUE;
		}
                startpage = FALSE;

            } else
		fprintf(stderr, "unknown control code '%c' in column 1, line %d\n",
			line[0], line_count);
	    break;
	}
    }
}
