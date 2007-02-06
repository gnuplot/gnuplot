#ifndef lint
static char *RCSid() { return RCSid("$Id: misc.c,v 1.85 2007/01/17 05:34:17 sfeam Exp $"); }
#endif

/* GNUPLOT - misc.c */

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

#include "misc.h"

#include "alloc.h"
#include "command.h"
#include "graphics.h"
#include "parse.h"		/* for const_*() */
#include "plot.h"
#include "tables.h"
#include "util.h"
#include "variable.h"
#include "axis.h"

#if defined(HAVE_DIRENT_H)
# include <sys/types.h>
# include <dirent.h>
#elif defined(_Windows)
# include <windows.h>
#endif

/* name of command file; NULL if terminal */
char *infile_name = NULL;

static TBOOLEAN lf_pop __PROTO((void));
static void lf_push __PROTO((FILE *));
static void arrow_use_properties __PROTO((struct arrow_style_type *arrow, int tag));
static char *recursivefullname __PROTO((const char *path, const char *filename, TBOOLEAN recursive));

/* A copy of the declaration from set.c */
/* There should only be one declaration in a header file. But I do not know
 * where to put it */
/* void get_position __PROTO((struct position * pos)); */

/* State information for load_file(), to recover from errors
 * and properly handle recursive load_file calls
 */
LFS *lf_head = NULL;		/* NULL if not in load_file */

/* these two could be in load_file, except for error recovery */
static TBOOLEAN do_load_arg_substitution = FALSE;
static char *call_args[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

/*
 * iso_alloc() allocates a iso_curve structure that can hold 'num'
 * points.
 */
struct iso_curve *
iso_alloc(int num)
{
    struct iso_curve *ip;
    ip = (struct iso_curve *) gp_alloc(sizeof(struct iso_curve), "iso curve");
    ip->p_max = (num >= 0 ? num : 0);
    if (num > 0) {
	ip->points = (struct coordinate GPHUGE *)
	    gp_alloc(num * sizeof(struct coordinate), "iso curve points");
    } else
	ip->points = (struct coordinate GPHUGE *) NULL;
    ip->next = NULL;
    return (ip);
}

/*
 * iso_extend() reallocates a iso_curve structure to hold "num"
 * points. This will either expand or shrink the storage.
 */
void
iso_extend(struct iso_curve *ip, int num)
{
    if (num == ip->p_max)
	return;

#if defined(DOS16) || defined(WIN16)
    /* Make sure we do not allocate more than 64k points in msdos since
       * indexing is done with 16-bit int
       * Leave some bytes for malloc maintainance.
     */
    if (num > 32700)
	int_error(NO_CARET, "Array index must be less than 32k in msdos");
#endif /* 16bit (Win)Doze */

    if (num > 0) {
	if (ip->points == NULL) {
	    ip->points = (struct coordinate GPHUGE *)
		gp_alloc(num * sizeof(struct coordinate), "iso curve points");
	} else {
	    ip->points = (struct coordinate GPHUGE *)
		gp_realloc(ip->points, num * sizeof(struct coordinate), "expanding curve points");
	}
	ip->p_max = num;
    } else {
	if (ip->points != (struct coordinate GPHUGE *) NULL)
	    free(ip->points);
	ip->points = (struct coordinate GPHUGE *) NULL;
	ip->p_max = 0;
    }
}

/*
 * iso_free() releases any memory which was previously malloc()'d to hold
 *   iso curve points.
 */
void
iso_free(struct iso_curve *ip)
{
    if (ip) {
	if (ip->points)
	    free((char *) ip->points);
	free((char *) ip);
    }
}

void
load_file(FILE *fp, char *name, TBOOLEAN can_do_args)
{
    int len;

    int start, left;
    int more;
    int stop = FALSE;

    lf_push(fp);		/* save state for errors and recursion */
    do_load_arg_substitution = can_do_args;

    if (fp == (FILE *) NULL) {
	os_error(c_token, "Cannot open %s file '%s'",
		 can_do_args ? "call" : "load", name);
    } else if (fp == stdin) {
	/* DBT 10-6-98  go interactive if "-" named as load file */
	interactive = TRUE;
	while (!com_line());
    } else {
	/* go into non-interactive mode during load */
	/* will be undone below, or in load_file_error */
	int argc = 0; /* number arguments passed by "call" */
	interactive = FALSE;
	inline_num = 0;
	infile_name = name;

	if (can_do_args) {
	    while (c_token < num_tokens && argc <= 9) {
		if (isstring(c_token))
		    m_quote_capture(&call_args[argc++], c_token, c_token);
		else
		    m_capture(&call_args[argc++], c_token, c_token);
		c_token++;
	    }
	    /* Gnuplot "call" command can have up to 10 arguments named "$0"
	       to "$9". After reading the 10th argument (i.e., "$9") the
	       variable 'argc' equals 10.
	       Also, "call" must be the last command on the command line.
	    */
	    if (c_token < num_tokens)
		int_error(++c_token, "too many arguments for 'call <file>'");
	}
	while (!stop) {		/* read all commands in file */
	    /* read one command */
	    left = gp_input_line_len;
	    start = 0;
	    more = TRUE;

	    while (more) {
		if (fgets(&(gp_input_line[start]), left, fp) == (char *) NULL) {
		    stop = TRUE;	/* EOF in file */
		    gp_input_line[start] = '\0';
		    more = FALSE;
		} else {
		    inline_num++;
		    len = strlen(gp_input_line) - 1;
		    if (gp_input_line[len] == '\n') {	/* remove any newline */
			gp_input_line[len] = '\0';
			/* Look, len was 1-1 = 0 before, take care here! */
			if (len > 0)
			    --len;
			if (gp_input_line[len] == '\r') {	/* remove any carriage return */
			    gp_input_line[len] = NUL;
			    if (len > 0)
				--len;
			}
		    } else if (len + 2 >= left) {
			extend_input_line();
			left = gp_input_line_len - len - 1;
			start = len + 1;
			continue;	/* don't check for '\' */
		    }
		    if (gp_input_line[len] == '\\') {
			/* line continuation */
			start = len;
			left = gp_input_line_len - start;
		    } else
			more = FALSE;
		}
	    }

	    if (strlen(gp_input_line) > 0) {
		if (can_do_args) {
		    int il = 0;
		    char *rl;
		    char *raw_line = gp_strdup(gp_input_line);

		    rl = raw_line;
		    *gp_input_line = '\0';
		    while (*rl) {
			int aix;
			if (*rl == '$'
			    && ((aix = *(++rl)) != 0)	/* HBB 980308: quiet BCC warning */
			    && ((aix >= '0' && aix <= '9') || aix == '#')) {
			    if (aix == '#') {
				/* replace $# by number of passed arguments */
				len = argc < 10 ? 1 : 2; /* argc can be 0 .. 10 */
				while (gp_input_line_len - il < len + 1) {
				    extend_input_line();
				}
				sprintf(gp_input_line + il, "%i", argc);
				il += len;
			    } else
			    if (call_args[aix -= '0']) {
				/* replace $n for n=0..9 by the passed argument */
				len = strlen(call_args[aix]);
				while (gp_input_line_len - il < len + 1) {
				    extend_input_line();
				}
				strcpy(gp_input_line + il, call_args[aix]);
				il += len;
			    }
			} else {
			    /* substitute for $<n> here */
			    if (il + 1 > gp_input_line_len) {
				extend_input_line();
			    }
			    gp_input_line[il++] = *rl;
			}
			rl++;
		    }
		    if (il + 1 > gp_input_line_len) {
			extend_input_line();
		    }
		    gp_input_line[il] = '\0';
		    free(raw_line);
		}
		screen_ok = FALSE;	/* make sure command line is
					   echoed on error */
		if (do_line())
		    stop = TRUE;
	    }
	}
    }

    /* pop state */
    (void) lf_pop();		/* also closes file fp */
}

/* pop from load_file state stack
   FALSE if stack was empty
   called by load_file and load_file_error */
static TBOOLEAN
lf_pop()
{
    LFS *lf;

    if (lf_head == NULL)
	return (FALSE);
    else {
	int argindex;
	lf = lf_head;
	if (lf->fp != (FILE *) NULL && lf->fp != stdin) {
	    /* DBT 10-6-98  do not close stdin in the case
	     * that "-" is named as a load file
	     */
	    (void) fclose(lf->fp);
	}
	for (argindex = 0; argindex < 10; argindex++) {
	    if (call_args[argindex]) {
		free(call_args[argindex]);
	    }
	    call_args[argindex] = lf->call_args[argindex];
	}
	do_load_arg_substitution = lf->do_load_arg_substitution;
	interactive = lf->interactive;
	inline_num = lf->inline_num;
	infile_name = lf->name;
	lf_head = lf->prev;
	free((char *) lf);
	return (TRUE);
    }
}

/* push onto load_file state stack
   essentially, we save information needed to undo the load_file changes
   called by load_file */
static void
lf_push(FILE *fp)
{
    LFS *lf;
    int argindex;

    lf = (LFS *) gp_alloc(sizeof(LFS), (char *) NULL);
    if (lf == (LFS *) NULL) {
	if (fp != (FILE *) NULL)
	    (void) fclose(fp);	/* it won't be otherwise */
	int_error(c_token, "not enough memory to load file");
    }
    lf->fp = fp;		/* save this file pointer */
    lf->name = infile_name;	/* save current name */
    lf->interactive = interactive;	/* save current state */
    lf->inline_num = inline_num;	/* save current line number */
    lf->do_load_arg_substitution = do_load_arg_substitution;
    for (argindex = 0; argindex < 10; argindex++) {
	lf->call_args[argindex] = call_args[argindex];
	call_args[argindex] = NULL;	/* initially no args */
    }
    lf->prev = lf_head;		/* link to stack */
    lf_head = lf;
}

/* used for reread  vsnyder@math.jpl.nasa.gov */
FILE *
lf_top()
{
    if (lf_head == (LFS *) NULL)
	return ((FILE *) NULL);
    return (lf_head->fp);
}

/* called from main */
void
load_file_error()
{
    /* clean up from error in load_file */
    /* pop off everything on stack */
    while (lf_pop());
}

/* find max len of keys and count keys with len > 0 */

/* FIXME HBB 2000508: by design, this one belongs into 'graphics', and the
 * next to into 'graph3d'. Actually, the existence of a module like this
 * 'misc' is almost always a sign of bad design, IMHO */
/* may return NULL */
FILE *
loadpath_fopen(const char *filename, const char *mode)
{
    FILE *fp;

#if defined(PIPES)
    if (*filename == '<') {
	if ((fp = popen(filename + 1, "r")) == (FILE *) NULL)
	    return (FILE *) 0;
    } else
#endif /* PIPES */
    if ((fp = fopen(filename, mode)) == (FILE *) NULL) {
	/* try 'loadpath' variable */
	char *fullname = NULL, *path;

	while ((path = get_loadpath()) != NULL) {
	    /* length of path, dir separator, filename, \0 */
	    fullname = gp_realloc(fullname, strlen(path) + 1 + strlen(filename) + 1, "loadpath_fopen");
	    strcpy(fullname, path);
	    PATH_CONCAT(fullname, filename);
	    if ((fp = fopen(fullname, mode)) != NULL) {
		free(fullname);
		fullname = NULL;
		/* reset loadpath internals!
		 * maybe this can be replaced by calling get_loadpath with
		 * a NULL argument and some loadpath_handler internal logic */
		while (get_loadpath());
		break;
	    }
	}

	if (fullname)
	    free(fullname);

    }

    return fp;
}


/* Harald Harders <h.harders@tu-bs.de> */
/* Thanks to John Bollinger <jab@bollingerbands.com> who has tested the
   windows part */
static char *
recursivefullname(const char *path, const char *filename, TBOOLEAN recursive)
{
    char *fullname = NULL;
    FILE *fp;

    /* length of path, dir separator, filename, \0 */
    fullname = gp_alloc(strlen(path) + 1 + strlen(filename) + 1,
			"recursivefullname");
    strcpy(fullname, path);
    PATH_CONCAT(fullname, filename);

    if ((fp = fopen(fullname, "r")) != NULL) {
	fclose(fp);
	return fullname;
    } else {
	free(fullname);
	fullname = NULL;
    }

    if (recursive) {
#ifdef HAVE_DIRENT_H
	DIR *dir;
	struct dirent *direntry;
	struct stat buf;

	dir = opendir(path);
	if (dir) {
	    while ((direntry = readdir(dir)) != NULL) {
		char *fulldir = gp_alloc(strlen(path) + 1 + strlen(direntry->d_name) + 1,
					 "fontpath_fullname");
		strcpy(fulldir, path);
#  if defined(VMS)
		if (fulldir[strlen(fulldir) - 1] == ']')
		    fulldir[strlen(fulldir) - 1] = '\0';
		strcpy(&(fulldir[strlen(fulldir)]), ".");
		strcpy(&(fulldir[strlen(fulldir)]), direntry->d_name);
		strcpy(&(fulldir[strlen(fulldir)]), "]");
#  else
		PATH_CONCAT(fulldir, direntry->d_name);
#  endif
		stat(fulldir, &buf);
		if ((S_ISDIR(buf.st_mode)) &&
		    (strcmp(direntry->d_name, ".") != 0) &&
		    (strcmp(direntry->d_name, "..") != 0)) {
		    fullname = recursivefullname(fulldir, filename, TRUE);
		    if (fullname != NULL)
			break;
		}
		free(fulldir);
	    }
	    closedir(dir);
	}
#elif defined(_Windows)
	HANDLE filehandle;
	WIN32_FIND_DATA finddata;
	char *pathwildcard = gp_alloc(strlen(path) + 2, "fontpath_fullname");

	strcpy(pathwildcard, path);
	PATH_CONCAT(pathwildcard, "*");

	filehandle = FindFirstFile(pathwildcard, &finddata);
	free(pathwildcard);
	if (filehandle != INVALID_HANDLE_VALUE)
	    do {
		if ((finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
		    (strcmp(finddata.cFileName, ".") != 0) &&
		    (strcmp(finddata.cFileName, "..") != 0)) {
		    char *fulldir = gp_alloc(strlen(path) + 1 + strlen(finddata.cFileName) + 1,
					     "fontpath_fullname");
		    strcpy(fulldir, path);
		    PATH_CONCAT(fulldir, finddata.cFileName);

		    fullname = recursivefullname(fulldir, filename, TRUE);
		    free(fulldir);
		    if (fullname != NULL)
			break;
		}
	    } while (FindNextFile(filehandle, &finddata) != 0);
	FindClose(filehandle);

#else
	int_warn(NO_CARET, "Recursive directory search not supported\n\t('%s!')", path);
#endif
    }
    return fullname;
}


/* may return NULL */
char *
fontpath_fullname(const char *filename)
{
    FILE *fp;
    char *fullname = NULL;

#if defined(PIPES)
    if (*filename == '<') {
	os_error(NO_CARET, "fontpath_fullname: No Pipe allowed");
    } else
#endif /* PIPES */
    if ((fp = fopen(filename, "r")) == (FILE *) NULL) {
	/* try 'fontpath' variable */
	char *tmppath, *path = NULL;

	while ((tmppath = get_fontpath()) != NULL) {
	    TBOOLEAN subdirs = FALSE;
	    path = gp_strdup(tmppath);
	    if (path[strlen(path) - 1] == '!') {
		path[strlen(path) - 1] = '\0';
		subdirs = TRUE;
	    }			/* if */
	    fullname = recursivefullname(path, filename, subdirs);
	    if (fullname != NULL) {
		while (get_fontpath());
		free(path);
		break;
	    }
	    free(path);
	}

    } else
	fullname = gp_strdup(filename);

    return fullname;
}


/* Push current terminal.
 * Called 1. in main(), just after init_terminal(),
 *        2. from load_rcfile(),
 *        3. anytime by user command "set term push".
 */
static char *push_term_name = NULL;
static char *push_term_opts = NULL;

void
push_terminal(int is_interactive)
{
    if (term) {
	free(push_term_name);
	free(push_term_opts);
	push_term_name = gp_strdup(term->name);
	push_term_opts = gp_strdup(term_options);
	if (is_interactive)
	    fprintf(stderr, "   pushed terminal %s %s\n", push_term_name, push_term_opts);
    } else {
	if (is_interactive)
	    fputs("\tcurrent terminal type is unknown\n", stderr);
    }
}

/* Pop the terminal.
 * Called anytime by user command "set term pop".
 */
void
pop_terminal()
{
    if (push_term_name != NULL) {
	char *s;
	int i = strlen(push_term_name) + 11;
	if (push_term_opts) {
	    /* do_string() does not like backslashes -- thus remove them */
	    for (s=push_term_opts; *s; s++)
		if (*s=='\\' || *s=='\n') *s=' ';
	    i += strlen(push_term_opts);
	}
	s = gp_alloc(i, "pop");
	i = interactive;
	interactive = 0;
	sprintf(s,"set term %s %s", push_term_name, (push_term_opts ? push_term_opts : ""));
	do_string(s);
	free(s);
	interactive = i;
	if (interactive)
	    fprintf(stderr,"   restored terminal is %s %s\n", term->name, ((*term_options) ? term_options : ""));
    } else
	fprintf(stderr,"No terminal has been pushed yet\n");
}


/* Parse a plot style. Used by 'set style {data|function}' and by
 * (s)plot.  */
enum PLOT_STYLE
get_style()
{
    /* defined in plot.h */
    enum PLOT_STYLE ps;

    c_token++;

    ps = lookup_table(&plotstyle_tbl[0], c_token);

    c_token++;

    if (ps == -1) {
	int_error(c_token, "\
expecting 'lines', 'points', 'linespoints', 'dots', 'impulses',\n\
\t'yerrorbars', 'xerrorbars', 'xyerrorbars', 'steps', 'fsteps',\n\
\t'histeps', 'filledcurves', 'boxes', 'boxerrorbars', 'boxxyerrorbars',\n\
\t'vectors', 'financebars', 'candlesticks', 'errorlines', 'xerrorlines',\n\
\t'yerrorlines', 'xyerrorlines', 'pm3d', 'labels', 'histograms'"
#ifdef WITH_IMAGE
",\n\t 'image', 'rgbimage'"
#endif
);
	ps = LINES;
    }

    return ps;
}

/* Parse options for style filledcurves and fill fco accordingly.
 * If no option given, then set fco->opt_given to 0.
 */
void
get_filledcurves_style_options(filledcurves_opts *fco)
{
    int p;
    p = lookup_table(&filledcurves_opts_tbl[0], c_token);

    if (p == FILLEDCURVES_ABOVE) {
	fco->oneside = 1;
	p = lookup_table(&filledcurves_opts_tbl[0], ++c_token);
    } else if (p == FILLEDCURVES_BELOW) {
	fco->oneside = -1;
	p = lookup_table(&filledcurves_opts_tbl[0], ++c_token);
    } else
	fco->oneside = 0;

    if (p == -1) {
	fco->opt_given = 0;
	return;			/* no option given */
    } else
	fco->opt_given = 1;

    c_token++;

    fco->closeto = p;
    if (!equals(c_token, "="))
	return;
    /* parameter required for filledcurves x1=... and friends */
    if (p != FILLEDCURVES_ATXY)
	fco->closeto += 4;
    c_token++;
    fco->at = real_expression();
    if (p != FILLEDCURVES_ATXY)
	return;
    /* two values required for FILLEDCURVES_ATXY */
    if (!equals(c_token, ","))
	int_error(c_token, "syntax is xy=<x>,<y>");
    c_token++;
    fco->aty = real_expression();
    return;
}

/* Print filledcurves style options to a file (used by 'show' and 'save'
 * commands).
 */
void
filledcurves_options_tofile(filledcurves_opts *fco, FILE *fp)
{
    if (!fco->opt_given)
	return;
    if (fco->oneside)
	fputs(fco->oneside > 0 ? "above " : "below ", fp);
    if (fco->closeto == FILLEDCURVES_CLOSED) {
	fputs("closed", fp);
	return;
    }
    if (fco->closeto <= FILLEDCURVES_Y2) {
	fputs(filledcurves_opts_tbl[fco->closeto].key, fp);
	return;
    }
    if (fco->closeto <= FILLEDCURVES_ATY2) {
	fprintf(fp, "%s=%g", filledcurves_opts_tbl[fco->closeto - 4].key, fco->at);
	return;
    }
    if (fco->closeto == FILLEDCURVES_ATXY) {
	fprintf(fp, "xy=%g,%g", fco->at, fco->aty);
	return;
    }
}

/* line/point parsing...
 *
 * allow_ls controls whether we are allowed to accept linestyle in
 * the current context [ie not when doing a  set linestyle command]
 * allow_point is whether we accept a point command
 * We assume compiler will optimise away if(0) or if(1)
 */

void
lp_use_properties(struct lp_style_type *lp, int tag, int pointflag)
{
    /*  This function looks for a linestyle defined by 'tag' and copies
     *  its data into the structure 'lp'.
     *
     *  If 'pointflag' equals ZERO, the properties belong to a linestyle
     *  used with arrows.  In this case no point properties will be
     *  passed to the terminal (cf. function 'term_apply_lp_properties' below).
     */

    struct linestyle_def *this;

    this = first_linestyle;
    while (this != NULL) {
	if (this->tag == tag) {
	    *lp = this->lp_properties;
	    /* FIXME - It would be nicer if this were always true already */
	    if (!lp->use_palette) {
		lp->pm3d_color.type = TC_LT;
		lp->pm3d_color.lt = lp->l_type;
	    }
	    lp->pointflag = pointflag;
	    return;
	} else {
	    this = this->next;
	}
    }

    /* tag not found: */
    /* Mar 2006 - This used to be a fatal error; now we fall back to line type */
    lp->l_type = tag - 1;
    lp->pm3d_color.type = TC_LT;
    lp->pm3d_color.lt = lp->l_type;
}


/* allow any order of options - pm 24.11.2001 */
/* EAM Oct 2005 - Require that default values have been placed in lp by the caller */
void
lp_parse(struct lp_style_type *lp, TBOOLEAN allow_ls, TBOOLEAN allow_point)
{
    if (allow_ls &&
	(almost_equals(c_token, "lines$tyle") || equals(c_token, "ls"))) {
	c_token++;
	lp_use_properties(lp, int_expression(), allow_point);
    } else {
	/* avoid duplicating options */
	int set_lt = 0, set_pal = 0, set_lw = 0, set_pt = 0, set_ps = 0;

	lp->pointflag = allow_point;

	while (!END_OF_COMMAND) {
	    if (almost_equals(c_token, "linet$ype") || equals(c_token, "lt")) {
		if (set_lt++)
		    break;
		c_token++;
		if (almost_equals(c_token, "rgb$color")) {
		    if (set_pal++)
			break;
		    c_token--;
		    parse_colorspec(&lp->pm3d_color, TC_RGB);
		    lp->use_palette = 1;
		} else
		/* both syntaxes allowed: 'with lt pal' as well as 'with pal' */
		if (almost_equals(c_token, "pal$ette")) {
		    if (set_pal++)
			break;
		    c_token--;
		    parse_colorspec(&lp->pm3d_color, TC_Z);
		    lp->use_palette = 1;
#ifdef KEYWORD_BGND
		} else if (equals(c_token,"bgnd")) {
		    lp->l_type = LT_BACKGROUND;
		    c_token++;
#endif
		} else {
		    int lt = int_expression();
		    lp->l_type = lt - 1;
		    /* user may prefer explicit line styles */
		    if (prefer_line_styles && allow_ls)
			lp_use_properties(lp, lt, TRUE);
		}
	    } /* linetype, lt */

	    /* both syntaxes allowed: 'with lt pal' as well as 'with pal' */
	    if (almost_equals(c_token, "pal$ette")) {
		if (set_pal++)
		    break;
		c_token--;
		parse_colorspec(&lp->pm3d_color, TC_Z);
		lp->use_palette = 1;
		continue;
	    }

	    if (equals(c_token,"lc") || almost_equals(c_token,"linec$olor")) {
		lp->use_palette = 1;
		if (set_pal++)
		    break;
		c_token++;
		if (almost_equals(c_token, "rgb$color")) {
		    c_token--;
		    parse_colorspec(&lp->pm3d_color, TC_RGB);
		} else if (almost_equals(c_token, "pal$ette")) {
		    c_token--;
		    parse_colorspec(&lp->pm3d_color, TC_Z);
#ifdef KEYWORD_BGND
		} else if (equals(c_token,"bgnd")) {
		    lp->pm3d_color.type = TC_LT;
		    lp->pm3d_color.lt = LT_BACKGROUND;
		    c_token++;
#endif
		} else {
		    lp->pm3d_color.type = TC_LT;
		    lp->pm3d_color.lt = int_expression() - 1;
		}
		continue;
	    }

	    if (almost_equals(c_token, "linew$idth") || equals(c_token, "lw")) {
		if (set_lw++)
		    break;
		c_token++;
		lp->l_width = real_expression();
		if (lp->l_width < 0)
		    lp->l_width = 0;
		continue;
	    }

	    if (equals(c_token,"bgnd")) {
		lp->l_type = LT_BACKGROUND;
		lp->use_palette = 0;
		c_token++;
		continue;
	    }

	    /* HBB 20010622: restructured to allow for more descriptive
	     * error message, here. Would otherwise only print out
	     * 'undefined variable: pointtype' --- rather unhelpful. */
	    if (almost_equals(c_token, "pointt$ype") || equals(c_token, "pt")) {
		if (allow_point) {
		    if (set_pt++)
			break;
		    c_token++;
		    lp->p_type = int_expression() - 1;
		} else {
		    int_warn(c_token, "No pointtype specifier allowed, here");
		    c_token += 2;
		}
		continue;
	    }

	    if (almost_equals(c_token, "points$ize") || equals(c_token, "ps")) {
		if (allow_point) {
		    if (set_ps++)
			break;
		    c_token++;
		    if (almost_equals(c_token, "var$iable")) {
			lp->p_size = PTSZ_VARIABLE;
			c_token++;
		    } else if (almost_equals(c_token, "def$ault")) {
			lp->p_size = PTSZ_DEFAULT;
			c_token++;
		    } else {
			lp->p_size = real_expression();
			if (lp->p_size < 0)
			    lp->p_size = 0;
		    }
		} else {
		    int_warn(c_token, "No pointsize specifier allowed, here");
		    c_token += 2;
		}
		continue;
	    }

	    /* unknown option catched -> quit the while(1) loop */
	    break;
	}

	if (set_lt > 1 || set_pal > 1 || set_lw > 1 || set_pt > 1 || set_ps > 1)
	    int_error(c_token, "duplicated arguments in style specification");

#if defined(__FILE__) && defined(__LINE__) && defined(DEBUG_LP)
	fprintf(stderr,
		"lp_properties at %s:%d : lt: %d, lw: %.3f, pt: %d, ps: %.3f\n",
		__FILE__, __LINE__, lp->l_type, lp->l_width, lp->p_type,
		lp->p_size);
#endif
    }
}

/* <fillstyle> = {empty | solid {<density>} | pattern {<n>}} {noborder | border {<lt>}} */
void
parse_fillstyle(struct fill_style_type *fs, int def_style, int def_density, int def_pattern, int def_bordertype)
{
    TBOOLEAN set_fill = FALSE;
    TBOOLEAN set_param = FALSE;
    TBOOLEAN transparent = FALSE;

    /* Set defaults */
    fs->fillstyle = def_style;
    fs->filldensity = def_density;
    fs->fillpattern = def_pattern;
    fs->border_linetype = def_bordertype;

    if (END_OF_COMMAND)
	return;
    if (!equals(c_token, "fs") && !almost_equals(c_token, "fill$style"))
	return;
    c_token++;

    while (!END_OF_COMMAND) {
	if (almost_equals(c_token, "trans$parent")) {
	    transparent = TRUE;
	    c_token++;
	}

	if (almost_equals(c_token, "e$mpty")) {
	    fs->fillstyle = FS_EMPTY;
	    c_token++;
	} else if (almost_equals(c_token, "s$olid")) {
	    fs->fillstyle = transparent ? FS_TRANSPARENT_SOLID : FS_SOLID;
	    set_fill = TRUE;
	    c_token++;
	} else if (almost_equals(c_token, "p$attern")) {
	    fs->fillstyle = transparent ? FS_TRANSPARENT_PATTERN : FS_PATTERN;
	    set_fill = TRUE;
	    c_token++;
	}

	if (END_OF_COMMAND)
	    continue;
	else if (almost_equals(c_token, "bo$rder")) {
	    fs->border_linetype = LT_UNDEFINED;
	    c_token++;
	    /* FIXME EAM - isanumber really means `is a positive number` */
	    if (isanumber(c_token) ||
		(equals(c_token, "-") && isanumber(c_token + 1))) {
		fs->border_linetype = int_expression() - 1;
	    }
	    continue;
	} else if (almost_equals(c_token, "nobo$rder")) {
	    fs->border_linetype = LT_NODRAW;
	    c_token++;
	    continue;
	}
	/* We hit something unexpected */
	else if (!set_fill || !isanumber(c_token) || set_param)
	    break;

	if (fs->fillstyle == FS_SOLID || fs->fillstyle == FS_TRANSPARENT_SOLID) {
	    /* user sets 0...1, but is stored as an integer 0..100 */
	    fs->filldensity = 100.0 * real_expression() + 0.5;
	    if (fs->filldensity < 0)
		fs->filldensity = 0;
	    if (fs->filldensity > 100)
		fs->filldensity = 100;
	    set_param = TRUE;
	} else if (fs->fillstyle == FS_PATTERN || fs->fillstyle == FS_TRANSPARENT_PATTERN) {
	    fs->fillpattern = int_expression();
	    if (fs->fillpattern < 0)
		fs->fillpattern = 0;
	    set_param = TRUE;
	}
    }
}

/*
 * Parse the sub-options of text color specification
 *   { def$ault | lt <linetype> | pal$ette { cb <val> | frac$tion <val> | z }
 * The ordering of alternatives shown in the line above is kept in the symbol definitions
 * TC_DEFAULT TC_LT TC_RGB TC_CB TC_FRAC TC_Z  (0 1 2 3 4 5)
 * and the "options" parameter to parse_colorspec limits legal input to the
 * corresponding point in the series. So TC_LT allows only default or linetype
 * coloring, while TC_Z allows all coloring options up to and including pal z
 */
void
parse_colorspec(struct t_colorspec *tc, int options)
{
    c_token++;
    if (END_OF_COMMAND)
	int_error(c_token, "expected colorspec");
    if (almost_equals(c_token,"def$ault")) {
	c_token++;
	tc->type = TC_DEFAULT;
#ifdef KEYWORD_BGND
    } else if (equals(c_token,"bgnd")) {
	c_token++;
	tc->type = TC_LT;
	tc->lt = LT_BACKGROUND;
#endif
    } else if (equals(c_token,"lt")) {
	c_token++;
	if (END_OF_COMMAND)
	    int_error(c_token, "expected linetype");
	tc->type = TC_LT;
	tc->lt = int_expression()-1;
	if (tc->lt < LT_BACKGROUND) {
	    tc->type = TC_DEFAULT;
	    int_warn(c_token,"illegal linetype");
	}
    } else if (options <= TC_LT) {
	tc->type = TC_DEFAULT;
	int_error(c_token, "only tc lt <n> possible here");
    } else if (equals(c_token,"ls") || almost_equals(c_token,"lines$tyle")) {
	c_token++;
	tc->type = TC_LINESTYLE;
	tc->lt = real_expression();
    } else if (almost_equals(c_token,"rgb$color")) {
	char *color;
	int rgbtriple;
	c_token++;
	tc->type = TC_RGB;
	if (almost_equals(c_token, "var$iable")) {
	    tc->value = -1.0;
	    c_token++;
	    return;
	} else
	    tc->value = 0.0;
	if (!(color = try_to_get_string()))
	    int_error(c_token, "expected a color name or a string of form \"#RRGGBB\"");
	if ((rgbtriple = lookup_table_nth(pm3d_color_names_tbl, color)) >= 0)
	    rgbtriple = pm3d_color_names_tbl[rgbtriple].value;
	else
	    sscanf(color,"#%x",&rgbtriple);
	free(color);
	if (rgbtriple < 0)
	    int_error(c_token, "expected a known color name or a string of form \"#RRGGBB\"");
	tc->type = TC_RGB;
	tc->lt = rgbtriple;
    } else if (almost_equals(c_token,"pal$ette")) {
	c_token++;
	if (equals(c_token,"z")) {
	    /* The actual z value is not yet known, fill it in later */
	    if (options >= TC_Z) {
		tc->type = TC_Z;
	    } else {
		tc->type = TC_DEFAULT;
		int_error(c_token, "palette z not possible here");
	    }
	    c_token++;
	} else if (equals(c_token,"cb")) {
	    tc->type = TC_CB;
	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "expected cb value");
	    tc->value = real_expression();
	} else if (almost_equals(c_token,"frac$tion")) {
	    tc->type = TC_FRAC;
	    c_token++;
	    if (END_OF_COMMAND)
		int_error(c_token, "expected palette fraction");
	    tc->value = real_expression();
	    if (tc->value < 0. || tc->value > 1.0)
		int_error(c_token, "palette fraction out of range");
	} else {
	    /* END_OF_COMMAND or palette <blank> */
	    if (options >= TC_Z)
		tc->type = TC_Z;
	}
    } else {
	int_error(c_token, "colorspec option not recognized");
    }
}

/* arrow parsing...
 *
 * allow_as controls whether we are allowed to accept arrowstyle in
 * the current context [ie not when doing a  set style arrow command]
 */

static void
arrow_use_properties(struct arrow_style_type *arrow, int tag)
{
    /*  This function looks for an arrowstyle defined by 'tag' and
     *  copies its data into the structure 'ap'. */
    struct arrowstyle_def *this;

    this = first_arrowstyle;
    while (this != NULL) {
	if (this->tag == tag) {
	    *arrow = this->arrow_properties;
	    return;
	} else {
	    this = this->next;
	}
    }

    /* tag not found: */
    int_error(NO_CARET,"arrowstyle not found", NO_CARET);
}

void
arrow_parse(
    struct arrow_style_type *arrow,
    TBOOLEAN allow_as)
{
    if (allow_as && (almost_equals(c_token, "arrows$tyle") ||
		     equals(c_token, "as"))) {
	c_token++;
	arrow_use_properties(arrow, int_expression());
    } else {
	/* avoid duplicating options */
	int set_layer=0, set_line=0, set_head=0;
	int set_headsize=0, set_headfilled=0;

	while (!END_OF_COMMAND) {
	    if (equals(c_token, "nohead")) {
		if (set_head++)
		    break;
		c_token++;
		arrow->head = NOHEAD;
		continue;
	    }
	    if (equals(c_token, "head")) {
		if (set_head++)
		    break;
		c_token++;
		arrow->head = END_HEAD;
		continue;
	    }
	    if (equals(c_token, "backhead")) {
		if (set_head++)
		    break;
		c_token++;
		arrow->head = BACKHEAD;
		continue;
	    }
	    if (equals(c_token, "heads")) {
		if (set_head++)
		    break;
		c_token++;
		arrow->head = BACKHEAD | END_HEAD;
		continue;
	    }

	    if (almost_equals(c_token, "fill$ed")) {
		if (set_headfilled++)
		    break;
		c_token++;
		arrow->head_filled = 2;
		continue;
	    }
	    if (almost_equals(c_token, "empty")) {
		if (set_headfilled++)
		    break;
		c_token++;
		arrow->head_filled = 1;
		continue;
	    }
	    if (almost_equals(c_token, "nofill$ed")) {
		if (set_headfilled++)
		    break;
		c_token++;
		arrow->head_filled = 0;
		continue;
	    }

	    if (equals(c_token, "size")) {
		struct position hsize;
		if (set_headsize++)
		    break;
		hsize.scalex = hsize.scaley = hsize.scalez = first_axes;
		/* only scalex used; scaley is angle of the head in [deg] */
		c_token++;
		if (END_OF_COMMAND)
		    int_error(c_token, "head size expected");
		get_position(&hsize);
		arrow->head_length = hsize.x;
		arrow->head_lengthunit = hsize.scalex;
		arrow->head_angle = hsize.y;
		arrow->head_backangle = hsize.z;
		/* invalid backangle --> default of 90.0° */
		if (arrow->head_backangle <= arrow->head_angle)
		    arrow->head_backangle = 90.0;
		continue;
	    }

	    if (equals(c_token, "back")) {
		if (set_layer++)
		    break;
		c_token++;
		arrow->layer = 0;
		continue;
	    }
	    if (equals(c_token, "front")) {
		if (set_layer++)
		    break;
		c_token++;
		arrow->layer = 1;
		continue;
	    }

	    /* pick up a line spec - allow ls, but no point. */
	    {
		int stored_token = c_token;
		lp_parse(&arrow->lp_properties, TRUE, FALSE);
		if (stored_token == c_token || set_line++)
		    break;
		continue;
	    }

	    /* unknown option caught -> quit the while(1) loop */
	    break;
	}

	if (set_layer>1 || set_line>1 || set_head>1 || set_headsize>1 || set_headfilled>1)
	    int_error(c_token, "duplicated arguments in style specification");

#if defined(__FILE__) && defined(__LINE__) && defined(DEBUG_LP)
	arrow->layer = 0;
	arrow->lp_properties = tmp_lp_style;
	arrow->head = 1;
	arrow->head_length = 0.0;
	arrow->head_lengthunit = first_axes;
	arrow->head_angle = 15.0;
	arrow->head_backangle = 90.0;
	arrow->head_filled = 0;
	fprintf(stderr,
		"arrow_properties at %s:%d : layer: %d, lt: %d, lw: %.3f, head: %d, headlength/unit: %.3f/%d, headangles: %.3f/%.3f, headfilled %d\n",
		__FILE__, __LINE__, arrow->layer, arrow->lp_properties.l_type,
		arrow->lp_properties.l_width, arrow->head, arrow->head_length,
		arrow->head_lengthunit, arrow->head_angle,
		arrow->head_backangle, arrow->head_filled);
#endif
    }
}

#ifdef WITH_IMAGE
void
get_image_options(t_image *image)
{
    if (equals(c_token, "failsafe")) {
	c_token++;
	image->fallback = TRUE;
    }

}
#endif
