/*
 * $Id: doc2info.c,v 1.9 1998/12/04 15:18:04 lhecking Exp $
 *
 */

/* GNUPLOT - doc2info.c */

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
 * doc2info.c  -- program to convert Gnuplot .DOC format to 
 *        (Emacs -) Info-file format
 *
 * Created by Stefan Bodewig from doc2gih by Thomas Williams 
 * and doc2html by Russel Lang
 * 1/29/1996
 *
 * usage:  doc2info gnuplot.doc gnuplot.info
 *
 */

/* note that tables must begin in at least the second column to */
/* be formatted correctly and tabs are forbidden */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "ansichek.h"
#include "stdfn.h"

#define MAX_LINE_LEN 1023

#include "doc2x.h"
#include "xref.h"

struct BUFFER {			/* buffer to reformat paragraphs with xrefs */
    char *content;
    struct BUFFER *next;
    struct BUFFER *prev;
};

struct XREFLIST {		/* a list of xrefs allready mentioned in this node */
    struct LIST *ref;
    struct XREFLIST *next;
    struct XREFLIST *prev;
};

/* xref.c */
extern struct LIST *list;
extern struct LIST *head;

extern struct LIST *keylist;
extern struct LIST *keyhead;

extern int maxlevel;
extern int listitems;

char title[MAX_LINE_LEN+1];
char ifile[MAX_LINE_LEN+1];
char ofile[MAX_LINE_LEN+1];
struct XREFLIST *refhead = NULL;

void convert __PROTO((FILE *, FILE *));
void process_line __PROTO((char *, FILE *));
void node_head __PROTO((char *, char *, char *, char *, FILE *));
void name_free __PROTO((char **));
char **name_alloc __PROTO(());
void clear_buffer __PROTO((struct BUFFER *, FILE *));
int inxreflist __PROTO((struct LIST *));
void xref_free __PROTO((void));

int main(argc, argv)
int argc;
char **argv;
{
    FILE *infile;
    FILE *outfile;
    infile = stdin;
    outfile = stdout;

    if (argc > 3) {
	fprintf(stderr, "Usage: %s [infile [outfile]]\n", argv[0]);
	exit(EXIT_FAILURE);
    }
    if (argc >= 2) {
	strcpy(ifile, argv[1]);
	if ((infile = fopen(argv[1], "r")) == (FILE *) NULL) {
	    fprintf(stderr, "%s: Can't open %s for reading\n",
		    argv[0], argv[1]);
	    exit(EXIT_FAILURE);
	}
    } else
	safe_strncpy(ifile, "gnuplot.doc", sizeof(ifile)); /* default value */
    if (argc == 3) {
	safe_strncpy(ofile, argv[2], sizeof(ofile));
	if ((outfile = fopen(argv[2], "w")) == (FILE *) NULL) {
	    fprintf(stderr, "%s: Can't open %s for writing\n",
		    argv[0], argv[2]);
	    exit(EXIT_FAILURE);
	}
    } else
	safe_strncpy(ofile, "gnuplot.info", sizeof(ofile)); /* default value */

    safe_strncpy(title, ofile, sizeof(title));
    strtok(title, ".");		/* without type */
    convert(infile, outfile);
    exit(EXIT_SUCCESS);
}


void convert(a, b)
FILE *a, *b;
{
    static char line[MAX_LINE_LEN+1];

    parse(a);

    refhead = (struct XREFLIST *) xmalloc(sizeof(struct XREFLIST));
    refhead->next = refhead->prev = NULL;

    /* Info header */
    fprintf(b, "\
This file is %s created by doc2info from %s.\n\
\n\
START-INFO-DIR-ENTRY\n\
* Gnuplot: (gnuplot).           Gnuplot plotting program\n\
END-INFO-DIR-ENTRY\n\n",
	    ofile, ifile);

    /* and Top node */
    node_head(NULL, NULL, head->next->string, NULL, b);

    while (get_line(line, sizeof(line), a)) {
	process_line(line, b);
    }
    list_free();
    free(refhead);
}

/*
 * scans the lines for xrefs, creates new nodes and prints or ignores
 * the rest.
 *
 * Info xrefs are visible. Therefore we have to reformat the paragraphs 
 * containing them. All lines of the paragraph are written into a buffer
 * and printed at the end of the paragraph.
 */
void process_line(line, b)
char *line;
FILE *b;
{
    static int line_count = 0;
    struct LIST *node;		/* current node */
    static struct LIST *prev = NULL;	/* previous node */
    int level;			/* current level */
    static char **up = NULL;	/* Array with node names */
    static char **pre = NULL;
    char topic[MAX_LINE_LEN+1];	/* for xrefs */
    int i, j, k, l;
    static int inref = FALSE;	/* flags */
    static int inbold = FALSE;
    static struct BUFFER *buffer;	/* buffer to hold the lines of a paragraph */
    static struct BUFFER *buf_head = NULL;
    int inbuf = 0;		/* offset into buffer */
    char line2[3*MAX_LINE_LEN+1];	/* line of text with added xrefs */
    struct LIST *reflist;
    static struct XREFLIST *lastref = NULL;	/* xrefs that are already mentioned in this node */

    line2[0] = NUL;
    if (!prev)			/* last node visited */
	prev = head;
    if (!lastref)
	lastref = refhead;
    if (!up) {			/* Names of `Prev:' and `Up:' nodes */
	up = name_alloc();
	pre = name_alloc();
	strcpy(up[0], "(dir)");
	strcpy(up[1], "Top");
	strcpy(pre[1], "(dir)");
	strcpy(pre[2], "Top");
    }
    line_count++;

    if (line[0] == ' ')		/* scan line for xrefs  */
	for (i = 0; line[i] != NUL; ++i)
	    if (line[i] == '`') {	/* Reference or boldface (ignore the latter) */
		if (!inref && !inbold) {
		    k = i + 1;	/* next character */
		    j = 0;	/* index into topic */
		    while (line[k] != '`' && line[k] != NUL)
			topic[j++] = line[k++];
		    topic[j] = NUL;

		    /* try to find the xref */
		    reflist = lookup(topic);
		    if (reflist) {
			/* now we have the (key-)list-entry */
			/* convert it to a list-entry that represents a node */
			reflist = lkup_by_number(reflist->line);
		    }
		    /* not interested in xrefs pointing to `Top' or same node */
		    /* we want only one reference per topic in node */
		    if (reflist && reflist->level != 0 && reflist != prev && !inxreflist(reflist)) {
			/* new entry to xreflist */
			lastref->next = (struct XREFLIST *) xmalloc(sizeof(struct XREFLIST));
			lastref->next->prev = lastref;
			lastref = lastref->next;
			lastref->ref = reflist;
			lastref->next = NULL;
			if (!buf_head) {	/* No buffer yet */
			    buf_head = (struct BUFFER *) xmalloc(sizeof(struct BUFFER));
			    buffer = buf_head;
			    buffer->prev = NULL;
			    buffer->next = NULL;
			}
			/* eliminate leading spaces of topic */
			for (j = 0; isspace((int) reflist->string[j]); ++j);
			/* encountered end of line */
			if (line[k] == NUL) {
			    if (line[k - 1] == '\n')	/* throw away new-lines */
				line[--k] = NUL;
			    /* insert xref into line */
			    sprintf(line2, "%s%s (*note %s:: )", line2, line + inbuf, reflist->string + j);
			    inref = TRUE;
			    /* line is done */
			    break;
			}
			/* eliminate spaces before the second ` */
			if (isspace((int) line[k - 1]))
			    for (l = k - 1; line[l] != NUL; ++l)
				line[l] = line[l + 1];

			/* let `plot`s look nicer */
			if (isalpha((int) line[k + 1]))
			    ++k;
			sprintf(line2, "%s%.*s (*note %s:: )", line2, k - inbuf + 1, line + inbuf, reflist->string + j);
			/* line2 contains first inbuf characters of line */
			i = inbuf = k;
		    } else {	/* found no reference */
			inbold = TRUE;
		    }
		} else {
		    if (inref)	/* inref || inbold */
			inref = FALSE;
		    else
			inbold = FALSE;
		}
	    }
    /* just copy normal characters of line with xref */
	    else if (inbuf) {
		strncat(line2, line + i, 1);
		inbuf++;
	    }
    switch (line[0]) {		/* control character */
    case '?':{			/* interactive help entry */
	    break;		/* ignore */
	}
    case '@':{			/* start/end table */
	    break;		/* ignore */
	}
    case '#':{			/* latex table entry */
	    break;		/* ignore */
	}
    case '%':{			/* troff table entry */
	    break;		/* ignore */
	}
    case '^':{			/* html entry */
	    break;		/* ignore */
	}
    case '\n':			/* empty text line */
	if (buf_head) {		/* do we have a buffer? */
	    /* paragraph finished, print it */
	    clear_buffer(buf_head, b);
	    buffer = buf_head = NULL;
	} else			/* just copy the blank line */
	    fputs(line, b);
	break;
    case ' ':{			/* normal text line */
	    if (buf_head) {	/* must be inserted in buffer ? */
		buffer->next = (struct BUFFER *) xmalloc(sizeof(struct BUFFER));
		buffer->next->prev = buffer;
		buffer = buffer->next;
		buffer->next = NULL;
		if (line2[0] == NUL) {	/* line doesn't contain xref */
		    buffer->content = (char *) xmalloc(strlen(line) + 1);
		    strcpy(buffer->content, line);
		} else {	/* line contains xref */
		    buffer->content = (char *) xmalloc(strlen(line2) + 1);
		    strcpy(buffer->content, line2);
		}
	    } else		/* no buffer, just copy */
		fputs(line, b);
	    break;
	}
    default:
	if (isdigit((int) line[0])) {	/* start of section */
	    /* clear xref-list */
	    xref_free();
	    lastref = 0;
	    if (buf_head) {	/* do we have a buffer */
		/* paragraphs are not allways separated by a blank line */
		clear_buffer(buf_head, b);
		buffer = buf_head = NULL;
	    }
	    level = line[0] - '0';

	    if (level > prev->level)	/* going down */
		/* so write menu of previous node */
		refs(prev->line, b, "\n* Menu:\n\n", NULL, "* %s::\n");
	    node = prev->next;
	    if (!node->next) {	/* last node ? */
		node_head(node->string, pre[level + 1], NULL, up[level], b);
		name_free(up);
		name_free(pre);

		/* next node will go up, no 'Next:' node */
	    } else if (node->next->level < level)
		node_head(node->string, pre[level + 1], NULL, up[level], b);

	    else {
		node_head(node->string, pre[level + 1], node->next->string, up[level], b);
		strcpy(pre[level + 1], node->string);

		/* next node will go down */
		if (level < node->next->level) {
		    strcpy(up[level + 1], node->string);
		    strcpy(pre[node->next->level + 1], node->string);
		}
	    }
	    prev = node;
	} else
	    fprintf(stderr, "unknown control code '%c' in column 1, line %d\n",
		    line[0], line_count);
	break;
    }
}

/*
 * write the header of an Info node, treat Top node specially
 */
void node_head(node, prev, next, up, b)
char *node, *prev, *next, *up;
FILE *b;
{
    /* eliminate leading spaces */
    while (node && isspace((int) *node))
	node++;
    while (next && isspace((int) *next))
	next++;
    while (prev && isspace((int) *prev))
	prev++;
    while (up && isspace((int) *up))
	up++;

    if (!prev) {		/* Top node */
	int i;
	fprintf(b, "\nFile: %s, Node: Top, Prev: (dir), Next: %s, Up: (dir)\n\n", ofile, next);
	fprintf(b, "%s\n", title);
	for (i = 0; i < strlen(title); ++i)
	    fprintf(b, "*");
	fprintf(b, "\n\n");
	return;
    }
    fprintf(b, "\n\nFile: %s, ", ofile);
    fprintf(b, "Node: %s, Prev: %s, Up: %s", node, prev, up);

    if (next)
	fprintf(b, ", Next: %s\n\n", next);
    else
	fputs("\n\n", b);
}

/*
 * allocate memory for the node titles (up and prev) 
 * need at most maxlevel+Top+(dir) entries
 */
char **name_alloc __PROTO((void))
{
    char **a;
    int i;

    a = (char **) xmalloc((maxlevel + 2) * sizeof(char *));
    for (i = 0; i <= maxlevel + 1; i++)
	a[i] = (char *) xmalloc(MAX_LINE_LEN+1);
    return a;
}

/*
 * free node names
 */
void name_free(names)
char **names;
{
    int i;

    for (i = 0; i <= maxlevel + 1; i++)
	free(names[i]);
    free(names);
}

/*
 * reformat the buffered lines
 */
void clear_buffer(buf_head, b)
struct BUFFER *buf_head;
FILE *b;
{
    struct BUFFER *run;
    int in_line = 0;		/* offset into current line */
    int in_buf = 0;		/* offset into buffer */
    int i, todo;
    char c;

    /* for all buffer entries */
    for (run = buf_head; run->next; run = run->next) {

	/* unprinted characters */
	todo = strlen(run->next->content);

	/* eliminate new-lines */
	if (run->next->content[todo - 1] == '\n')
	    run->next->content[--todo] = NUL;

	while (todo)
	    if (79 - in_line > todo) {	/* buffer fits into line */
		fprintf(b, "%s", run->next->content + in_buf);
		in_line += todo;
		todo = in_buf = 0;

	    } else {		/* buffer must be split over lines */

		/* search for whitespace to split at */
		for (i = 79 - in_line; i > 2; --i)
		    if (isspace((int) (run->next->content[in_buf + i]))) {
			char *beginnote, *linestart;
			c = run->next->content[in_buf + i - 1];
			if (c == '.')	/* don't split at end of sentence */
			    continue;
			if (c == ' ')	/* ditto */
			    continue;

			/* dont break xref  */
			/* search for xref in current line */
			linestart = run->next->content + in_buf;
			beginnote = strstr(linestart, "(*note");
			while (beginnote && beginnote < linestart + i) {
			    /* don't split if it didn't fit into the line as a whole */
			    if (strchr(beginnote, ')') > linestart + i)
				break;
			    /* xref is complete, maybe there's another one? */
			    beginnote = strstr(beginnote + 1, "(*note");
			}

			/* unbalanced xref ? */
			if (beginnote && beginnote < linestart + i)
			    continue;

			break;
		    }
		if (i > 2) {	/* found a point to split buffer */
		    fprintf(b, "%.*s\n", i, run->next->content + in_buf);
		    todo -= i;
		    in_buf += i;
		    in_line = 0;
		} else {	/* try with a new line */
		    fputs("\n", b);
		    in_line = 0;
		}
	    }
    }
    if (in_line)		/* paragraph ended incomplete line */
	fputs("\n", b);
    fputs("\n", b);

    /* free the buffer */
    for (run = run->prev; run->prev; run = run->prev) {
	free(run->next->content);
	free(run->next);
	run->next = NULL;
    }
    if (buf_head->next) {
	free(buf_head->next->content);
	free(buf_head->next);
	buf_head->next = NULL;
    }
    free(buf_head);
}

/*
 * test whether topic is allready referenced in node
 */
int inxreflist(reflist)
struct LIST *reflist;
{
    struct XREFLIST *run;

    for (run = refhead; run->next; run = run->next)
	if (run->next->ref == reflist)
	    return TRUE;
    return FALSE;
}

/*
 * free the list of xrefs
 */
void xref_free __PROTO((void))
{
    struct XREFLIST *lastref;

    for (lastref = refhead; lastref->next; lastref = lastref->next);
    if (lastref != refhead)
	for (lastref = lastref->prev; lastref->prev; lastref = lastref->prev)
	    free(lastref->next);
    if (refhead->next)
	free(refhead->next);
    refhead->next = NULL;
}
