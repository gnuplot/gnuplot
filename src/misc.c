#ifndef lint
static char *RCSid() { return RCSid("$Id: misc.c,v 1.23.2.1 2000/05/03 21:26:11 joze Exp $"); }
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
#include "setshow.h"
#include "util.h"

/* name of command file; NULL if terminal */
char *infile_name = NULL;

static TBOOLEAN lf_pop __PROTO((void));
static void lf_push __PROTO((FILE * fp));
static int find_maxl_cntr __PROTO((struct gnuplot_contours * contours, int *count));

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
 * cp_alloc() allocates a curve_points structure that can hold 'num'
 * points.
 */
struct curve_points *
cp_alloc(num)
int num;
{
    struct curve_points *cp;

    cp = (struct curve_points *) gp_alloc(sizeof(struct curve_points), "curve");
    cp->p_max = (num >= 0 ? num : 0);

    if (num > 0) {
	cp->points = (struct coordinate GPHUGE *)
	    gp_alloc(num * sizeof(struct coordinate), "curve points");
    } else
	cp->points = (struct coordinate GPHUGE *) NULL;
    cp->next = NULL;
    cp->title = NULL;
    return (cp);
}


/*
 * cp_extend() reallocates a curve_points structure to hold "num"
 * points. This will either expand or shrink the storage.
 */
void
cp_extend(cp, num)
struct curve_points *cp;
int num;
{

#if defined(DOS16) || defined(WIN16)
    /* Make sure we do not allocate more than 64k points in msdos since 
     * indexing is done with 16-bit int
     * Leave some bytes for malloc maintainance.
     */
    if (num > 32700)
	int_error(NO_CARET, "Array index must be less than 32k in msdos");
#endif /* MSDOS */

    if (num == cp->p_max)
	return;

    if (num > 0) {
	if (cp->points == NULL) {
	    cp->points = (struct coordinate GPHUGE *)
		gp_alloc(num * sizeof(struct coordinate), "curve points");
	} else {
	    cp->points = (struct coordinate GPHUGE *)
		gp_realloc(cp->points, num * sizeof(struct coordinate), "expanding curve points");
	}
	cp->p_max = num;
    } else {
	if (cp->points != (struct coordinate GPHUGE *) NULL)
	    free(cp->points);
	cp->points = (struct coordinate GPHUGE *) NULL;
	cp->p_max = 0;
    }
}

/*
 * cp_free() releases any memory which was previously malloc()'d to hold
 *   curve points (and recursively down the linked list).
 */
void
cp_free(cp)
struct curve_points *cp;
{
    if (cp) {
	cp_free(cp->next);
	if (cp->title)
	    free((char *) cp->title);
	if (cp->points)
	    free((char *) cp->points);
	free((char *) cp);
    }
}

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

/*
 * sp_alloc() allocates a surface_points structure that can hold 'num_iso_1'
 * iso-curves with 'num_samp_2' samples and 'num_iso_2' iso-curves with
 * 'num_samp_1' samples.
 * If, however num_iso_2 or num_samp_1 is zero no iso curves are allocated.
 */
struct surface_points *
sp_alloc(num_samp_1, num_iso_1, num_samp_2, num_iso_2)
int num_samp_1, num_iso_1, num_samp_2, num_iso_2;
{
    struct surface_points *sp;

    sp = (struct surface_points *) gp_alloc(sizeof(struct surface_points),
					    "surface");
    sp->next_sp = NULL;
    sp->title = NULL;
    sp->contours = NULL;
    sp->iso_crvs = NULL;
    sp->num_iso_read = 0;

    if (num_iso_2 > 0 && num_samp_1 > 0) {
	int i;
	struct iso_curve *icrv;

	for (i = 0; i < num_iso_1; i++) {
	    icrv = iso_alloc(num_samp_2);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
	for (i = 0; i < num_iso_2; i++) {
	    icrv = iso_alloc(num_samp_1);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
    } else
	sp->iso_crvs = (struct iso_curve *) NULL;

    return (sp);
}

/*
 * sp_replace() updates a surface_points structure so it can hold 'num_iso_1'
 * iso-curves with 'num_samp_2' samples and 'num_iso_2' iso-curves with
 * 'num_samp_1' samples.
 * If, however num_iso_2 or num_samp_1 is zero no iso curves are allocated.
 */
void
sp_replace(sp, num_samp_1, num_iso_1, num_samp_2, num_iso_2)
struct surface_points *sp;
int num_samp_1, num_iso_1, num_samp_2, num_iso_2;
{
    int i;
    struct iso_curve *icrv, *icrvs = sp->iso_crvs;

    while (icrvs) {
	icrv = icrvs;
	icrvs = icrvs->next;
	iso_free(icrv);
    }
    sp->iso_crvs = NULL;

    if (num_iso_2 > 0 && num_samp_1 > 0) {
	for (i = 0; i < num_iso_1; i++) {
	    icrv = iso_alloc(num_samp_2);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
	for (i = 0; i < num_iso_2; i++) {
	    icrv = iso_alloc(num_samp_1);
	    icrv->next = sp->iso_crvs;
	    sp->iso_crvs = icrv;
	}
    } else
	sp->iso_crvs = (struct iso_curve *) NULL;
}

/*
 * sp_free() releases any memory which was previously malloc()'d to hold
 *   surface points.
 */
void
sp_free(sp)
struct surface_points *sp;
{
    if (sp) {
	sp_free(sp->next_sp);
	if (sp->title)
	    free((char *) sp->title);
	if (sp->contours) {
	    struct gnuplot_contours *cntr, *cntrs = sp->contours;

	    while (cntrs) {
		cntr = cntrs;
		cntrs = cntrs->next;
		free(cntr->coords);
		free(cntr);
	    }
	}
	if (sp->iso_crvs) {
	    struct iso_curve *icrv, *icrvs = sp->iso_crvs;

	    while (icrvs) {
		icrv = icrvs;
		icrvs = icrvs->next;
		iso_free(icrv);
	    }
	}
	free((char *) sp);
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

int
find_maxl_keys(plots, count, kcnt)
struct curve_points *plots;
int count, *kcnt;
{
    int mlen, len, curve, cnt;
    register struct curve_points *this_plot;

    mlen = cnt = 0;
    this_plot = plots;
    for (curve = 0; curve < count; this_plot = this_plot->next, curve++)
	if (this_plot->title
	    && ((len = /*assign */ strlen(this_plot->title)) != 0)	/* HBB 980308: quiet BCC warning */
	    ) {
	    cnt++;
	    if (len > mlen)
		mlen = strlen(this_plot->title);
	}
    if (kcnt != NULL)
	*kcnt = cnt;
    return (mlen);
}


/* calculate the number and max-width of the keys for an splot.
 * Note that a blank line is issued after each set of contours
 */
int
find_maxl_keys3d(plots, count, kcnt)
struct surface_points *plots;
int count, *kcnt;
{
    int mlen, len, surf, cnt;
    struct surface_points *this_plot;

    mlen = cnt = 0;
    this_plot = plots;
    for (surf = 0; surf < count; this_plot = this_plot->next_sp, surf++) {

	/* we draw a main entry if there is one, and we are
	 * drawing either surface, or unlabelled contours
	 */
	if (this_plot->title && *this_plot->title &&
	    (draw_surface || (draw_contour && !label_contours))) {
	    ++cnt;
	    len = strlen(this_plot->title);
	    if (len > mlen)
		mlen = len;
	}
	if (draw_contour && label_contours && this_plot->contours != NULL) {
	    len = find_maxl_cntr(this_plot->contours, &cnt);
	    if (len > mlen)
		mlen = len;
	}
    }

    if (kcnt != NULL)
	*kcnt = cnt;
    return (mlen);
}

static int
find_maxl_cntr(contours, count)
struct gnuplot_contours *contours;
int *count;
{
    register int cnt;
    register int mlen, len;
    register struct gnuplot_contours *cntrs = contours;

    mlen = cnt = 0;
    while (cntrs) {
	if (label_contours && cntrs->isNewLevel) {
	    len = strlen(cntrs->label);
	    if (len)
		cnt++;
	    if (len > mlen)
		mlen = len;
	}
	cntrs = cntrs->next;
    }
    *count += cnt;
    return (mlen);
}


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
