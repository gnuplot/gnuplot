#ifndef lint
static char *RCSid = "$Id: hidden3d.c,v 1.33 1998/06/18 14:55:10 ddenholm Exp $";
#endif

/* GNUPLOT - hidden3d.c */

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
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 *
 * 'someone'  contributed a complete rewrite of the hidden line
 * stuff, for about beta 173 or so, called 'newhide.tgz'
 *
 * 1995-1997 Hans-Bernhard Br"oker (Broeker@physik.rwth-aachen.de)
 *   Speedup, cleanup and several modification to integrate
 *   'newhide' with 3.6 betas ~188 (start of work) up to 273.
 *   As of beta 290, this is 'officially' in gnuplot.
 *
 */

#include "plot.h"
#include "setshow.h"

/* TODO (HBB's notes, just in case you're interested):
 * + fix all the problems I annotated by a 'FIXME' comment
 * + Find out which value EPSILON should have, and why
 * + Ask gnuplot-beta for a suitable way of concentrating
 *   all those 'VERYSMALL', 'EPSILON', and other numbers
 *   into one, consistent scheme, possibly based on
 *   <float.h>. E.g., I'd say that for most applications,
 *   the best 'epsilon' is either DBL_EPS or its square root.
 * + redo all the profiling, esp. to find out if TEST_GRIDCHECK=1
 *   actually gains some speed, and if it is worth the memory
 *   spent to store the bit masks
 *   -> seems not improve speed at all, at least for my standard
 *   test case (mandd.gpl) it even slows it down by ~ 10%
 * + Evaluate the speed/memory comparison for storing vertex
 *   indices instead of (double) floating point constants
 *   to store {x|y}{min|max}
 * + remove those of my comments that are now pointless
 * + indent all that code
 * + try to really understand that 'hl_buffer' stuff...
 * + get rid of hardcoded things like sizeof(short)==4,
 *   those 0x7fff terms and all that.
 * + restructure the hl_buffer storing method to make it more efficient
 * + Try to cut the speed decrease of this code rel. to the old hidden
 *   hidden line removal. (For 'mandd.gpl', it costs an overall
 *   factor of 9 compared with my older versions of gnuplot!)
 */

/*************************/
/* Configuration section */
/*************************/

/* HBB: if this module is compiled with TEST_GRIDCHECK=1 defined,
 * it will store the information about {x|y}{min|max} in an
 * other (additional!) form: a bit mask, with each bit representing
 * one horizontal or vertical strip of the screen. The bits
 * for  strips a polygon spans are set to one. This allows to
 * test for xy_overlap simply by comparing bit patterns.
 */
#ifndef TEST_GRIDCHECK
#define TEST_GRIDCHECK 0
#endif

/* HBB 961124; If you don't want the color-distinction between the
 * 'top' and 'bottom' sides of the surface, like I do, then just compile
 * with -DBACKSIDE_LINETYPE_OFFSET=0. */
#ifndef BACKSIDE_LINETYPE_OFFSET
#define BACKSIDE_LINETYPE_OFFSET 1
#endif

/* HBB 961127: defining FOUR_TRIANGLES=1 will separate each quadrangle
 * of data points into *four* triangles, by introducing an additional
 * point at the mean of the four corners. Status: experimental 
 */
#ifndef FOUR_TRIANGLES
#define FOUR_TRIANGLES 0
#endif

/* HBB 970322: I removed the SENTINEL flag and its implementation
 * completely.  Its speed improvement wasn't that great, and it never
 * fully worked anyway, so that shouldn't make much of a difference at
 * all. */

/* HBB 961212: this #define lets you choose if the diagonals that
 * divide each original quadrangle in two triangles will be drawn
 * visible or not: do draw them, define it to be 7L, otherwise let be
 * 3L (for the FOUR_TRIANGLES method, the values are 7L and 1L) */
/* drd : default to 3, for back-compatibility with 3.5 */
#ifndef TRIANGLE_LINESDRAWN_PATTERN
#define TRIANGLE_LINESDRAWN_PATTERN 3L
#endif

/* HBB 970131: with HANDLE_UNDEFINED_POINTS=1, let's try to handle
 * UNDEFINED data points in the input we're given by the rest of
 * gnuplot. We do that by marking these points by giving them z=-2
 * (which normally never happens), and then refusing to build any
 * polygon if one of its vertices has this mark. Thus, there's now a
 * hole in the generated surface. */
/* HBB 970322: some of this code is now hardwired (in
 * PREPARE_POLYGON), so HANDLE_UNDEFINED_POINTS=0 might have stopped
 * to be a useful setting. I doubt anyone will miss it, anyway. */
/* drd : 2 means undefined only. 1 means outrange treated as undefined */
#ifndef HANDLE_UNDEFINED_POINTS
#define HANDLE_UNDEFINED_POINTS 1
#endif
/* Symbolic value for 'do not handle Undefined Points specially' */
#define UNHANDLED (UNDEFINED+1)

/* HBB 970322: If both subtriangles of a quad were cancelled, try if
 * using the other diagonal is better. This only makes a difference if
 * exactly one vertex of the quad is unusable, and that one is on the
 * first tried diagonal. In such a case, the border of the hole in the
 * surface will be less rough than with the previous method. To
 * disable this, #define SHOW_ALTERNATIVE_DIAGONAL as 0 */
#ifndef SHOW_ALTERNATIVE_DIAGONAL
#define SHOW_ALTERNATIVE_DIAGONAL 1
#endif

/* HBB 9790522: If the two triangles in a quad are both drawn, and
 * they show different sides to the user (the quad is 'bent over'),
 * then it often looks more sensible to draw the diagonal visibly to
 * avoid other parts of the scene being obscured by what the user
 * can't see. To disable, #define HANDLE_BENTOVER_QUADRANGLES 0 */
#ifndef HANDLE_BENTOVER_QUADRANGLES
#define HANDLE_BENTOVER_QUADRANGLES 1
#endif

/* HBB 970618: The code of split_polygon_by_plane used to make a
 * separate decision about what should happen to points that are 'in'
 * the splitting plane (within EPSILON accuracy, i.e.), based on the
 * orientation of the splitting plane. I had disabled that code for I
 * assumed it might be the cause of some of the buggyness. I'm not yet
 * fully convinced on wether that assumption holds or not, so it's now
 * choosable.  OLD_SPLIT_PLANE==1 will enable it. Some older comments
 * from the source:*/
/* HBB 970325: re-inserted this from older versions. Finally, someone
 * came up with a test case where it is actually needed :-( */
/* HBB 970427: seems to have been an incorrect analysis of that error.
 * the problematic plot works without this as well. */
#ifndef OLD_SPLIT_INPLANE
#define OLD_SPLIT_INPLANE 1
#endif

/* HBB 970618: this way, new edges introduced by splitting a polygon
 * by the plane of another one will be made visible. Not too useful on
 * production output, but can help in debugging. */
#ifndef SHOW_SPLITTING_EDGES
#define SHOW_SPLITTING_EDGES 0
#endif

/********************************/
/* END of configuration section */
/********************************/



/* Variables to hold configurable values. This is meant to prepare for
 * making these settable by the user via 'set hidden [option]...' */

int hiddenBacksideLinetypeOffset = BACKSIDE_LINETYPE_OFFSET;
long hiddenTriangleLinesdrawnPattern = TRIANGLE_LINESDRAWN_PATTERN;
int hiddenHandleUndefinedPoints = HANDLE_UNDEFINED_POINTS;
int hiddenShowAlternativeDiagonal = SHOW_ALTERNATIVE_DIAGONAL;
int hiddenHandleBentoverQuadrangles = HANDLE_BENTOVER_QUADRANGLES;

/* The functions to map from user 3D space into normalized -1..1 */
#define map_x3d(x) ((x-min_array[FIRST_X_AXIS])*xscale3d-1.0)
#define map_y3d(y) ((y-min_array[FIRST_Y_AXIS])*yscale3d-1.0)
#define map_z3d(z) ((z-base_z)*zscale3d-1.0)

extern int suppressMove;
extern int xright, xleft, ybot, ytop;
extern int xmiddle, ymiddle, xscaler, yscaler;
extern double min_array[], max_array[];
extern int auto_array[], log_array[];
extern double base_array[], log_base_array[];
extern double xscale3d, yscale3d, zscale3d;
extern double base_z;

extern int hidden_no_update, hidden_active;
extern int hidden_line_type_above, hidden_line_type_below;

extern double trans_mat[4][4];


#ifdef HAVE_CPP_STRINGIFY
/* ANSI preprocessor concatenation */
# define CONCAT(x,y) x##y
# define CONCAT3(x,y,z) x##y##z
#else
/* K&R preprocessor concatenation */
# define CONCAT(x,y) x/**/y
# define CONCAT3(x,y,z) x/**/y/**/z
#endif

/* Bitmap of the screen.  The array for each x value is malloc-ed as needed */
/* HBB 961111: started parametrisation of type t_pnt, to prepare change from
 * short to normal ints in **pnt. The other changes aren't always annotated,
 * so watch out! */
typedef unsigned short int t_pnt;
#define PNT_BITS (CHAR_BIT*sizeof(t_pnt))
#define PNT_MAX USHRT_MAX /* caution! ANSI-dependant ! ? */
typedef t_pnt *tp_pnt;
static tp_pnt *pnt;

/* Testroutine for the bitmap */
/* HBB 961110: fixed slight problem indicated by lclint: 
 * calling IS_UNSET with pnt==0 (possible?) */
/* HBB 961111: changed for new t_pnt, let compiler take care of optimisation
 * `%' -> `&' */
/* HBB 961124: switched semantics: as this was *always* called as !IS_SET(),
 * it's better to check for *unset* bits right away: */
#define IS_UNSET(X,Y) ((!pnt || pnt[X]==0) ? 1 : !(((pnt[X])[(Y)/PNT_BITS] >> ((Y)%PNT_BITS)) & 0x01))

/* Amount of space we need for one vertical row of bitmap, 
   and byte offset of first used element */
static unsigned long y_malloc;

/* These numbers are chosen as dividers into the bitmap. */
static int xfact, yfact;
#define XREDUCE(X) ((X)/xfact)
#define YREDUCE(Y) ((Y)/yfact)

/* These variables are only used for drawing the individual polygons
   that make up the 3d figure.  After each polygon is drawn, the
   information is copied to the bitmap: xmin_hl, ymin_hl are used to
   keep track of the range of x values.  The arrays ymin_hl[],
   ymax_hl[] are used to keep track of the minimum and maximum y
   values used for each X value. */

/* HBB 961124: new macro, to avoid a wordsize-depence */
#define HL_EXTENT_X_MAX UINT_MAX /* caution! ANSI-only !? */
static unsigned int xmin_hl, xmax_hl;

/* HBB: parametrized type of hl_buffer elements, to allow easier
   changing later on: */
#define HL_EXTENT_Y_MAX SHRT_MAX /* caution! ANSI-only !? */
typedef short int t_hl_extent_y;
static t_hl_extent_y *ymin_hl, *ymax_hl;


/* hl_buffer is a buffer which is needed to draw polygons with very small 
   angles correct:

   One polygon may be split during the sorting algorithmus into
   several polygons. Before drawing a part of a polygon, I save in
   this buffer all lines of the polygon, which will not yet be drawn.
   If then the actual part draws a virtual line (a invisible line,
   which splits the polygon into 2 parts), it draws the line visible
   in those points, which are set in the buffer.

   The layout of the buffer:
   hl_buffer[i] stands for the i'th column of the bitmap.
   Each column is splitted into pairs of points (a,b).
   Each pair describes a cross of one line with the column. */

struct Cross {
    int a, b;
	struct Cross GPHUGE *next;
  };

static struct Cross GPHUGE * GPHUGE *hl_buffer;

/* HBB 980303: added a global array of Cross structures, to save lots
 * of gp_alloc() calls (3 millions in a run through all.dem!)  */

#define CROSS_STORE_INCREASE 500 /* number of Cross'es to alloc at a time */
static struct Cross *Cross_store = 0;
static int last_Cross_store=0, free_Cross_store = 0;

struct Vertex {
    coordval x, y, z;
    int style;
  };

/* HBB 971114: two new macros, to properly hide the machinery that
 *implements flagging of vertices as 'undefined' (or 'out of range',
 *handled equally). Thanks to Lars Hecking for pointing this out */
#define FLAG_VERTEX_AS_UNDEFINED(v) \
  do { (v).z = -2.0; } while (0)
#define VERTEX_IS_UNDEFINED(v) ((v).z == -2.0)

/* These values serve as flags to detect loops of polygons obscuring
 * each other in in_front() */
typedef enum {
	is_untested = 0, is_tested, is_in_loop, is_moved_or_split
} t_poly_tested;

/* Strings for printing out values of type t_poly_tested: */
static const char *string_poly_tested[] = {
	"is_untested",
	"is_tested",
	"is_in_loop",
	"is_moved_or_split"
};

struct Polygon {
    int n;                      /* amount of vertices */
    long *vertex;								/* The vertices (indices on vlist) */
    long line;									/* i'th bit shows, if the i'th line should be drawn */
    coordval xmin, xmax, ymin, ymax, zmin, zmax;
																/* min/max in all three directions */
    struct lp_style_type *lp;		/* pointer to l/p properties */
    int style;									/* The style of the lines */
    long next;									/* index of next polygon in z-sorted list */
    long next_frag;							/* next fragment of same polygon... */
    int id;											/* Which polygons belong together? */
    t_poly_tested tested;				/* To determine a loop during the sorting algorithm */
    /* HBB 980317: on the 16 bit PC platforms, the struct size must
     * be a power of two, so it exactly fits into a 64KB segment. First
     * we'll add the TEST_GRIDCHECK fields, regardless wether that
     * feature was activated or not. */
#if TEST_GRIDCHECK || defined(DOS16) || defined(WIN16)
    unsigned int xextent, yextent;
#endif
    /* HBB 980317: the remaining bit of padding. */
#if defined(DOS16) || defined(WIN16)
    char dummies[8];
#endif
  };

typedef enum {			/* result type for polygon_plane_intersection() */
  infront, inside, behind, intersects
} t_poly_plane_intersect;

static struct Vertex GPHUGE *vlist;		/* The vertices */
static struct Polygon GPHUGE *plist;		/* The polygons */
static long nvert, npoly;				/* amount of vertices and polygons */
static long pfree, vert_free;		/* index on the first free entries */

static long pfirst;							/* Index on the first polygon */

/* macros for (somewhat) safer handling of the dynamic array of
 * polygons, 'plist[]' */
#define EXTEND_PLIST() \
    plist = (struct Polygon GPHUGE *) gp_realloc(plist, \
      (unsigned long)sizeof(struct Polygon)*(npoly+=10L), "hidden plist")

#define CHECK_PLIST() if (pfree >= npoly) EXTEND_PLIST()
#define CHECK_PLIST_2() if (pfree+1 >= npoly) EXTEND_PLIST()
#define CHECK_PLIST_EXTRA(extra) if (pfree >= npoly) { EXTEND_PLIST() ; extra; }


/* precision of calculations in normalized space: */
#define EPSILON 1e-5 /* HBB: was 1.0E-5 */
#define SIGNOF(X)  ( ((X)<-EPSILON) ? -1: ((X)>EPSILON) )

/* Some inexact relations: == , > , >= */
#define EQ(X,Y)  (fabs( (X) - (Y) ) < EPSILON)      /* X == Y */
#define GR(X,Y)  ((X) > (Y) + EPSILON)      /* X >  Y */
#define GE(X,Y)  ((X) >=(Y) - EPSILON)      /* X >= Y */

/* Test, if two vertices are near each other */
#define V_EQUAL(a,b) ( GE(0.0, fabs((a)->x - (b)->x) + \
   fabs((a)->y - (b)->y) + fabs((a)->z - (b)->z)) )

#define SETBIT(a,i,set) a= (set) ?  (a) | (1L<<(i)) : (a) & ~(1L<<(i))
#define GETBIT(a,i) (((a)>>(i))&1L)

#define UINT_BITS (CHAR_BIT*sizeof(unsigned int))
#define USHRT_BITS (CHAR_BIT*sizeof(unsigned short))


static void print_polygon __PROTO((struct Polygon GPHUGE * p, const char *pname));

/* HBB: new routine, to help debug 'Logic errors', mainly */
static void
print_polygon(p, pname)
		 struct Polygon GPHUGE * p;
		 const char *pname;
{
	struct Vertex GPHUGE *v;
  int i;

  fprintf(stderr, "#%s:(ind %d) n:%d, id:%d, next:%ld, tested:%s\n",
					pname, (int)(p-plist), p->n, p->id, p->next,
					string_poly_tested[(int)(p->tested)]);
  fprintf(stderr,"#zmin:%g, zmax:%g\n", p->zmin, p->zmax);
  for (i=0; i<p->n; i++, v++) {
    v = vlist + p->vertex[i];
    fprintf(stderr,"%g %g %g \t%ld\n", v->x, v->y, v->z, p->vertex[i]);
  }
	/*repeat first vertex, so the line gets closed: */
	v = vlist + p->vertex[0];
	fprintf(stderr,"%g %g %g \t%ld\n", v->x, v->y, v->z, p->vertex[0]);
	/*two blank lines, as a multimesh separator: */
	fprintf(stderr,"\n\n");
}

/* Gets Minimum 'C' value of polygon, C is x, y, or z: */
#define GET_MIN(p, C, min) do { \
  int i; \
  long *v=p->vertex; \
                       \
  min = vlist[*v++].C; \
  for (i=1; i< p->n; i++, v++) \
    if (vlist[*v].C < min) \
      min = vlist[*v].C; \
} while (0)

/* Gets Maximum 'C' value of polygon, C is x, y, or z: */
#define GET_MAX(p, C, max) do { \
  int i; \
  long *v=p->vertex; \
                      \
  max = vlist[*v++].C; \
  for (i=1; i< p->n; i++, v++) \
    if (vlist[*v].C > max) \
      max = vlist[*v].C; \
} while (0)

/* Prototypes for internal functions of this module. */
static void map3d_xyz __PROTO((double x, double y, double z, struct Vertex GPHUGE *v));
static int reduce_polygon __PROTO((int *n, long **v, long *line, int nmin));
static void build_polygon __PROTO((struct Polygon GPHUGE *p, int n,
  long *v, long line, int style, struct lp_style_type *lp,
  long next, long next_frag, int id, t_poly_tested tested));
static GP_INLINE int maybe_build_polygon __PROTO((struct Polygon GPHUGE *p, int n,
  long *v, long line, int style, struct lp_style_type *lp,
  long next, long next_frag, int id, t_poly_tested tested));
static void init_polygons __PROTO((struct surface_points *plots, int pcount));
static int  compare_by_zmax __PROTO((const void *p1, const void *p2));
static void sort_by_zmax __PROTO((void));
static int obscured __PROTO((struct Polygon GPHUGE *p));
static GP_INLINE int xy_overlap __PROTO((struct Polygon GPHUGE *a, struct Polygon GPHUGE *b));
static void get_plane __PROTO((struct Polygon GPHUGE *p, double *a, double *b,
                           double *c, double *d));
static t_poly_plane_intersect polygon_plane_intersection __PROTO((struct Polygon GPHUGE *p, double a,
							      double b, double c, double  d));
static int intersect __PROTO((struct Vertex GPHUGE *v1, struct Vertex GPHUGE *v2,
													struct Vertex GPHUGE *v3, struct Vertex GPHUGE *v4));
static int v_intersect __PROTO((struct Vertex GPHUGE *v1, struct Vertex GPHUGE *v2,
														struct Vertex GPHUGE *v3, struct Vertex GPHUGE *v4));
static int intersect_polygon __PROTO((struct Vertex GPHUGE *v1, struct Vertex GPHUGE *v2,
																	struct Polygon GPHUGE *p));
static int full_xy_overlap __PROTO((struct Polygon GPHUGE *a, struct Polygon GPHUGE *b));
static long build_new_vertex __PROTO((long V1, long V2, double w));
static long split_polygon_by_plane __PROTO((long P, double a, double b, double c,
                                        double d, TBOOLEAN f));
static int in_front __PROTO((long Last, long Test));

/* HBB 980303: new, for the new back-buffer for *Cross structures: */
static GP_INLINE struct Cross *get_Cross_from_store __PROTO((void));
static GP_INLINE void init_Cross_store __PROTO((void));

static GP_INLINE TBOOLEAN hl_buffer_set __PROTO((int xv, int yv));
static GP_INLINE void update_hl_buffer_column __PROTO((int xv, int ya, int yv));
static void draw_clip_line_buffer __PROTO((int x1, int y1, int x2, int y2));
static void draw_clip_line_update __PROTO((int x1, int y1, int x2, int y2,
                                       int virtual));
static GP_INLINE void clip_vector_h __PROTO((int x, int y));
static GP_INLINE void clip_vector_virtual __PROTO((int x, int y));
static GP_INLINE void clip_vector_buffer __PROTO((int x, int y));
static void draw __PROTO((struct Polygon GPHUGE *p));


/* Set the options for hidden3d. To be called from set.c, when the
 * user has begun a command with 'set hidden3d', to parse the rest of
 * that command */
void
set_hidden3doptions()
{
  struct value t;

  c_token++;
	
	if (END_OF_COMMAND) {
		return;
	}
		
	if (almost_equals(c_token,"def$aults")) {
		/* reset all parameters to defaults */
		hiddenBacksideLinetypeOffset = BACKSIDE_LINETYPE_OFFSET;
		hiddenTriangleLinesdrawnPattern = TRIANGLE_LINESDRAWN_PATTERN;
		hiddenHandleUndefinedPoints = HANDLE_UNDEFINED_POINTS;
		hiddenShowAlternativeDiagonal = SHOW_ALTERNATIVE_DIAGONAL;
		hiddenHandleBentoverQuadrangles = HANDLE_BENTOVER_QUADRANGLES;
		c_token++;
		if (!END_OF_COMMAND)
			int_error("No further options allowed after 'defaults'", c_token);
		return;
	}

  if (almost_equals(c_token, "off$set")) {
    c_token++;
    hiddenBacksideLinetypeOffset = (int) real(const_express(&t));
  } else if (almost_equals(c_token, "nooff$set")) {
		c_token++;
    hiddenBacksideLinetypeOffset = 0;
	}

  if (END_OF_COMMAND) {
		return;
	}
  if (almost_equals(c_token, "tri$anglepattern")) {
    c_token++;
    hiddenTriangleLinesdrawnPattern = (int) real(const_express(&t));
  }

  if (END_OF_COMMAND) {
		return;
	}
  if (almost_equals(c_token, "undef$ined")) {
		int tmp;

    c_token++;
    tmp = (int) real(const_express(&t));
		if (tmp<=0 || tmp > UNHANDLED)
			tmp = UNHANDLED;
		hiddenHandleUndefinedPoints = tmp;
  } else if (almost_equals(c_token, "nound$efined")) {
    c_token++;
    hiddenHandleUndefinedPoints = UNHANDLED;
	}

  if (END_OF_COMMAND) {
		return;
	}
  if (almost_equals(c_token, "alt$diagonal")) {
    c_token++;
    hiddenShowAlternativeDiagonal = 1;
	} else if (almost_equals(c_token, "noalt$diagonal")) {
		c_token++;
    hiddenShowAlternativeDiagonal = 0;
  }
  
  if (END_OF_COMMAND) {
		return;
	}
  if (almost_equals(c_token, "bent$over")) {
    c_token++;
    hiddenHandleBentoverQuadrangles = 1;
  } else if (almost_equals(c_token, "nobent$over")) {
    c_token++;
    hiddenHandleBentoverQuadrangles = 0;
  }
	
	if (!END_OF_COMMAND) {
		int_error("No such option to hidden3d (or wrong order)", c_token);
	}
}

/* HBB 970619: new routine for displaying the option settings */
void
show_hidden3doptions()
{
	fprintf(stderr, "\t  Back side of surfaces has linestyle offset of %d\n",
					hiddenBacksideLinetypeOffset);
	fprintf(stderr, "\t  Bit-Mask of Lines to draw in each triangle is %ld\n",
					hiddenTriangleLinesdrawnPattern);
	fprintf(stderr, "\t  %d: ", hiddenHandleUndefinedPoints);
	switch (hiddenHandleUndefinedPoints) {
	case OUTRANGE:
		fprintf(stderr, "Outranged and undefined datapoints are omitted from the surface.\n");
		break;
	case UNDEFINED:
		fputs("Only undefined datapoints are omitted from the surface.\n", stderr);
		break;
	case UNHANDLED:
		fputs("Will not check for undefined datapoints (may cause crashes).\n", stderr);
		break;
	default:
		fputs("This value is illegal!!!\n", stderr);
		break;
	}
	fprintf(stderr, "\t  Will %suse other diagonal if it gives a less jaggy outline\n",
					hiddenShowAlternativeDiagonal ? "" : "not ");
	fprintf(stderr, "\t  Will %sdraw diagonal visibly if quadrangle is 'bent over'\n",
					hiddenHandleBentoverQuadrangles ? "" : "not ");
}

/* HBB 971117: new function.*/
/* Implements proper 'save'ing of the new hidden3d options... */
void
save_hidden3doptions(fp)
		 FILE *fp;
{
	if (!hidden3d) {
		fputs("set nohidden3d\n", fp);
		return;
	}
	fprintf(fp, "set hidden3d offset %d trianglepattern %ld undefined %d %saltdiagonal %sbentover\n",
					hiddenBacksideLinetypeOffset,
					hiddenTriangleLinesdrawnPattern,
					hiddenHandleUndefinedPoints,
					hiddenShowAlternativeDiagonal ? "" : "no",
					hiddenHandleBentoverQuadrangles ? "" : "no");
}

/* Initialize the necessary steps for hidden line removal and
   initialize global variables. */
void 
init_hidden_line_removal ()
{
  int i;

	/* Check for some necessary conditions to be set elsewhere: */
	/* HandleUndefinedPoints mechanism depends on these: */
	assert(OUTRANGE==1);					
	assert(UNDEFINED==2);
	/* '3' makes the test easier in the critical section */
	if (hiddenHandleUndefinedPoints<OUTRANGE)
		hiddenHandleUndefinedPoints=UNHANDLED;
	
  /*  We want to keep the bitmap size less than 2048x2048, so we choose
   *  integer dividers for the x and y coordinates to keep the x and y
   *  ranges less than 2048.  In practice, the x and y sizes for the bitmap
   *  will be somewhere between 1024 and 2048, except in cases where the
   *  coordinates ranges for the device are already less than 1024.
   *  We do this mainly to control the size of the bitmap, but it also
   *  speeds up the computation.  We maintain separate dividers for
   *  x and y.
   */
  xfact = (xright - xleft) / 1024;
  yfact = (ytop - ybot) / 1024;
  if (xfact == 0)
    xfact = 1;
  if (yfact == 0)
    yfact = 1;
  if (pnt == 0)
    {
      i = XREDUCE (xright) - XREDUCE (xleft) + 1;
      pnt = (tp_pnt *) gp_alloc (
        (unsigned long) (i * sizeof (tp_pnt)), "hidden pnt");
      while (--i>=0)
        pnt[i] = (tp_pnt) 0;
    }
}

/* Reset the hidden line data to a fresh start.                              */
void 
reset_hidden_line_removal ()
{
  int i;
  if (pnt)
    {
      for (i = 0; i <= XREDUCE (xright) - XREDUCE (xleft); i++)
	{
	  if (pnt[i])
	    {
	      free (pnt[i]);
	      pnt[i] = 0;
	    };
	};
    };
}


/* Terminates the hidden line removal process.                  */
/* Free any memory allocated by init_hidden_line_removal above. */
void 
term_hidden_line_removal ()
{
  if (pnt)
    {
      int j;
      for (j = 0; j <= XREDUCE (xright) - XREDUCE (xleft); j++)
	{
	  if (pnt[j])
	    {
	      free (pnt[j]);
	      pnt[j] = 0;
	    };
	};
      free (pnt);
      pnt = 0;
    };
  if (ymin_hl)
    free (ymin_hl), ymin_hl = 0;
  if (ymax_hl)
    free (ymax_hl), ymax_hl = 0;
}


/* Maps from normalized space to terminal coordinates */
#define TERMCOORD(v,x,y) x = ((int)((v)->x * xscaler)) + xmiddle, \
			 y = ((int)((v)->y * yscaler)) + ymiddle;


static void
map3d_xyz (x, y, z, v)
     double x, y, z;		/* user coordinates */
		 struct Vertex GPHUGE *v;         /* the point in normalized space */
{
  int i, j;
  double V[4], Res[4],		/* Homogeneous coords. vectors. */
    w = trans_mat[3][3];
  /* Normalize object space to -1..1 */
  V[0] = map_x3d (x);
  V[1] = map_y3d (y);
  V[2] = map_z3d (z);
  V[3] = 1.0;
  Res[0]=0; /*HBB 961110: lclint wanted this... Why? */
  for (i = 0; i < 3; i++)
    {
      Res[i] = trans_mat[3][i];	/* Initiate it with the weight factor. */
      for (j = 0; j < 3; j++)
	Res[i] += V[j] * trans_mat[j][i];
    }
  for (i = 0; i < 3; i++)
    w += V[i] * trans_mat[i][3];
  if (w == 0)
    w = 1.0e-5;
  v->x = Res[0] / w;
  v->y = Res[1] / w;
  v->z = Res[2] / w;
}


/* remove unnecessary Vertices from a polygon: */
static int 
reduce_polygon (n, v, line, nmin)
     int *n;			/* number of vertices */
     long **v;			/* the vertices */
     long *line;		/* the information, which line should be drawn */
     int nmin;			/* if the number reduces under nmin, forget polygon */
     /* Return value 1: the reduced polygon has still more or equal 
        vertices than nmin.
        Return value 0: the polygon was trivial; memory is given free */
{
  int less, i;
  long *w = *v;
  for (less = 0, i = 0; i < *n - 1; i++) {
		if (V_EQUAL (vlist + w[i], vlist + w[i + 1])
				/* HBB 970321: try to remove an endless loop bug (part1/2) */
				&& ((w[i] == w[i+1])
						|| !GETBIT (*line, i)
						|| GETBIT (*line, i + 1)
						|| ((i > 0) ? GETBIT (*line, i - 1) : GETBIT (*line, *n - 1))))
			less++;
		else if (less) {
			w[i - less] = w[i];
			SETBIT (*line, i - less, GETBIT (*line, i));
		}
	}
  if (i - less > 0 
      && V_EQUAL (vlist + w[i], vlist + w[0]) 
			/* HBB 970321: try to remove an endless loop bug (part2/2) */
      && (w[i] == w[0]
					|| !GETBIT (*line, i)
					|| GETBIT (*line, i - 1)
					|| GETBIT (*line, 0)))
		less++;
	else if (less) {
		w[i - less] = w[i];
		SETBIT (*line, i - less, GETBIT (*line, i));
	}
	*n -= less;
  if (*n < nmin) {
		free (w);
		*v = 0;											/* HBB 980213: signal that v(==w) is free()d */
		return 0;
	}
  if (less) {
		w = (long *) gp_realloc (w, (unsigned long) sizeof (long) * (*n),
												 "hidden red_poly");
		*v = w;
		for (; less > 0; less--)
			SETBIT (*line, *n + less - 1, 0);
	}
  return 1;
}

/* Write polygon in plist */
/*
 * HBB: changed this to precalculate {x|y}{min|max}
 */
static void
build_polygon (p, n, v, line, style, lp, next, next_frag, id, tested)
     struct Polygon GPHUGE *p;               /* this should point at a free entry in plist */
     int n;			/* number of vertices */
     long *v;			/* array of indices on the vertices (in vlist) */
     long line;			/* information, which line should be drawn */
     int style;			/* color of the lines */
     struct lp_style_type *lp;	/* pointer to line/point properties */
     long next;			/* next polygon in the list */
     long next_frag;		/* next fragment of same polygon */
     int id;			/* Which polygons belong together? */
     t_poly_tested tested;		/* Is polygon already on the right place of list? */
{
  int i;
  struct Vertex vert;

  if (p >= plist+npoly)
  	fprintf(stderr, "uh oh !\n");

  CHECK_POINTER(plist, p);
  
  p->n = n;
  p->vertex = v;
  p->line = line;
  p->lp = lp;           /* line and point properties */
  p->style = style;
  CHECK_POINTER(vlist, (vlist + v[0]));
  vert=vlist[v[0]];
  p->xmin = p->xmax = vert.x;
  p->ymin = p->ymax = vert.y;
  p->zmin = p->zmax = vert.z;
  for (i = 1; i < n; i++)
    {
      CHECK_POINTER(vlist, (vlist + v[i]));
      vert=vlist[v[i]];
      if (vert.x < p->xmin)
        p->xmin=vert.x;
      else if (vert.x > p->xmax)
        p->xmax=vert.x;
      if (vert.y < p->ymin)
        p->ymin=vert.y;
      else if (vert.y > p->ymax)
        p->ymax=vert.y;
      if (vert.z < p->zmin)
        p->zmin=vert.z;
      else if (vert.z > p->zmax)
        p->zmax=vert.z;
    }

#if TEST_GRIDCHECK
/* FIXME: this should probably use a table of bit-patterns instead...*/
  p->xextent = (~ (~0U << (unsigned int)((p->xmax+1.0)/2.0*UINT_BITS+1.0)))
               & (~0U << (unsigned int)((p->xmin+1.0)/2.0*UINT_BITS));
  p->yextent = (~ (~0U << (unsigned int)((p->ymax+1.0)/2.0*UINT_BITS+1.0)))
               & (~0U << (unsigned int)((p->ymin+1.0)/2.0*UINT_BITS));
#endif

  p->next = next;
  p->next_frag = next_frag; /* HBB 961201: link fragments, to speed up draw() q-loop */
  p->id = id;
  p->tested = tested;
}


/* get the amount of curves in a plot */
#define GETNCRV(NCRV) do {\
   if(this_plot->plot_type==FUNC3D)        \
     for(icrvs=this_plot->iso_crvs,NCRV=0; \
	 icrvs;icrvs=icrvs->next,NCRV++);  \
   else if(this_plot->plot_type == DATA3D) \
        NCRV = this_plot->num_iso_read;    \
    else {                                 \
    	graph_error("Plot type is neither function nor data"); \
    	return; /* stops a gcc -Wunitialised warning */        \
    } \
} while (0)

/* Do we see the top or bottom of the polygon, or is it 'on edge'? */
#define GET_SIDE(vlst,csign) do { \
  double c = vlist[vlst[0]].x * (vlist[vlst[1]].y - vlist[vlst[2]].y) + \
    vlist[vlst[1]].x * (vlist[vlst[2]].y - vlist[vlst[0]].y) + \
    vlist[vlst[2]].x * (vlist[vlst[0]].y - vlist[vlst[1]].y); \
  csign = SIGNOF (c); \
} while (0)

/* HBB 970131: new function, to allow handling of undefined datapoints
 * by leaving out the corresponding polygons from the list.
 * Return Value: 1 if polygon was built, 0 otherwise */
static GP_INLINE int
maybe_build_polygon (p, n, v, line, style, lp,
										 next, next_frag, id, tested)
		 struct Polygon GPHUGE *p;
		 struct lp_style_type *lp;
		 int n, style, id;
		 t_poly_tested tested;
		 long *v, line, next, next_frag;
{
  int i;

  assert(v);

	if (hiddenHandleUndefinedPoints!=UNHANDLED) 
  for (i=0; i<n ; i++)
			if (VERTEX_IS_UNDEFINED(vlist[v[i]])) {
				free(v);								/* HBB 980213: free mem... */
				v = 0;
      return 0;			/* *don't* build the polygon! */
			}
  build_polygon (p, n, v, line, style, lp, next, next_frag, id, tested );
  return 1;
}

/* HBB 970322: new macro, invented because code like this occured several
 * times inside init_polygons, esp. after I added the option to choose the
 * other diagonal to divide a quad when necessary */
/* HBB 980213: Here and below in init_polygons, added code to properly
 * free any allocated vertex lists ('v' here), or avoid allocating
 * them in the first place. Thanks to Stefan Schroepfer for reporting
 * the leak.  */
#define PREPARE_POLYGON(n,v,i0,i1,i2,line,c,border,i_chk,ncrv_chk,style) do {\
  (n) = 3; \
  if (VERTEX_IS_UNDEFINED(vlist[vert_free + (i0)])\
      || VERTEX_IS_UNDEFINED(vlist[vert_free + (i1)]) \
      || VERTEX_IS_UNDEFINED(vlist[vert_free + (i2)])) { \
    /* These values avoid any further action on this polygon */\
    (v)=0; /* flag this as undefined */ \
    (c)=(border)=0; \
  } else {\
  (v) = (long *) gp_alloc ((unsigned long) sizeof (long) * (n), "hidden PREPARE_POLYGON"); \
  (v)[0] = vert_free + (i0);\
  (v)[1] = vert_free + (i1);\
  (v)[2] = vert_free + (i2);\
  (line) = hiddenTriangleLinesdrawnPattern;\
    GET_SIDE((v),(c));\
    /* Is this polygon at the border of the surface? */\
    (border) = (i == (i_chk) || ncrv == (ncrv_chk));\
  }\
  (style) = ((c) >= 0) ? above : below;\
} while (0)

static void
init_polygons (plots, pcount)
     struct surface_points *plots;
     int pcount;
     /* Initialize vlist, plist */
{
  long int i;
  struct surface_points *this_plot;
  int surface;
  long int ncrv, ncrv1;
  struct iso_curve *icrvs;
  int row_offset;
  int id;
  int n1, n2;                   /* number of vertices of a Polygon */
  long *v1, *v2;                /* the Vertices */
  long line1, line2;            /* which  lines should be drawn */
  int above=-1, below, style1, style2;     /* the line color */
  struct lp_style_type *lp;     /* pointer to line and point properties */
  int c1, c2;                   /* do we see the front or the back */
  TBOOLEAN border1, border2;		/* is it at the border of the surface */

	/* allocate memory for polylist and nodelist */
  nvert = npoly = 0;
  for (this_plot = plots, surface = 0;
       surface < pcount;
       this_plot = this_plot->next_sp, surface++) {
		GETNCRV (ncrv);
		switch (this_plot->plot_style) {
		case LINESPOINTS:
		case STEPS:             /* handle as LINES */
		case FSTEPS:
		case HISTEPS:
		case LINES:
#if FOUR_TRIANGLES
			nvert += ncrv * (2 * this_plot->iso_crvs->p_count - 1);
#else
			nvert += ncrv * (this_plot->iso_crvs->p_count);
#endif
			npoly += 2 * (ncrv - 1) * (this_plot->iso_crvs->p_count - 1);
			break;
		case DOTS:
		case XERRORBARS:        /* handle as POINTSTYLE */
		case YERRORBARS:
		case XYERRORBARS:
		case BOXXYERROR:
		case BOXERROR:
		case CANDLESTICKS:      /* HBB: these as well */
		case FINANCEBARS:
		case VECTOR:
		case POINTSTYLE:
			nvert += ncrv * (this_plot->iso_crvs->p_count);
			npoly += (ncrv - 1) * (this_plot->iso_crvs->p_count - 1);
			break;
		case BOXES:             /* handle as IMPULSES */
		case IMPULSES:
			nvert += 2 * ncrv * (this_plot->iso_crvs->p_count);
			npoly += (ncrv - 1) * (this_plot->iso_crvs->p_count - 1);
			break;
		}
	}
  vlist = (struct Vertex GPHUGE *)
    gp_alloc ((unsigned long) sizeof (struct Vertex) * nvert, "hidden vlist");
  plist = (struct Polygon GPHUGE *)
    gp_alloc ((unsigned long) sizeof (struct Polygon) * npoly, "hidden plist");

  /* initialize vlist: */
  for (vert_free = 0, this_plot = plots, surface = 0;
       surface < pcount;
       this_plot = this_plot->next_sp, surface++) {
		switch (this_plot->plot_style) {
		case LINESPOINTS:
		case BOXERROR:          /* handle as POINTSTYLE */
		case BOXXYERROR:
		case XERRORBARS:
		case YERRORBARS:
		case XYERRORBARS:
		case CANDLESTICKS:      /* HBB: these as well */
		case FINANCEBARS:
		case VECTOR:
		case POINTSTYLE:
			above = this_plot->lp_properties.p_type;
			break;
		case STEPS:             /* handle as LINES */
		case FSTEPS:
		case HISTEPS:
		case LINES:
		case DOTS:
		case BOXES:             /* handle as IMPULSES */
		case IMPULSES:
			above = -1;
			break;
		}
		GETNCRV (ncrv1);
		for (ncrv = 0, icrvs = this_plot->iso_crvs;
				 ncrv < ncrv1 && icrvs;
				 ncrv++, icrvs = icrvs->next) {
			struct coordinate GPHUGE *points = icrvs->points;

			for (i = 0; i < icrvs->p_count; i++) {
				/* As long as the point types keep OUTRANGE==1 and
				 * UNDEFINED==2, we can just compare
				 * hiddenHandleUndefinedPoints to the type of the point. To
				 * simplify this, let UNHANDLED := UNDEFINED+1 mean 'do not
				 * handle undefined or outranged points at all' (dangerous
				 * option, that is...) */
				if (points[i].type >= hiddenHandleUndefinedPoints) {
					/* mark this vertex as a bad one */
					FLAG_VERTEX_AS_UNDEFINED(vlist[vert_free++]);
					continue;
				}

				map3d_xyz (points[i].x, points[i].y, points[i].z, vlist + vert_free);
				vlist[vert_free++].style = above;
				if (this_plot->plot_style == IMPULSES 
						|| this_plot->plot_style == BOXES) {
					map3d_xyz (points[i].x, points[i].y, min_array[FIRST_Z_AXIS], vlist + vert_free);
					vlist[vert_free++].style = above;
				}
#if FOUR_TRIANGLES
				/* FIXME: handle other styles as well! */
				if (this_plot->plot_style == LINES && 
						i < icrvs->p_count - 1) 
					vert_free++;    /* keep one entry free for quad-center */
#endif
			}
		}
	}

  /* initialize plist: */
  id = 0;
  for (pfree = vert_free = 0, this_plot = plots, surface = 0;
       surface < pcount;
       this_plot = this_plot->next_sp, surface++) {
		row_offset = this_plot->iso_crvs->p_count;
		lp = &(this_plot->lp_properties);
		above = this_plot->lp_properties.l_type;
		below = this_plot->lp_properties.l_type + hiddenBacksideLinetypeOffset;
		GETNCRV (ncrv1);
		for (ncrv = 0, icrvs = this_plot->iso_crvs;
				 ncrv < ncrv1 && icrvs;
				 ncrv++, icrvs = icrvs->next)
			for (i = 0; i < icrvs->p_count; i++)
				switch (this_plot->plot_style) {
				case LINESPOINTS:
				case STEPS: /* handle as LINES */
				case FSTEPS:
				case HISTEPS:
				case LINES:
					if (i < icrvs->p_count - 1 && ncrv < ncrv1 - 1) {
#if FOUR_TRIANGLES
						long *v3, *v4;
						int n3, n4;
						long line3, line4;
						int c3, c4, style3, style4;
						TBOOLEAN border3, border4, inc_id=FALSE;
                  
						/* Build medium vertex: */
						vlist[vert_free+1].x = 
							(vlist[vert_free].x + vlist[vert_free+2*row_offset-1].x +
							 vlist[vert_free+2].x + vlist[vert_free+2*row_offset+1].x
							 ) / 4;
						vlist[vert_free+1].y =
							(vlist[vert_free].y + vlist[vert_free+2*row_offset-1].y +
							 vlist[vert_free+2].y + vlist[vert_free+2*row_offset+1].y
							 ) / 4;
						vlist[vert_free+1].z =
							(vlist[vert_free].z + vlist[vert_free+2*row_offset-1].z +
							 vlist[vert_free+2].z + vlist[vert_free+2*row_offset+1].z
							 ) / 4;
						vlist[vert_free+1].style = above;

						PREPARE_POLYGON(n1, v1, 2, 0, 1, line1, c1,
														border1, 0, -1, style1);
						/* !!FIXME!! special casing (see the older code) still needs to be
						 * implemented! */
						if ((c1 || border1)
								&& reduce_polygon (&n1, &v1, &line1, 3)) {
							CHECK_PLIST();
							pfree += maybe_build_polygon (plist + pfree, n1, v1, line1,
																						style1, lp, -1L, -1L, id++,
																						is_untested);
							inc_id = TRUE;
						}                 

						PREPARE_POLYGON(n2, v2, 0, 2*row_offset-1, 1, line2, c2,
														border2, -1, 0, style2);
						if ((c2 || border2)
								&& reduce_polygon (&n2, &v2, &line2, 3)) {
							CHECK_PLIST();
							pfree += maybe_build_polygon (plist + pfree, n2, v2, line2,
																						style2,	lp, -1L, -1L, id++,
																						is_untested);
							inc_id = TRUE;
						}                 

						PREPARE_POLYGON(n3, v3, 2*row_offset-1, 2*row_offset+1, 1,
														line3, c3, border3, icrvs->p_count-2, -1,
														style3);
						if ((c3 || border3)
								&& reduce_polygon (&n3, &v3, &line3, 3)) {
							CHECK_PLIST();
							pfree += maybe_build_polygon (plist + pfree, n3, v3, line3,
																						style3, lp, -1L, -1L, id++,
																						is_untested);
							inc_id = TRUE;
						}                 

						PREPARE_POLYGON(n4, v4, 2*row_offset+1, 0, 1, line4, c4,
														border4, -1, ncrv1-2, style4);
						if ((c4 || border4) 
								&& reduce_polygon (&n4, &v4, &line4, 3)) {
							CHECK_PLIST();
							pfree += maybe_build_polygon (plist + pfree, n4, v4, line4,
																						style4,	lp, -1L, -1L, id++,
																						is_untested);
							inc_id = TRUE;
						}

						/*if (inc_id)
							id++;*/
						vert_free++;

#else  /* FOUR_TRIANGLES */

						PREPARE_POLYGON(n1, v1, row_offset, 0, 1, line1, c1,
														border1, 0, 0, style1);
						PREPARE_POLYGON(n2, v2, 1, row_offset+1, row_offset, line2,
														c2, border2, icrvs->p_count - 2, ncrv1 - 2,
														style2);

						/* if this makes at least one of the two triangles
						 * visible, use the other diagonal here */
						if (hiddenShowAlternativeDiagonal
								&& !(v1)				/* HBB 980213: only try this in the case of */
								&& !(v2)				/* undefined vertices -> *missing* trigs */
								)
							if (VERTEX_IS_UNDEFINED(vlist[vert_free+1])) {
								PREPARE_POLYGON(n1, v1, row_offset+1, row_offset, 0,
																line1, c1, border1, 0, ncrv1 - 2,
																style1);
							} else if (VERTEX_IS_UNDEFINED(vlist[vert_free+row_offset])) {
								PREPARE_POLYGON(n1, v1, 0, 1, row_offset+1, line1,
																c1, border1, icrvs->p_count - 2, 0,
																style1);
							}

						/* HBB 970323: if visible sides of the two triangles
						 * differs, make diagonal visible! */
						/* HBB 970428: FIXME: should this be changed to only
						 * activate *one* of the triangles' diagonals? If so, how
						 * to find out the right one? Maybe the one with its
						 * normal closer to the z direction? */
						if (hiddenHandleBentoverQuadrangles
								&& (c1*c2)<0
								) { 
							SETBIT(line1, n1-1, 1);
							SETBIT(line2, n2-1, 1);
						}

						if ((c1 || border1) 
								&& reduce_polygon (&n1, &v1, &line1, 3)) {
							if ((c2 || border2) 
									&& reduce_polygon (&n2, &v2, &line2, 3)) {
								/* These two will need special handling, to
								 * ensure the links from one to the other
								 * are only set up when *both* polygons are
								 * valid */
								int r1, r2; /* temp. results */

								CHECK_PLIST_2();
								r1 = maybe_build_polygon (plist + pfree, n1, v1,
																					line1, style1, lp, -1L,
																					-1L, id, is_untested);
								r2 = maybe_build_polygon (plist + pfree+r1, n2, v2,
																					line2, style2, lp, -1L,
																					-1L, id++, is_untested);
								if (r1 && r2) {
									plist[pfree].next_frag=pfree+1;
									plist[pfree+1].next_frag=pfree;
								}
								pfree += r1 + r2;
							} else {					/* P1 was ok, but P2 wasn't */
/* HBB 980213: P2 is invisible, so free its vertex list: */
								if (v2) {
									free(v2);
									v2=0;
								}
/* HBB 970323: if other polygon wasn't drawn, draw this polygon's
 * diagonal visibly (not only if it was 'on edge'). */
								/*if (c2 || border2)*/ 
								SETBIT (line1, n1 - 1, 1);
								CHECK_PLIST();
								pfree += maybe_build_polygon (plist + pfree, n1, v1, line1,
																							style1, lp, -1L, -1L, id++,
																							is_untested);
							}
						} else {						/* P1 was not ok */
/* HBB 980213: P1 is invisible, so free its vertex list: */
							if (v1) {
								free(v1);
								v1=0;
							}
/* HBB 970323: if other polygon wasn't drawn, draw this polygon's
 * diagonal visibly (not only if it was 'on edge'). */
							/*if (c1 || border1)*/
							SETBIT (line2, n2 - 1, 1);
/* HBB 970522: yet another problem triggered by the handling of
 * undefined points: if !(c2||border2), we mustn't try to reduce
 * triangle 2. */
							if (0
									|| (1
											&& (c2 || border2)
											&& reduce_polygon (&n2, &v2, &line2, 2))
									|| (1
/* HBB 980213: reduce_... above might have zeroed v... */
											&& (v2)		
											&& (c1 || border1))) {
								CHECK_PLIST();
								pfree += maybe_build_polygon (plist + pfree, n2, v2, line2,
																							style2,	lp, -1L, -1L, id++,
																							is_untested);
							} else {
								/* HBB 980213: sigh... both P1 and P2 were invisible, but
								 * at least one of them was a not undefined.. */
								if (v1)
									free(v1);
								if (v2)
									free(v2);
							}
						}
#endif /* else: FOUR_TRIANGLES */
					}
					vert_free++;
					break;
				case DOTS:
					v1 = (long *) gp_alloc ((unsigned long) sizeof (long) * 1,
																	"hidden v1 for dots");
					v1[0] = vert_free++;
					CHECK_PLIST();
					pfree += maybe_build_polygon (plist + pfree, 1, v1, 1L, above,
																				lp, -1L, -1L, id++, is_untested);
					break;
				case BOXERROR:      /* handle as POINTSTYLE */
				case BOXXYERROR:
				case XERRORBARS:    /* handle as POINTSTYLE */
				case YERRORBARS:
				case XYERRORBARS:
				case CANDLESTICKS:      /* HBB: these as well */
				case FINANCEBARS:
				case POINTSTYLE:
				case VECTOR:
					v1 = (long *) gp_alloc ((unsigned long) sizeof (long) * 1,
																	"hidden v1 for point");
					v1[0] = vert_free++;
					CHECK_PLIST();
					pfree += maybe_build_polygon (plist + pfree, 1, v1, 0L, above,
																				lp, -1L, -1L, id++, is_untested);
					break;
				case BOXES: /* handle as IMPULSES */
				case IMPULSES:
					n1 = 2;
					v1 = (long *) gp_alloc ((unsigned long) sizeof (long) * n1,
																	"hidden v1 for impulse");
					v1[0] = vert_free++;
					v1[1] = vert_free++;
					line1 = 2L;
					(void)reduce_polygon (&n1, &v1, &line1, 1);
					CHECK_PLIST();
					pfree += maybe_build_polygon (plist + pfree, n1, v1, line1, above,
																				lp, -1L, -1L, id++, is_untested);
					break;
				}
	}
}

static int 
compare_by_zmax (p1, p2)
     const void *p1, *p2;
{
  return (SIGNOF (plist[*(const long *)p2].zmax - plist[*(const long *)p1].zmax));
}

static void 
sort_by_zmax ()
     /* and build the list (return_value = start of list) */
{
  long *sortarray, i;
	struct Polygon GPHUGE *this;
  sortarray = (long *) gp_alloc ((unsigned long) sizeof (long) * pfree, "hidden sortarray");
  for (i = 0; i < pfree; i++)
    sortarray[i] = i;
  qsort (sortarray, (size_t) pfree, sizeof (long), compare_by_zmax);
  this = plist + sortarray[0];
  for (i = 1; i < pfree; i++)
    {
      this->next = sortarray[i];
      this = plist + sortarray[i];
    }
  this->next = -1L;
  pfirst = sortarray[0];
  free (sortarray);
}


/* HBB: try if converting this code to unsigned int (from signed shorts)
 *  fixes any of the remaining bugs. In the same motion, remove
 *  hardcoded sizeof(short)=2 (09.10.1996) */

#define LASTBITS (USHRT_BITS -1) /* ????? */
static int 
obscured (p)
     /* is p obscured by already drawn polygons? (only a simple minimax-test) */
		 struct Polygon GPHUGE *p;
{
  int l_xmin, l_xmax, l_ymin, l_ymax; /* HBB 961110: avoid shadowing external names */
  t_pnt mask1, mask2;
  long indx1, indx2, k, m;
  tp_pnt cpnt;
  /* build the minimax-box */
  l_xmin = (p->xmin * xscaler) + xmiddle;
  l_xmax = (p->xmax * xscaler) + xmiddle;
  l_ymin = (p->ymin * yscaler) + ymiddle;
  l_ymax = (p->ymax * yscaler) + ymiddle;
  if (l_xmin < xleft)
    l_xmin = xleft;
  if (l_xmax > xright)
    l_xmax = xright;
  if (l_ymin < ybot)
    l_ymin = ybot;
  if (l_ymax > ytop)
    l_ymax = ytop;
  if (l_xmin > l_xmax || l_ymin > l_ymax)
    return 1;			/* not viewable */
  l_ymin = YREDUCE (l_ymin) - YREDUCE(ybot);
  l_ymax = YREDUCE (l_ymax) - YREDUCE(ybot);
  l_xmin = XREDUCE (l_xmin) - XREDUCE(xleft);
  l_xmax = XREDUCE (l_xmax) - XREDUCE(xleft);
  /* Now check bitmap */
  indx1 = l_ymin / PNT_BITS;
  indx2 = l_ymax / PNT_BITS;
  mask1 = PNT_MAX << (((unsigned)l_ymin) % PNT_BITS);
  mask2 = PNT_MAX >> ((~((unsigned)l_ymax)) % PNT_BITS);
  /* HBB: lclint wanted this: */
  assert(pnt != 0);
  for (m = l_xmin; m <= l_xmax; m++)
    {
      if (pnt[m] == 0)
	return 0;		/* not obscured */
      cpnt = pnt[m] + indx1;
      if (indx1 == indx2)
        {
          if (~(*cpnt) & mask1 & mask2)
	    return 0;
	}
      else
	{
	  if (~(*cpnt++) & mask1)
	    return 0;
          for (k=indx1+1; k<indx2; k++)
            if ((*cpnt++) != PNT_MAX)
              return 0;
          if (~(*cpnt++) & mask2)
	    return 0;
	}
    }
  return 1;
}


void draw_line_hidden(x1, y1, x2, y2)
  unsigned int x1, y1, x2, y2;
{
  register struct termentry *t = term;
  TBOOLEAN flag;
  register int xv, yv, errx, erry, err;
  register unsigned int xvr, yvr;
  int unsigned xve, yve;
  register int dy, nstep=0, dyr;
  int i;

  if (x1 > x2){
    xvr = x2;
    yvr = y2;
    xve = x1;
    yve = y1;
  } else {
    xvr = x1;
    yvr = y1;
    xve = x2;
    yve = y2;
  };
  errx = XREDUCE(xve) - XREDUCE(xvr);
  erry = YREDUCE(yve) - YREDUCE(yvr);
  dy = (erry > 0 ? 1 : -1);
  dyr = dy*yfact;
  switch (dy){
  case 1:
    nstep = errx + erry;
    errx = -errx;
    break;
  case -1:
    nstep = errx - erry;
    errx = -errx;
    erry = -erry;
    break;
  };
  err = errx + erry;
  errx <<= 1;
  erry <<= 1;
  xv = XREDUCE(xvr) - XREDUCE(xleft);
  yv = YREDUCE(yvr) - YREDUCE(ybot);
  (*t->move)(xvr,yvr);
  flag = !IS_UNSET(xv,yv);
  if(!hidden_no_update){ /* Check first point */
    assert(ymax_hl!=0);
    assert(ymin_hl!=0);
    if (xv < xmin_hl) xmin_hl = xv;
    if (xv > xmax_hl) xmax_hl = xv;
    if (yv > ymax_hl[xv]) ymax_hl[xv] = yv;
    if (yv < ymin_hl[xv]) ymin_hl[xv] = yv;
  };
  for (i=0;i<nstep;i++){
    if (err < 0){
      xv ++;
      xvr += xfact;
      err += erry;
    } else {
      yv += dy;
      yvr += dyr;
      err += errx;
    };
    if( IS_UNSET(xv,yv)){
      if (flag)
	{
	  (*t->move)(xvr,yvr);
	  flag = FALSE;
	};
    } else {
      if (!flag)
	{
	  (*t->vector)(xvr,yvr);
	  flag = TRUE;
	};
    };
    if(!hidden_no_update){
      /* HBB 961110: lclint wanted these: */
      assert(ymax_hl!=0);
      assert(ymin_hl!=0);
      if (xv < xmin_hl) xmin_hl = xv;
      if (xv > xmax_hl) xmax_hl = xv;
      if (yv > ymax_hl[xv]) ymax_hl[xv] = yv;
      if (yv < ymin_hl[xv]) ymin_hl[xv] = yv;
    };
  };
  if (!flag)
    (*t->vector)(xve, yve);
  return;
}

static GP_INLINE int
xy_overlap (a, b)
     /* Do a and b overlap in x or y (minimax test) */
		 struct Polygon GPHUGE *a, GPHUGE *b;
{
#if TEST_GRIDCHECK
  /* First, check by comparing the extent bit patterns: */
  if (! ((a->xextent & b->xextent) && (a->yextent & b->yextent)) )
    return 0;
#endif
  return  ((int)( GE(b->xmax, a->xmin) && GE(a->xmax, b->xmin)
                  && GE(b->ymax, a->ymin) && GE(a->ymax, b->ymin)));
}


static void
get_plane (p, a, b, c, d)
		 struct Polygon GPHUGE *p;
     double *a, *b, *c, *d;
{
  int i;
	struct Vertex GPHUGE *v1, GPHUGE *v2;
  double x, y, z, s;
  if (p->n == 1)
    {
      *a = 0.0;
      *b = 0.0;
      *c = 1.0;
      *d = -vlist[p->vertex[0]].z;
      return;
    }
  v1 = vlist + p->vertex[p->n - 1];
  v2 = vlist + p->vertex[0];
  *a = (v1->y - v2->y) * (v1->z + v2->z);
  *b = (v1->z - v2->z) * (v1->x + v2->x);
  *c = (v1->x - v2->x) * (v1->y + v2->y);
  for (i = 0; i < p->n - 1; i++)
    {
      v1 = vlist + p->vertex[i];
      v2 = vlist + p->vertex[i + 1];
      *a += (v1->y - v2->y) * (v1->z + v2->z);
      *b += (v1->z - v2->z) * (v1->x + v2->x);
      *c += (v1->x - v2->x) * (v1->y + v2->y);
    }
  s = sqrt( *a* *a + *b * *b + *c * *c);
  if (GE (0.0, s))
    {                           /* => s==0 => the vertices are in one line */
      v1 = vlist + p->vertex[0];
      for (i = 1; i < p->n; i++)
	{
          v2 = vlist + p->vertex[i];
	  if (!V_EQUAL (v1, v2))
	    break;
	}
      /* (x,y,z) is l.u. from <v1, v2> */
      x = v1->x;
      y = v1->y;
      z = v1->z;
      if (EQ (y, v2->y))
	y += 1.0;
      else
	x += 1.0;
      /* build a vector that is orthogonal to the line of the polygon */
      *a = v1->y * (v2->z - z) + v2->y * (z - v1->z) + y * (v1->z - v2->z);
      *b = v1->z * (v2->x - x) + v2->z * (x - v1->x) + z * (v1->x - v2->x);
      *c = v1->x * (v2->y - y) + v2->x * (y - v1->y) + x * (v1->y - v2->y);
      s = sqrt(*a * *a + *b * *b + *c * *c);
    }
  if (*c < 0.0) /* ensure that normalized c is > 0 */
    s *= -1.0;			/* The exact relation, because c can be very small */
  *a /= s;
  *b /= s;
  *c /= s;
  *d = -*a * v1->x - *b * v1->y - *c * v1->z;
  return;
}


static t_poly_plane_intersect
polygon_plane_intersection(p, a, b, c, d)
		 struct Polygon GPHUGE *p;
     double a, b, c, d;
{
  int i, sign, max_sign, min_sign;
	struct Vertex GPHUGE *v;

  CHECK_POINTER(plist,p);
  
  v = vlist + p->vertex[0];
  max_sign = min_sign = SIGNOF (a * v->x + b * v->y + c * v->z + d);
  for (i=1; i<p->n; i++) {
          v = vlist + p->vertex[i];
    sign=SIGNOF (a * v->x + b * v->y + c * v->z + d);
    if (sign > max_sign)
      max_sign=sign;
    else if (sign < min_sign)
      min_sign=sign;
	}

  /* Yes, this is a bit tricky, but it's simpler for the computer... */
  if (min_sign==-1) {
    if (max_sign==1)
      return (intersects);
    else
      return behind;
  } else {
    if ((max_sign==0) && (min_sign==0))
      return (inside);
    else
      return infront;
    }
}


/* full xy-overlap test (support routines first) */

/* What do negative return values mean?
   It is allowed, that the 2 polygons touch themselves in one point.
   There are 2 possibilities of this case:
   (A) A vertex of the one polygon is on an edge of the other
   (B) The two polygons have a vertex together.
   In case (A) the algorithm finds 2 edges, wich touches an edge of the
   other polygon. In case (B) the algorithm finds four pairs of edges.
   I handle both cases with negative return values:
   Case (A) returns -2, and case (B) returns -1.
   So I can say, if the sum of negative return values goes greater than 4
   (absolutly), there must be more than one touch point. So the 
   polygons must overlap. */

/* some variables, for keeping track of minima and maxima across
 * all these routines */

static double m1x, M1x, m1y, M1y, m2x, M2x, m2y, M2y;

#define MINIMAX  (GE(M2x,m1x) && GE(M1x,m2x) && GE(M2y,m1y) && GE(M1y,m2y))


/* Does the edge [v1,v2] intersect edge [v3, v4] ?
 * This is for non-vertical [v1,v2]
 */
static int 
intersect (v1, v2, v3, v4)
		 struct Vertex GPHUGE *v1, GPHUGE *v2, GPHUGE *v3, GPHUGE *v4;
{
  double m1, m2, t1, t2, x, y, minx, maxx;

  m1 = (v2->y - v1->y) / (v2->x - v1->x);
  t1 = v1->y - m1 * v1->x;

  /* if [v3,v4] vertical: */
  if (EQ (v3->x, v4->x))
    {
      y = v3->x * m1 + t1;
      if (GR (m2x, m1x) && GR (M1x, m2x))
	{
	  if (GR (y, m2y) && GR (M2y, y))
	    return 1;
	  if (EQ (y, m2y) || EQ (y, M2y))
	    return -2;
	  return 0;
	}
      /* m2x==m1x || m2x==M1x */
      if (GR (y, m2y) && GR (M2y, y))
	return -2;
      if (EQ (y, m2y) || EQ (y, M2y))
	return -1;
      return 0;
    }

  /* [v3,v4] not vertical */
  m2 = (v4->y - v3->y) / (v4->x - v3->x);
  t2 = v3->y - m2 * v3->x;
  if (!SIGNOF (m1 - m2))  /* if edges are parallel: */
    {
      x = m1 * v3->x + t1 - v3->y;
      if (!EQ (m1 * v3->x + t1 - v3->y, 0.0))
	return 0;
      if (GR (M2x, m1x) && GR (M1x, m2x))
	return 1;
      return -1;                /* the edges have a common vertex */
    }
  x = (t2 - t1) / (m1 - m2);
  minx = GPMAX (m1x, m2x);
  maxx = GPMIN (M1x, M2x);
  if (GR (x, minx) && GR (maxx, x))
    return 1;
  if (GR (minx, x) || GR (x, maxx))
    return 0;
  if ((EQ (x, m1x) || EQ (x, M1x)) && (EQ (x, m1x) || EQ (x, M1x)))
    return -1;
  return -2;
}

/* Does the edge [v1,v2] intersect edge [v3, v4] ?
 * This is for vertical [v1,v2]
 */
static int 
v_intersect (v1, v2, v3, v4)
		 struct Vertex GPHUGE *v1, GPHUGE *v2, GPHUGE *v3, GPHUGE *v4;
{
  double y;

	/* HBB 971115: to silence picky compilers, use v2 at least once, in
	 * a dummy expression (caution, may be ANSI-C specific because of
	 * the use of 'void'...) */
	(void) v2;

  /* if [v3,v4] is vertical, too: */
  /* already known: rectangles do overlap, because MINIMAX was true */
  if (EQ (v3->x, v4->x))
    return (GR (M2y, m1y) && GR (M1y, m2y)) ? 1 : -1;

  /*  [v3,v4] not vertical */
  y = v3->y + (v1->x - v3->x) * (v4->y - v3->y) / (v4->x - v3->x);
  if (GR (m1x, m2x) && GR (M2x, m1x))
    {
      if (GR (y, m1y) && GR (M1y, y))
	return 1;
      if (EQ (y, m1y) || EQ (y, M1y))
	return -2;
      return 0;
    }
  /* m1x==m2x || m1x==M2x */
  if (GR (y, m1y) && GR (M1y, y))
    return -2;
  if (EQ (y, m1y) || EQ (y, M1y))
    return -1;
  return 0;
}

#define UPD_MINMAX(v1,v2,npar) do {    \
  if (v1->x< v2->x)                 \
    CONCAT3(m,npar,x) =v1->x, CONCAT3(M,npar,x) =v2->x;   \
  else                              \
    CONCAT3(m,npar,x) =v2->x, CONCAT3(M,npar,x) =v1->x;   \
  if (v1->y< v2->y)                 \
    CONCAT3(m,npar,y)=v1->y, CONCAT3(M,npar,y)=v2->y;   \
  else                              \
    CONCAT3(m,npar,y)=v2->y, CONCAT3(M,npar,y)=v1->y;   \
} while (0)

/* does the edge [v1,v2] intersect polygon p? */
static int 
intersect_polygon (v1, v2, p)
		 struct Vertex GPHUGE *v1, GPHUGE *v2;
		 struct Polygon GPHUGE *p;
{
	struct Vertex GPHUGE *v3, GPHUGE *v4;
  int i, s, t = 0;
	int (*which_intersect) __PROTO((struct Vertex GPHUGE *v1, struct Vertex GPHUGE *v2,
						struct Vertex GPHUGE *v3, struct Vertex GPHUGE *v4))
    = intersect;

  /* Is [v1,v2] vertical? If, use v_intersect() */
  if (EQ (v1->x, v2->x))
    which_intersect=v_intersect;

  UPD_MINMAX(v1,v2,1);

  /* test first edge of polygon p */
  v3 = vlist + p->vertex[p->n - 1];
  v4 = vlist + p->vertex[0];
  UPD_MINMAX(v3,v4,2);


    if (MINIMAX)
      {
        s = which_intersect (v1, v2, v3, v4);
        if (s == 1 || (s < 0 && (t += s) < -4))
          return 1;
      }
    /* and the other edges... */
    for (i = 0; i < p->n - 1; i++)
      {
        v3 = vlist + p->vertex[i];
        v4 = vlist + p->vertex[i + 1];
        UPD_MINMAX(v3,v4,2);
        if (!MINIMAX)
          continue;
        s = which_intersect (v1, v2, v3, v4);
        if (s == 1 || (s < 0 && (t += s) < -4))
          return 1;
      }
    return t;
  }

/* OK, now here comes the 'real' polygon intersection routine:
 * Do a and b overlap in x or y (full test):
 * Do edges intersect, do the polygons touch in two points,
 * or is a vertex of one polygon inside the other polygon? */

static int 
full_xy_overlap (a, b)
		 struct Polygon GPHUGE *a, GPHUGE *b;
{
	struct Vertex GPHUGE *v1, GPHUGE *v2, v;
  int i, s, t = 0;

  if (a->n < 2 || b->n < 2)
    return 0;
  v1 = vlist + a->vertex[a->n - 1];
  v2 = vlist + a->vertex[0];
  s = intersect_polygon (v1, v2, b);
  if (s == 1 || (s < 0 && (t += s) < -4))
    return 1;
  for (i = 0; i < a->n - 1; i++)
    {
      v1 = vlist + a->vertex[i];
      v2 = vlist + a->vertex[i + 1];
      s = intersect_polygon (v1, v2, b);
      if (s == 1 || (s < 0 && (t += s) < -4))
	return 1;
    }
  /* No edges intersect. Is one polygon inside the other? */
  /* The  inner polygon has the greater minx */
  if (a->xmin < b->xmin)
    {
			struct Polygon GPHUGE *temp = a;
      a = b;
      b = temp;
    }
  /* Now, a is the inner polygon */
  for (i = 0; i < a->n; i++)
    {
      v1 = vlist + a->vertex[i];
      /* HBB: lclint seems to have found a problem here: v wasn't
       * fully defined when passed to intersect_polygon() */
      v = *v1;
      v.x = -1.1;
      s = intersect_polygon (v1, &v, b);
      if (s == 0)
	return 0;		/* a is outside polygon b */
      v.x = 1.1;
      if (s == 1)
	{
	  s = intersect_polygon (v1, &v, b);
	  if (s == 0)
	    return 0;
	  if (s == 1)
	    return 1;
	}
      else if (intersect_polygon (v1, &v, b) == 0)
	return 0;
    }
#if 1 /* 970614: don't bother, just presume that they do overlap: */    
  return 1; 
#else
  print_polygon(a, "a");
  print_polygon(b, "b");
  graph_error ("Logic Error in full_xy_overlap");
  return -1; /* HBB: shut up gcc -Wall */
#endif /* 0/1 */
}


/* Helper routine for split_polygon_by_plane */
static long
build_new_vertex (V1, V2, w)
     long V1, V2;               /* the vertices, between which a new vertex is demanded */
     double w;									/* information about where between V1 and V2 it should be */
{
  long V;
	struct Vertex GPHUGE *v, GPHUGE *v1, GPHUGE *v2;

  if (EQ (w, 0.0))
    return V1;
  if (EQ (w, 1.0))
    return V2;

  /* We need a new Vertex */
  if (vert_free == nvert) /* Extend vlist, if necessary */
		vlist = (struct Vertex GPHUGE *)
			gp_realloc(vlist, (unsigned long)sizeof(struct Vertex)*(nvert+=10L),
								 "hidden vlist");
  V = vert_free++;
  v = vlist + V;
  v1 = vlist + V1;
  v2 = vlist + V2;

  v->x = (v2->x - v1->x) * w + v1->x;
  v->y = (v2->y - v1->y) * w + v1->y;
  v->z = (v2->z - v1->z) * w + v1->z;
  v->style = -1;

	/* 970610: additional checks, to prevent adding unnecessary vertices */
	if (V_EQUAL(v, v1)) {
		vert_free--;
		return V1;
	}
	if (V_EQUAL(v,v2)) {
		vert_free--;
		return V2;
	}
	
  return V;
}


/* Splits polygon p by the plane represented by its equation
 * coeffecients a to d.
 * return-value: part of the polygon, that is in front/back of plane
 * (Distinction necessary to ensure the ordering of polygons in
 * the plist stays intact)
 * Caution: plist and vlist can change their location!!!
 * If a point is in plane, it comes to the behind - part
 * (HBB: that may well have changed, as I cut at the 'in plane'
 * mechanisms heavily)
 */

static long 
split_polygon_by_plane (P, a, b, c, d, f)
     long P;			/* Polygon as index on plist */
     double a, b, c, d;
     TBOOLEAN f;			/* return value = Front(1) or Back(0) */
{
  int i, j;
	struct Polygon GPHUGE *p = plist + P;
	struct Vertex GPHUGE *v;
  int snew, stest;
  int in_plane;									/* what is about points in plane? */
  int cross1, cross2;           /* first vertices, after p crossed the plane */
  double a1=0.0, b1, a2=0.0, b2; /* parameters of the crosses */
  long Front;										/* the new Polygon */
  int n1, n2;
  long *v1, *v2;
  long line1, line2;
  long next_frag_temp;
      

  CHECK_POINTER(plist, p);
  
  in_plane = (EQ (c, 0.0) && f) ? 1 : -1;
  /* first vertex */
  v = vlist + p->vertex[0];

  CHECK_POINTER(vlist, v);
  
  b1 = a * v->x + b * v->y + c * v->z + d;
  stest = SIGNOF (b1);
#if OLD_SPLIT_INPLANE
	if (!stest)
		stest = in_plane;
#endif

  /* find first vertex that is on the other side of the plane */
  for (cross1 = 1, snew = stest; snew == stest && cross1 < p->n; cross1++) {
		a1 = b1;
		v = vlist + p->vertex[cross1];
		CHECK_POINTER(vlist,v);
		b1 = a * v->x + b * v->y + c * v->z + d;
		snew = SIGNOF (b1);
#if OLD_SPLIT_INPLANE
		if (!snew)
			snew = in_plane;
#endif
	}

  if (snew == stest) {
		/*HBB: this is possibly not an error in split_polygon, it's just
		 * not possible to split this polygon by this plane. I.e., the
		 * error is in the caller!  */
		fprintf(stderr, "split_poly failed, polygon nr. %ld\n", P);
		return (-1); /* return 'unsplittable' */
	}
	
  cross1--; /* now, it's the last one on 'this' side */

  /* first vertex that is on the first side again */
  for (b2 = b1, cross2 = cross1 + 1;
			 snew != stest && cross2 < p->n; cross2++) {
		a2 = b2;
		v = vlist + p->vertex[cross2];
		CHECK_POINTER(vlist,v);
		b2 = a * v->x + b * v->y + c * v->z + d;
		snew = SIGNOF (b2);
#if OLD_SPLIT_INPLANE
		if (!snew)
			snew = - in_plane;
#endif			
	}
  
	if (snew != stest) {
		/* only p->vertex[0] is on 'this' side */
		a2 = b2;
		v = vlist + p->vertex[0];
		CHECK_POINTER(vlist,v);
		b2 = a * v->x + b * v->y + c * v->z + d;
	} else
    cross2--; /* now it's the last one on 'the other' side */

  /* We need two new polygons instead of the old one: */
  n1 = p->n - cross2 + cross1 + 2;
  n2 = cross2 - cross1 + 2;
  v1 = (long *) gp_alloc ((unsigned long) sizeof (long) * n1,
													"hidden v1 for two new poly");
  v2 = (long *) gp_alloc ((unsigned long) sizeof (long) * n2,
													"hidden v2 for two new poly");
#if SHOW_SPLITTING_EDGES
	line1 = 1L << (n1-1);
	line2 = 1L << (n2-1);
#else
  line1 = line2 = 0L;
#endif
  v1[0] = v2[n2 - 1] =
		build_new_vertex (p->vertex[cross2 - 1],
											p->vertex[(cross2 < p->n) ? cross2 : 0], a2 / (a2 - b2));
  v2[0] = v1[n1 - 1] =
    build_new_vertex (p->vertex[cross1 - 1],
											p->vertex[cross1], a1 / (a1 - b1));

	/* Copy visibility from split edges to their fragments: */
  if (p->line & (1 << (cross1 - 1))) {
		line1 |= 1L << (n1 - 2);
		line2 |= 1L;
	}
  if (p->line & (1L << (cross2 - 1))) {
		line1 |= 1L;
		line2 |= 1L << (n2 - 2);
	}

	/* Copy the old line patterns and vertex listings into new places */
	/* p->vertex[cross2...p->n-1] --> v1[1...] */
  for (i = cross2, j = 1; i < p->n;) {
		if (p->line & (1L << i))
			line1 |= 1L << j;
		v1[j++] = p->vertex[i++];
	}
	/* p->vertex[0...cross1-1] --> v1[...n1-2] */
  for (i = 0; i < cross1 - 1;) {
		if (p->line & (1L << i))
			line1 |= 1L << j;
		v1[j++] = p->vertex[i++];
	}
  v1[j++] = p->vertex[i++];

	/* p->vertex[cross1...cross2-1] -> v2[1...n2-2] */
  for (j = 1; i < cross2 - 1;) {
		if (p->line & (1L << i))
			line2 |= 1L << j;
		v2[j++] = p->vertex[i++];
	}
  v2[j++] = p->vertex[i];

	/* old vertex list isn't needed any more: */
  free (p->vertex);
  p->vertex = 0;

  if (!reduce_polygon (&n1, &v1, &line1, 1) ||
      !reduce_polygon (&n2, &v2, &line2, 1))
    graph_error ("Logic Error 2 in split_polygon");

  /* Build the 2 new polygons : we are reusing one + making one new */
  CHECK_PLIST_EXTRA(p=plist+P);
  Front = pfree++;

  if ((next_frag_temp = p->next_frag) <0) 
    next_frag_temp = P; /* First split of this polygon at all */
      
  /* HBB 961110: lclint said == shouldn't be used on Boolean's */
  if ((f && (stest < 0)) || ((! f) && !(stest<0))) {
		build_polygon (plist + P, n1, v1, line1, p->style, p->lp,
									 p->next, Front, p->id, p->tested);
		build_polygon (plist + Front, n2, v2, line2, p->style, p->lp,
									 -1, next_frag_temp, p->id, is_moved_or_split);
	} else {
		build_polygon (plist + P, n2, v2, line2, p->style, p->lp,
									 p->next, Front, p->id, p->tested);
		build_polygon (plist + Front, n1, v1, line1, p->style, p->lp,
									 -1, next_frag_temp, p->id, is_moved_or_split);
	}
  return Front;
}

/* HBB: these pieces of code are found repeatedly in in_front(), so
 * I put them into macros
 * Note: can't use the 'do {...} while (0)' method for
 * these: 'continue' wouldn't work any more.
 */
#define PUT_P_IN_FRONT_TEST(new_tested) {\
  plist[Plast].next = p->next; \
  p->next = Test; \
  p->tested = new_tested; \
  if (Insert >= 0) \
    plist[Insert].next = P; \
  else \
    pfirst = P; \
  Insert = P; \
  P = Plast; \
  p = plist + P; \
  continue; \
}

#define SPLIT_TEST_BY_P {\
  long Back = split_polygon_by_plane (Test, pa, pb, pc, pd, 0); \
  if (Back >0) { \
    p = plist + P; \
    test = plist + Test;  /* plist can change */ \
    plist[Back].next = p->next; \
    p->next = Back; \
    P = Back; \
    p = plist + P; \
    zmin = test->zmin;  /* the new z-value */ \
  } else { \
    fprintf(stderr, "Failed to split test (%ld) by p (%ld)\n", Test, P); \
    graph_error("Error in hidden line removal: couldn't split..."); \
  } \
  continue; \
}

#define SPLIT_P_BY_TEST {\
  long Front = split_polygon_by_plane (P, ta, tb, tc, td, 1);\
  if (Front >0) {\
    p = plist + P;\
    test = plist + Test;	/* plist can change */\
    plist[Front].next = Test;\
    if (Insert >= 0)\
      plist[Insert].next = Front;\
    else\
      pfirst = Front;\
    Insert = Front;\
  } else {\
    fprintf(stderr, "Failed to split p (%ld) by test(%ld), relations are %d, %d\n",\
	    P, Test, p_rel_tplane, t_rel_pplane);\
    print_polygon(test, "test");\
    print_polygon(p, "p");\
    fprintf(stderr, "\n");\
  }\
  continue; /* FIXME: should we continue nevertheless? */\
}


/* Is Test in front of the other polygons?
 * If not, polygons which are in front of test come between
 * Last and Test (I.e. to the 'front' of the plist)
 */
static int 
in_front (Last, Test)
     long Last, Test;
{
	struct Polygon GPHUGE *test = plist + Test, GPHUGE *p;
  long Insert, P, Plast;
  double zmin,			/* minimum z-value of Test */
    ta, tb, tc, td,		/* the plane of Test      */
    pa, pb, pc, pd;		/* the plane of a polygon */
	t_poly_plane_intersect p_rel_tplane, t_rel_pplane;
  int loop = (test->tested == is_in_loop);

  CHECK_POINTER(plist,test);
  
	/* Maybe this should only done at the end of this routine... */
  /* test->tested = is_tested; */ 

  Insert = Last;

  /* minimal z-value of Test */
  zmin = test->zmin;

  /* The plane of the polygon Test */
  get_plane (test, &ta, &tb, &tc, &td);

  /* Compare Test with the following polygons, which overlap in z value */
  for (Plast = Test, p=test;
       ((P=p->next) >= 0) && (((p=plist+P)->zmax > zmin) || p->tested);
			 Plast = P) {
		CHECK_POINTER(plist,p);
		
		if (!xy_overlap (test, p))
			continue;
		
		if (p->zmax <= zmin)
			continue;
		
		p_rel_tplane=polygon_plane_intersection(p, ta, tb, tc, td);
		if ((p_rel_tplane==behind) || (p_rel_tplane==inside))
			continue;
		
		get_plane (p, &pa, &pb, &pc, &pd);
		t_rel_pplane=polygon_plane_intersection(test, pa, pb, pc, pd);
		if ((t_rel_pplane==infront) || (t_rel_pplane==inside))
			continue;
		
		if (!full_xy_overlap (test, p))
			continue;
		
#if 1 
		/* HBB 971115: try new approach developed directly from Foley et
		 * al.'s original description */
		/* OK, at this point, a total of 16 cases is left to be handled,
		 * as 4 variables are important, each with two possible values:
		 * t_rel_pplane may be 'behind' or 'intersects', p_rel_tplane may
		 * be 'infront' or 'intersects', 'test' may be (seem to be) part
		 * of a loop or not ('loop'), and p may have been considered
		 * already or not (p->tested =?= is_tested). */
		/* See if splitting wherever possible is a good idea: */
		if (p_rel_tplane == intersects) {
			p->tested = is_moved_or_split;
			SPLIT_P_BY_TEST;
		} else if (t_rel_pplane == intersects) {
			test->tested = is_moved_or_split;
			SPLIT_TEST_BY_P;
		} else {
			if (loop && (p->tested == is_tested)) {
				/* Ouch, seems like we're in trouble, really */
				fprintf(stderr, "#Failed: In loop, without intersections.\n");
				fprintf(stderr, "#Relations are %d, %d\n", p_rel_tplane, t_rel_pplane);
				print_polygon(test, "test");
				print_polygon(p, "p");
				continue; /* Keep quiet, maybe no-one will notice (;-) */
			} else {
				PUT_P_IN_FRONT_TEST(is_in_loop);
			}
		}
#else
		/* p obscures test (at least, this *might* be happening */
		if (loop)	{
			/* if P isn't interesting for the loop, put P in front of Test */
			if ((p->tested != is_tested)
					|| (p_rel_tplane==infront)
					|| (t_rel_pplane==behind)) /* HBB 970325: was infront (!?) */
				PUT_P_IN_FRONT_TEST(is_moved_or_split); 

			/* else, split either test or p */
			if (t_rel_pplane==intersects)
				SPLIT_TEST_BY_P;
			if (p_rel_tplane==intersects)
				SPLIT_P_BY_TEST;

			/* HBB: if we ever come through here, something went severely wrong */
			fprintf(stderr, "Failed: polygon %ld vs. %ld, relations are %d, %d\n", P, Test, p_rel_tplane, t_rel_pplane);
			print_polygon(test, "test");
			print_polygon(p, "p");
			fprintf(stderr, "\n");
			graph_error("Couldn't resolve a polygon overlapping pb. Go tell HBB...");
		}

		/* No loop: if it makes sense, put P in front of Test */
		if ((p->tested != is_tested)
				&& ((t_rel_pplane==behind)
						|| (p_rel_tplane==infront)))
			PUT_P_IN_FRONT_TEST(is_moved_or_split);

		/* If possible, split P */
		if ((p->tested != is_tested) || (p_rel_tplane==intersects))
			SPLIT_P_BY_TEST;

		/* if possible, split Test */
		if (t_rel_pplane==intersects)
			SPLIT_TEST_BY_P;

		/* else, we have a loop: mark P as loop and put it in front of Test */
		PUT_P_IN_FRONT_TEST(is_in_loop); /* -2: p is in a loop */
#endif
	}
	if (Insert == Last)
		test->tested = is_tested;
  return (int)(Insert == Last);
}


/* Drawing the polygons */

/* HBB 980303: new functions to handle Cross back-buffer (saves
 * gp_alloc()'s) */

static GP_INLINE void init_Cross_store()
{
	assert (!Cross_store && !last_Cross_store);
	last_Cross_store = CROSS_STORE_INCREASE;
	free_Cross_store = 0;
	Cross_store = (struct Cross *)
		gp_alloc ((unsigned long) last_Cross_store * sizeof (struct Cross),
							"hidden cross store");
}

static GP_INLINE struct Cross * get_Cross_from_store()
{
	while (last_Cross_store <= free_Cross_store) {
		last_Cross_store += CROSS_STORE_INCREASE;
		Cross_store = (struct Cross *)
			gp_realloc(Cross_store,
								 (unsigned long) last_Cross_store * sizeof (struct Cross),
								 "hidden cross store");
	}
	return Cross_store+(free_Cross_store++);
}

static GP_INLINE TBOOLEAN
hl_buffer_set (xv, yv)
     int xv, yv;
{
  struct Cross GPHUGE *c;
  /*HBB 961110: lclint wanted this: */
  assert (hl_buffer !=0);
  for (c = hl_buffer[xv]; c != NULL; c = c->next)
    if (c->a <= yv && c->b >= yv) {
      return TRUE;
    }
  return FALSE; 
}

/* HBB 961201: new var's, to speed up free()ing hl_buffer later */
static int hl_buff_xmin, hl_buff_xmax;

/* HBB 961124: new routine. All this occured twice in
 * draw_clip_line_buffer */
/* Store a line crossing the x interval around xv between y=ya and
 * y=yb in the hl_buffer */
static GP_INLINE void
update_hl_buffer_column(xv, ya, yb) 
     int xv, ya, yb;
{
  struct Cross GPHUGE * GPHUGE *cross, GPHUGE *cross2;

  /* First, ensure that ya <= yb */
  if (ya > yb) {
		int y_temp = yb;
		yb = ya;
		ya = y_temp;
	}
  /* loop over all previous crossings at this x-value */
	for (cross = hl_buffer + xv; 1; cross = &(*cross)->next) {
		if (*cross == NULL)	{
			/* first or new highest crossing at this x-value */

			/* HBB 980303: new method to allocate Cross structures */
			*cross = get_Cross_from_store();
		  (*cross)->a = ya;
		  (*cross)->b = yb;
		  (*cross)->next = NULL;
			/* HBB 961201: keep track of x-range of hl_buffer, to speedup free()ing it */
			if (xv < hl_buff_xmin)
				hl_buff_xmin = xv;
			if (xv > hl_buff_xmax)
				hl_buff_xmax = xv;
		  break;
		}
		if (yb < (*cross)->a - 1)	{
			/* crossing below 'cross', create new entry before 'cross' */
			cross2 = *cross;
			/* HBB 980303: new method to allocate Cross structures */
			*cross = get_Cross_from_store();
		  (*cross)->a = ya;
		  (*cross)->b = yb;
		  (*cross)->next = cross2;
		  break;
		}
		else if (ya <= (*cross)->b + 1)	{
			/* crossing overlaps or covers 'cross' */
			if (ya < (*cross)->a)
				(*cross)->a = ya;
			if (yb > (*cross)->b) {
				if ((*cross)->next && (*cross)->next->a <= yb) {
					/* crossing spans all the way up to 'cross->next' so
					 * unite them */
					cross2 = (*cross)->next;
					(*cross)->b = cross2->b;
					(*cross)->next = cross2->next;
				}	else
					(*cross)->b = yb;
			}
		  break;
		}
	}
  return;
}


static void
draw_clip_line_buffer (x1, y1, x2, y2)
     int x1, y1, x2, y2;
     /* Draw a line in the hl_buffer */
{
  register int xv, yv, errx, erry, err;
  register int dy, nstep;
  register int ya;
  int i;

  if (!clip_line (&x1, &y1, &x2, &y2)) {
    /*printf("dcl_buffer: clipped away!\n");*/
    return;
	}
  if (x1 > x2) {
		errx = XREDUCE (x1) - XREDUCE (x2);
		erry = YREDUCE (y1) - YREDUCE (y2);
		xv = XREDUCE (x2) - XREDUCE (xleft);
		yv = YREDUCE (y2) - YREDUCE (ybot);
	} else {
		errx = XREDUCE (x2) - XREDUCE (x1);
		erry = YREDUCE (y2) - YREDUCE (y1);
		xv = XREDUCE (x1) - XREDUCE (xleft);
		yv = YREDUCE (y1) - YREDUCE (ybot);
	}
  if (erry > 0)
    dy = 1;
  else {
		dy = -1;
		erry = -erry;
	}
  nstep = errx + erry;
  err = -errx + erry;
  errx <<= 1;
  erry <<= 1;
  ya = yv;

  for (i = 0; i < nstep; i++)	{
		if (err < 0) {
			update_hl_buffer_column(xv, ya, yv);
			xv++;
			ya = yv;
			err += erry;
		} else {
			yv += dy;
			err -= errx;
		}
	}
  (void)update_hl_buffer_column(xv, ya, yv);
  return;
}


/* HBB 961124: improve similarity of this routine with
 * draw_clip_line_buffer ()*/
/* HBB 961124: changed from 'virtual' to 'do_draw', for clarity (this
 * also affects the routines calling this one, so beware! */
/* HBB 961124: introduced checking code, to search for the 'holes in
 * the surface' bug */
/* Draw a line into the bitmap (**cpnt), and optionally also to the
 * terminal */
static void
draw_clip_line_update (x1, y1, x2, y2, do_draw)
     int x1, y1, x2, y2;
     TBOOLEAN do_draw; 
{
  /* HBB 961110: made 'flag' a boolean variable, which needs some other changes below as well */
	/* HBB 970508: renamed 'flag' to 'is_drawn' (reverting its meaning). Should be clearer now */
  TBOOLEAN is_drawn;
  register struct termentry *t = term;
  register int xv, yv, errx, erry, err;
  register int xvr, yvr;
  int xve, yve;
  register int dy, nstep=0, dyr;
  int i;
  if (!clip_line (&x1, &y1, &x2, &y2)) {
    /*printf("dcl_update: clipped away!\n");*/
    return;
  }
  if (x1 > x2) {
		xvr = x2;
		yvr = y2;
		xve = x1;
		yve = y1;
	} else {
		xvr = x1;
		yvr = y1;
		xve = x2;
		yve = y2;
	}
  errx = XREDUCE (xve) - XREDUCE (xvr);
  erry = YREDUCE (yve) - YREDUCE (yvr);
  if (erry > 0)
    dy = 1;
  else {
		dy = -1;
		erry = -erry;
	}
  dyr = dy * yfact;
  nstep = errx + erry;
  err = -errx + erry; 
  errx <<= 1;
  erry <<= 1;
  xv = XREDUCE (xvr) - XREDUCE (xleft);
  yv = YREDUCE (yvr) - YREDUCE (ybot);
  (*t->move) ((unsigned int)xvr, (unsigned int)yvr);

  is_drawn = (IS_UNSET (xv, yv) && (do_draw || hl_buffer_set (xv, yv)));

  /* HBB 961110: lclint wanted these: */
  assert (ymin_hl !=0);
  assert (ymax_hl !=0);
  /* Check first point in hl-buffer */
  if (xv < xmin_hl) xmin_hl = xv;
  if (xv > xmax_hl) xmax_hl = xv;
  if (yv < ymin_hl[xv]) ymin_hl[xv] = yv;
  if (yv > ymax_hl[xv]) ymax_hl[xv] = yv;
  for (i = 0; i < nstep; i++) {
		unsigned int xvr_temp = xvr, yvr_temp = yvr; /* 970722: new variables */
		if (err < 0) {
			xv++;
			xvr += xfact;
			err += erry;
		} else {
			yv += dy;
			yvr += dyr;
			err -= errx; 
		}
		if (IS_UNSET (xv, yv) && (do_draw || hl_buffer_set (xv, yv))) {
			if (!is_drawn) {
				/* 970722: If this line was meant to be drawn, originally, and it was
				 * just the bitmap that stopped it, then draw it a bit longer (i.e.:
				 * start one pixel earlier */
				if (do_draw) 
					(*t->move) ((unsigned int)xvr_temp, (unsigned int)yvr_temp);
				else
	      (*t->move) ((unsigned int)xvr, (unsigned int)yvr);
	      is_drawn = TRUE;
	    }
		} else {
			if (is_drawn) {
				/* 970722: If this line is only drawn because hl_buffer_set() was true,
				 * then don't extend to the new position (where it isn't true any more).
				 * Draw the line one pixel shorter, instead: */
				if (do_draw)
	      (*t->vector) ((unsigned int)xvr, (unsigned int)yvr);
				else {
					(*t->vector) ((unsigned int)xvr_temp, (unsigned int)yvr_temp);
					(*t->move) ((unsigned int)xvr, (unsigned int)yvr);
				}
	      is_drawn = FALSE;
	    }
		}
		if (xv < xmin_hl) xmin_hl = xv;
		if (xv > xmax_hl) xmax_hl = xv;
		if (yv < ymin_hl[xv]) ymin_hl[xv] = yv;
		if (yv > ymax_hl[xv]) ymax_hl[xv] = yv;
	}
  if (is_drawn) 
    (*t->vector) ((unsigned int)xvr, (unsigned int)yvr);
  return;
}

#define DRAW_VERTEX(v, x, y) do { \
   if ((v)->style>=0 &&                                            \
       !clip_point(x,y)  &&                                        \
       IS_UNSET(XREDUCE(x)-XREDUCE(xleft),YREDUCE(y)-YREDUCE(ybot))) \
     (*t->point)(x,y, v->style);                                   \
   (v)->style=-1;                                                  \
} while (0)

/* Two routine to emulate move/vector sequence using line drawing routine. */
static unsigned int move_pos_x, move_pos_y;

void
clip_move(x,y)
		 unsigned int x,y;
{
    move_pos_x = x;
    move_pos_y = y;
}

void
clip_vector(x,y)
		 unsigned int x,y;
{
    draw_clip_line(move_pos_x,move_pos_y, x, y);
    move_pos_x = x;
    move_pos_y = y;
}

static GP_INLINE void
clip_vector_h(x,y)
		 int x,y;
     /* Draw a line on terminal and update bitmap */
{
  draw_clip_line_update(move_pos_x,move_pos_y, x, y, TRUE);
   move_pos_x = x;
   move_pos_y = y;
}
           

static GP_INLINE void
clip_vector_virtual(x,y)
     int x,y;
     /* update bitmap, do not really draw the line */
{
  draw_clip_line_update(move_pos_x,move_pos_y, x, y, FALSE);
  move_pos_x = x;    
  move_pos_y = y;
}

static GP_INLINE void
clip_vector_buffer(x,y)
     /* draw a line in the hl_buffer */
     int x,y;
{
  draw_clip_line_buffer(move_pos_x,move_pos_y, x, y);
  move_pos_x = x;    
  move_pos_y = y;
}

static void
draw (p)
		 struct Polygon GPHUGE *p;
{
  struct Vertex GPHUGE *v;
  struct Polygon GPHUGE *q;
  long Q;
  struct Cross *cross1, *cross2;
  int i;
  int x, y;			/* point in terminal coordinates */
  register struct termentry *t = term;
  t_pnt mask1, mask2;
  long int indx1, indx2, k;
  tp_pnt cpnt;

  xmin_hl = HL_EXTENT_X_MAX;	/* HBB 961124: parametrized this value */
  xmax_hl = 0;

  /* HBB 961201: store initial values for range of hl_buffer: */
  hl_buff_xmin = XREDUCE(xright) - XREDUCE(xleft);  
  hl_buff_xmax = 0;
  
  /* Write the lines of all polygons with the same id as p in the hl_buffer */
	/* HBB 961201: use next_frag pointers to loop over fragments: */
  for (q=p; (Q=q->next_frag) >= 0 && (q=plist+Q) != p; ) {
		if (q->id != p->id)
			continue;

		/* draw the lines of q into hl_buffer: */
		v = vlist + q->vertex[0];
		TERMCOORD (v, x, y);
		clip_move (x, y);
		for (i = 1; i < q->n; i++) {
			v = vlist + q->vertex[i];
			TERMCOORD (v, x, y);
			if (q->line & (1L << (i - 1)))
				clip_vector_buffer (x, y);
			else
				clip_move (x, y);
		}
		if (q->line & (1L << (i - 1))) {
			v = vlist + q->vertex[0];
			TERMCOORD (v, x, y);
			clip_vector_buffer (x, y);
		}
	}
	

	/* first, set the line and point styles: */
  {
    struct lp_style_type lp;

    lp = *(p->lp);
    lp.l_type = p->style;
    term_apply_lp_properties(&(lp));
  }

  /*print_polygon (p, "drawing"); */ 

  /* draw the lines of p */
  v = vlist + p->vertex[0];
  TERMCOORD (v, x, y);
  clip_move (x, y);
  DRAW_VERTEX (v, x, y);
  for (i = 1; i < p->n; i++) {
		v = vlist + p->vertex[i];
		TERMCOORD (v, x, y);
		if (p->line & (1L << (i - 1)))
			clip_vector_h (x, y);
		else
			clip_vector_virtual (x, y);
		DRAW_VERTEX (v, x, y);
	}
  TERMCOORD (vlist + p->vertex[0], x, y);
  if (p->line & (1L << (i - 1)))
    clip_vector_h (x, y);
  else
    clip_vector_virtual (x, y);


  /* reset the hl_buffer */
  /*HBB 961110: lclint wanted this: */
  assert (hl_buffer);
  for (i = hl_buff_xmin ; i<=hl_buff_xmax ; i++) {
    /* HBB 980303: one part was removed here. It isn't needed any more,
     * with the global store for Cross structs. */
		hl_buffer[i] = NULL;
	}
	/* HBB 980303: instead, set back the free pointer of the Cross store:*/
	free_Cross_store = 0;
	
  /* now mark the area as being filled in the bitmap. */
	/* HBB 971115: Yes, I do know that xmin_hl is unsigned, and the
	 * first test is thus useless. But I'd like to keep it anyway ;-) */
  if (xmin_hl < 0 || xmax_hl > XREDUCE (xright) - XREDUCE (xleft))
    graph_error ("Logic error #3 in hidden line");
	/* HBB 961110: lclint wanted these: */
  assert (ymin_hl !=0);
  assert (ymax_hl !=0);
  assert (pnt !=0); 
  if (xmin_hl < xmax_hl)
    for (i = xmin_hl; i <= xmax_hl; i++) {
			if (ymin_hl[i] == HL_EXTENT_Y_MAX)
				graph_error ("Logic error #2 in hidden line");
			if (pnt[i] == 0) {
				pnt[i] = (t_pnt *) gp_alloc ((unsigned long)y_malloc, "hidden ymalloc");
				memset (pnt[i], 0, (size_t)y_malloc);
			}
			if (ymin_hl[i] < 0 || ymax_hl[i] > YREDUCE (ytop) - YREDUCE (ybot))
				graph_error ("Logic error #1 in hidden line");
			/* HBB 970508: this shift was wordsize dependent. I think I fixed that. */
			indx1 = ymin_hl[i] / PNT_BITS;
			indx2 = ymax_hl[i] / PNT_BITS;
			mask1 = PNT_MAX << (((unsigned) ymin_hl[i]) % PNT_BITS);
			mask2 = PNT_MAX >> ((~((unsigned) ymax_hl[i])) % PNT_BITS);
			cpnt = pnt[i] + indx1;
			if (indx1 == indx2)
				*cpnt |= (mask1 & mask2);
			else {
				*cpnt++ |= mask1;
				for (k=indx1+1; k<indx2; k++)
					*cpnt++ = PNT_MAX;
				*cpnt |= mask2;
			}
			ymin_hl[i] = HL_EXTENT_Y_MAX;
			ymax_hl[i] = 0;
		}
}


void 
plot3d_hidden (plots, pcount)
     struct surface_points *plots;
     int pcount;
{
  long Last, This;
  long i;

#if defined(DOS16) || defined(WIN16)
  /* HBB 980309: Ensure that Polygon Structs exactly fit a 64K segment. The
   * problem this prevents is that even with 'huge pointers', a single
   * struct may not cross a 64K boundary: it will wrap around to the
   * beginning of that 64K segment. Someone ought to complain about
   * this nuisance, somewhere... :-( */
  assert (0 == (0x1 << 16) % (sizeof(struct Polygon)) );
#endif

  /* Initialize global variables */
  y_malloc = (2 + (YREDUCE (ytop) >> 4) - (YREDUCE (ybot) >> 4)) * sizeof (t_pnt);
  /* ymin_hl, ymax_hl: */
  i = sizeof (t_hl_extent_y) * (XREDUCE (xright) - XREDUCE (xleft) + 1);
  ymin_hl = (t_hl_extent_y *) gp_alloc ((unsigned long) i, "hidden ymin_hl");
  ymax_hl = (t_hl_extent_y *) gp_alloc ((unsigned long) i, "hidden ymax_hl");
  for (i = (XREDUCE (xright) - XREDUCE (xleft)); i >= 0; i--) {
		ymin_hl[i] = HL_EXTENT_Y_MAX;
		ymax_hl[i] = 0;
	}
  /* hl_buffer: */
	/* HBB 980303 new: initialize the global store for Cross structs: */
	init_Cross_store();
  i = XREDUCE (xright) - XREDUCE (xleft) + 1;
  hl_buffer =
    (struct Cross GPHUGE * GPHUGE *) gp_alloc ((unsigned long)(i*sizeof(struct Cross GPHUGE *)),
																"hidden hl_buffer");
  while (--i>=0)
    hl_buffer[i] = (struct Cross *) 0;

  init_polygons (plots, pcount);

	/* HBB 970326: if no polygons survived the cutting away of undefined
	 * and/or outranged vertices, bail out with a warning message.
	 * Without this, sort_by_zmax gets a SegFault */
	if (!pfree) {
		/* HBB 980701: new code to deallocate all the structures allocated up
		 * to this point. Should fix the core dump reported by 
		 * Stefan A. Deutscher today  */
    if (ymin_hl) {
  		free (ymin_hl);
  		ymin_hl = 0;
  	}
    if (ymax_hl) {
  		free (ymax_hl);
  		ymax_hl = 0;
  	}
    if (hl_buffer) {
  		free (hl_buffer);
  		hl_buffer = 0;
  	}
  	if (Cross_store) {
  		free (Cross_store);
  		Cross_store = 0;
  		last_Cross_store = 0;
  	}
		 
		graph_error("*All* polygons undefined or out of range, thus no plot.");
	} 
		
  sort_by_zmax ();

  /* sort the polygons and draw them in one loop */
  Last = -1L;
  This = pfirst;
  /* Search first polygon, that can be drawn */
  while (Last == -1L && This >= 0L) {
		This = pfirst;
		if (obscured (plist + This)) {
			Last = This;
			continue;
		}
		if (plist[This].tested != is_tested && !in_front (Last, This))
			continue;
		draw (plist + This);
		Last = This;
	}
  /* Draw the other polygons */
  for (This = plist[Last].next; This >= 0L; This = plist[Last].next) {
		if (obscured (plist + This)) {
			Last = This;
			continue;
		}
		if (plist[This].tested != is_tested && !in_front (Last, This))
			continue;
		draw (plist + This);
		Last = This;
	}
	
  /* Free memory */
  if (plist) {
		for (This = 0L; This < pfree; This++)
			free (plist[This].vertex);
		free (plist);
		plist = 0;
	}
  if (vlist) {
		free (vlist);
		vlist = 0;
	}
  if (ymin_hl) {
		free (ymin_hl);
		ymin_hl = 0;
	}
  if (ymax_hl) {
		free (ymax_hl);
		ymax_hl = 0;
	}
  if (hl_buffer) {
		free (hl_buffer);
		hl_buffer = 0;
	}
	if (Cross_store) {
		free (Cross_store);
		Cross_store = 0;
		last_Cross_store = 0;
	}
}

/* To ease editing with emacs: */
/* Local Variables: */
/* tab-width:2 */
/* END: */
