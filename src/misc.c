#ifndef lint
static char *RCSid() { return RCSid("$Id: misc.c,v 1.34 2002/01/06 16:31:12 mikulik Exp $"); }
#endif

/* GNUPLOT - misc.c */

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

#include "misc.h"

#include "alloc.h"
#include "command.h"
#include "graphics.h"
#include "parse.h"		/* for const_express() */
#include "plot.h"
#include "tables.h"
#include "util.h"
#include "variable.h"

/* name of command file; NULL if terminal */
char *infile_name = NULL;

static TBOOLEAN lf_pop __PROTO((void));
static void lf_push __PROTO((FILE * fp));

/* State information for load_file(), to recover from errors
 * and properly handle recursive load_file calls
 */
typedef struct lf_state_struct {
    FILE *fp;			/* file pointer for load file */
    char *name;			/* name of file */
    TBOOLEAN interactive;	/* value of interactive flag on entry */
    TBOOLEAN do_load_arg_substitution;	/* likewise ... */
    int inline_num;		/* inline_num on entry */
    struct lf_state_struct *prev;			/* defines a stack */
    char *call_args[10];	/* args when file is 'call'ed instead of 'load'ed */
}  LFS;
static LFS *lf_head = NULL;		/* NULL if not in load_file */

/* these two could be in load_file, except for error recovery */
static TBOOLEAN do_load_arg_substitution = FALSE;
static char *call_args[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

/*
 * iso_alloc() allocates a iso_curve structure that can hold 'num'
 * points.
 */
struct iso_curve *
iso_alloc(num)
int num;
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
iso_extend(ip, num)
struct iso_curve *ip;
int num;
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
iso_free(ip)
struct iso_curve *ip;
{
    if (ip) {
	if (ip->points)
	    free((char *) ip->points);
	free((char *) ip);
    }
}

void
load_file(fp, name, can_do_args)
FILE *fp;
char *name;
TBOOLEAN can_do_args;
{
    register int len;

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
	interactive = FALSE;
	inline_num = 0;
	infile_name = name;

	if (can_do_args) {
	    int aix = 0;
	    while (++c_token < num_tokens && aix <= 9) {
		if (isstring(c_token))
		    m_quote_capture(&call_args[aix++], c_token, c_token);
		else
		    m_capture(&call_args[aix++], c_token, c_token);
	    }

/*         A GNUPLOT "call" command can have up to _10_ arguments named "$0"
   to "$9".  After reading the 10th argument (i.e.: "$9") the variable
   'aix' contains the value '10' because of the 'aix++' construction
   in '&call_args[aix++]'.  So I think the following test of 'aix' 
   should be done against '10' instead of '9'.                (JFi) */

/*              if (c_token >= num_tokens && aix > 9) */
	    if (c_token >= num_tokens && aix > 10)
		int_error(++c_token, "too many arguments for CALL <file>");
	}
	while (!stop) {		/* read all commands in file */
	    /* read one command */
	    left = input_line_len;
	    start = 0;
	    more = TRUE;

	    while (more) {
		if (fgets(&(input_line[start]), left, fp) == (char *) NULL) {
		    stop = TRUE;	/* EOF in file */
		    input_line[start] = '\0';
		    more = FALSE;
		} else {
		    inline_num++;
		    len = strlen(input_line) - 1;
		    if (input_line[len] == '\n') {	/* remove any newline */
			input_line[len] = '\0';
			/* Look, len was 1-1 = 0 before, take care here! */
			if (len > 0)
			    --len;
			if (input_line[len] == '\r') {	/* remove any carriage return */
			    input_line[len] = NUL;
			    if (len > 0)
				--len;
			}
		    } else if (len + 2 >= left) {
			extend_input_line();
			left = input_line_len - len - 1;
			start = len + 1;
			continue;	/* don't check for '\' */
		    }
		    if (input_line[len] == '\\') {
			/* line continuation */
			start = len;
			left = input_line_len - start;
		    } else
			more = FALSE;
		}
	    }

	    if (strlen(input_line) > 0) {
		if (can_do_args) {
		    register int il = 0;
		    register char *rl;
		    char *raw_line = gp_strdup(input_line);

		    rl = raw_line;
		    *input_line = '\0';
		    while (*rl) {
			register int aix;
			if (*rl == '$'
			    && ((aix = *(++rl)) != 0)	/* HBB 980308: quiet BCC warning */
			    &&aix >= '0' && aix <= '9') {
			    if (call_args[aix -= '0']) {
				len = strlen(call_args[aix]);
				while (input_line_len - il < len + 1) {
				    extend_input_line();
				}
				strcpy(input_line + il, call_args[aix]);
				il += len;
			    }
			} else {
			    /* substitute for $<n> here */
			    if (il + 1 > input_line_len) {
				extend_input_line();
			    }
			    input_line[il++] = *rl;
			}
			rl++;
		    }
		    if (il + 1 > input_line_len) {
			extend_input_line();
		    }
		    input_line[il] = '\0';
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

/* pop from load_file state stack */
static TBOOLEAN			/* FALSE if stack was empty */
lf_pop()
{				/* called by load_file and load_file_error */
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

/* push onto load_file state stack */
/* essentially, we save information needed to undo the load_file changes */
static void
lf_push(fp)			/* called by load_file */
FILE *fp;
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
loadpath_fopen(filename, mode)
const char *filename, *mode;
{
    FILE *fp;

#if defined(PIPES)
    if (*filename == '<') {
	if ((fp = popen(filename + 1, "r")) == (FILE *) NULL)
	    return (FILE*) 0;
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

/* Parse a plot style. Used by 'set style {data|function}' and by
 * (s)plot.  */
enum PLOT_STYLE
get_style()
{
    /* defined in plot.h */
    register enum PLOT_STYLE ps;

    c_token++;

    ps = lookup_table(&plotstyle_tbl[0],c_token);

    c_token++;

    if (ps == -1) {
#if USE_ULIG_FILLEDBOXES
	int_error(c_token,"\
expecting 'lines', 'points', 'linespoints', 'dots', 'impulses',\n\
\t'yerrorbars', 'xerrorbars', 'xyerrorbars', 'steps', 'fsteps',\n\
\t'histeps', 'filledcurves', 'boxes', 'filledboxes', 'boxerrorbars',\n\
\t'boxxyerrorbars', 'vectors', 'financebars', 'candlesticks',\n\
\t'errorlines', 'xerrorlines', 'yerrorlines', 'xyerrorlines'");
#else  /* USE_ULIG_FILLEDBOXES*/
	int_error(c_token,"\
expecting 'lines', 'points', 'linespoints', 'dots', 'impulses',\n\
\t'yerrorbars', 'xerrorbars', 'xyerrorbars', 'steps', 'fsteps',\n\
\t'histeps', 'filledcurves', 'boxes', 'boxerrorbars', 'boxxyerrorbars',\n\
\t'vector', 'financebars', 'candlesticks', 'errorlines', 'xerrorlines',\n\
\t'yerrorlines', 'xyerrorlines'");
#endif /* USE_ULIG_FILLEDBOXES */
	ps = LINES;
    }

    return ps;
}

#ifdef PM3D
/* Parse options for style filledcurves and fill fco accordingly.
 * If no option given, then set fco->opt_given to 0.
 */
void
get_filledcurves_style_options( filledcurves_opts *fco )
{
    int p;
    struct value a;
    p = lookup_table(&filledcurves_opts_tbl[0], c_token);
    fco->opt_given = (p != -1);
    if (p==-1) return; /* no option given */
    c_token++;
    fco->closeto = p;
    if (!equals(c_token,"=")) return;
    /* parameter required for filledcurves x1=... and friends */
    if (p!=FILLEDCURVES_ATXY) fco->closeto += 4;
    c_token++;
    fco->at = real(const_express(&a));
    if (p!=FILLEDCURVES_ATXY) return;
    /* two values required for FILLEDCURVES_ATXY */
    if (!equals(c_token,","))
	int_error(c_token, "syntax is xy=<x>,<y>");
    c_token++;
    fco->aty = real(const_express(&a));
    return;
}

/* Print filledcurves style options to a file (used by 'show' and 'save'
 * commands).
 */
void
filledcurves_options_tofile(fco, fp)
    filledcurves_opts *fco;
    FILE *fp;
{
    if (!fco->opt_given) return;
    if (fco->closeto == FILLEDCURVES_CLOSED) {
	fputs("closed", fp);
	return;
    }
    if (fco->closeto <= FILLEDCURVES_Y2) {
	fputs(filledcurves_opts_tbl[fco->closeto].key, fp);
	return;
    }
    if (fco->closeto <= FILLEDCURVES_ATY2) {
	fprintf(fp,"%s=%g",filledcurves_opts_tbl[fco->closeto-4].key,fco->at);
	return;
    }
    if (fco->closeto == FILLEDCURVES_ATXY) {
	fprintf(fp,"xy=%g,%g",fco->at,fco->aty);
	return;
    }
}
#endif

/* line/point parsing...
 *
 * allow_ls controls whether we are allowed to accept linestyle in
 * the current context [ie not when doing a  set linestyle command]
 * allow_point is whether we accept a point command
 * We assume compiler will optimise away if(0) or if(1)
 */

void
lp_use_properties(lp, tag, pointflag)
struct lp_style_type *lp;
int tag, pointflag;
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
	    lp->pointflag = pointflag;
	    return;
	} else {
	    this = this->next;
	}
    }

    /* tag not found: */
    int_error(NO_CARET,"linestyle not found", NO_CARET);
}


/* was a macro in plot.h */
/* allow any order of options - pm 24.11.2001 */
void
lp_parse(lp, allow_ls, allow_point, def_line, def_point)
    struct lp_style_type *lp;
    TBOOLEAN allow_ls, allow_point;
    int def_line, def_point;
{
    struct value t;

    if (allow_ls && (almost_equals(c_token, "lines$tyle") ||
		     equals(c_token, "ls"))) {
	c_token++;
	lp_use_properties(lp, (int) real(const_express(&t)), allow_point);
    } else {
	/* avoid duplicating options */
	int set_lt=0, set_pal=0, set_lw=0, set_pt=0, set_ps=0;
	/* set default values */
	lp->l_type = def_line;
	lp->l_width = 1.0;
#ifdef PM3D
	lp->use_palette = 0;
#endif
	lp->pointflag = allow_point;
	lp->p_type = def_point;
	lp->p_size = pointsize; /* as in "set pointsize" */
	while (!END_OF_COMMAND) {
	    if (almost_equals(c_token, "linet$ype") || equals(c_token, "lt")) {
		if (set_lt++)
		    break;
		c_token++;
#ifdef PM3D
		/* both syntaxes allowed: 'with lt pal' as well as 'with pal' */
		if (almost_equals(c_token, "pal$ette")) {
		    if (set_pal++)
			break;
		    c_token++;
		    lp->use_palette = 1;
		} else
#endif
		    lp->l_type = (int) real(const_express(&t)) - 1;
		continue;
	    } /* linetype, lt */

#ifdef PM3D
	    /* both syntaxes allowed: 'with lt pal' as well as 'with pal' */
	    if (almost_equals(c_token, "pal$ette")) {
		if (set_pal++)
		    break;
		c_token++;
		lp->use_palette = 1;
		continue;
	    }
#endif

	    if (almost_equals(c_token, "linew$idth") || equals(c_token, "lw")) {
		if (set_lw++)
		    break;
		c_token++;
		lp->l_width = real(const_express(&t));
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
		    lp->p_type = (int) real(const_express(&t)) - 1;
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
		    lp->p_size = real(const_express(&t));
		} else {
		    int_warn(c_token, "No pointsize specifier allowed, here");
		    c_token += 2;
		}
		continue;
	    }

	    /* unknown option catched -> quit the while(1) loop */
	    break;
	}

	if (set_lt>1 || set_pal>1 || set_lw>1 || set_pt>1 || set_ps>1)
	    int_error(c_token, "duplicated arguments in style specification");

#if defined(__FILE__) && defined(__LINE__) && defined(DEBUG_LP)
	fprintf(stderr,
		"lp_properties at %s:%d : lt: %d, lw: %.3f, pt: %d, ps: %.3f\n",     
		__FILE__, __LINE__, lp->l_type, lp->l_width, lp->p_type, lp->p_size);
#endif
    }
}




