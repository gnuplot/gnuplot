#ifndef lint
static char *RCSid() { return RCSid("$Id: hidden3d.c,v 1.18.2.2 2000/10/23 04:35:27 joze Exp $"); }
#endif

/* GNUPLOT - hidden3d.c */

/*[
 * Copyright 1986 - 1993, 1998, 1999   Thomas Williams, Colin Kelley
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
 * 1999 Hans-Bernhard Br"oker (Broeker@physik.rwth-aachen.de)
 *
 * Major rewrite, affecting just about everything
 *
 */

#include "hidden3d.h"

#ifdef PM3D
# include "color.h"
# include "pm3d.h"
#endif

#include "alloc.h"
#include "command.h"
#include "dynarray.h"
#include "graphics.h"
#include "graph3d.h"
#include "parse.h"
#include "setshow.h"
#include "tables.h"
#include "term_api.h"
#include "util.h"
#include "util3d.h"


/*************************/
/* Configuration section */
/*************************/

/* If this module is compiled with TEST_GRIDBOX = 1 defined, it
 * will store the information about {x|y}{min|max} in an other
 * (additional!) form: a bit mask, with each bit representing one
 * horizontal or vertical strip of the screen. The bits for strips a
 * polygon spans are set to one. This allows to test for xy overlap
 * of an edge with a polygon simply by comparing bit patterns.  */
#ifndef TEST_GRIDBOX
#define TEST_GRIDBOX 1
#endif

/* If you don't want the color-distinction between the
 * 'top' and 'bottom' sides of the surface, like I do, then just compile
 * with -DBACKSIDE_LINETYPE_OFFSET = 0. */
#ifndef BACKSIDE_LINETYPE_OFFSET
#define BACKSIDE_LINETYPE_OFFSET 1
#endif

/* This #define lets you choose if the diagonals that
 * divide each original quadrangle in two triangles will be drawn
 * visible or not: do draw them, define it to be 7L, otherwise let be
 * 3L */
#ifndef TRIANGLE_LINESDRAWN_PATTERN
#define TRIANGLE_LINESDRAWN_PATTERN 3L
#endif

/* Handle out-of-range or undefined points. Compares the maximum
 * marking (0=inrange, 1=outrange, 2=undefined) of the coordinates of
 * a vertex to the value #defined here. If not less, the vertex is
 * rejected, and all edges that hit it, as well. NB: if this is set to
 * anything above 1, gnuplot may crash with a floating point exception
 * in hidden3d. You get what you asked for ... */
#ifndef HANDLE_UNDEFINED_POINTS
#define HANDLE_UNDEFINED_POINTS 1
#endif
/* Symbolic value for 'do not handle Undefined Points specially' */
#define UNHANDLED (UNDEFINED+1)

/* If both subtriangles of a quad were cancelled, try if using the
 * other diagonal is better. This only makes a difference if exactly
 * one vertex of the quad is unusable, and that one is on the 'usual'
 * tried diagonal. In such a case, the border of the hole in the
 * surface will be less rough than with the previous method, as the
 * border follows the undefined region as close as it can. */
#ifndef SHOW_ALTERNATIVE_DIAGONAL
#define SHOW_ALTERNATIVE_DIAGONAL 1
#endif

/* If the two triangles in a quad are both drawn, and they show
 * different sides to the user (the quad is 'bent over'), then it's
 * preferrable to force the diagonal being visible to avoid other
 * parts of the scene being obscured by a line the user can't
 * see. This avoids unnecessary user surprises. */
#ifndef HANDLE_BENTOVER_QUADRANGLES
#define HANDLE_BENTOVER_QUADRANGLES 1
#endif

/* The actual configuration is stored in these variables, modifiable
 * at runtime through 'set hidden3d' options */
static int hiddenBacksideLinetypeOffset = BACKSIDE_LINETYPE_OFFSET;
static long hiddenTriangleLinesdrawnPattern = TRIANGLE_LINESDRAWN_PATTERN;
static int hiddenHandleUndefinedPoints = HANDLE_UNDEFINED_POINTS;
static int hiddenShowAlternativeDiagonal = SHOW_ALTERNATIVE_DIAGONAL;
static int hiddenHandleBentoverQuadrangles = HANDLE_BENTOVER_QUADRANGLES;


/**************************************************************/
/**************************************************************
 * The 'real' code begins, here.                              *
 *                                                            *
 * first: types and global variables                          *
 **************************************************************/
/**************************************************************/

/* precision of calculations in normalized space. Coordinates closer to
 * each other than an absolute difference of EPSILON are considered
 * equal, by some of the routines in this module. */
#define EPSILON 1e-5

/* Some inexact operations: == , > , >=, sign() */
#define EQ(X,Y)  (fabs( (X) - (Y) ) < EPSILON)	/* X == Y */
#define GR(X,Y)  ((X) >  (Y) + EPSILON)		/* X >  Y */
#define GE(X,Y)  ((X) >= (Y) - EPSILON)		/* X >= Y */
#define SIGN(X)  ( ((X)<-EPSILON) ? -1: ((X)>EPSILON) )


/* Utility macros for vertices: */
#define FLAG_VERTEX_AS_UNDEFINED(v) \
  do { (v).z = -2.0; } while (0)
#define VERTEX_IS_UNDEFINED(v) ((v).z == -2.0)
#define V_EQUAL(a,b) ( GE(0.0, fabs((a)->x - (b)->x) + \
   fabs((a)->y - (b)->y) + fabs((a)->z - (b)->z)) )

/* A plane equation, stored as a four-element vector. The equation
 * itself is: x*p[0]+y*p[1]+z*p[2]+1*p[3]=0 */
typedef coordval t_plane[4];

/* One edge of the mesh. The edges are (currently) organized into a
 * linked list as a method of traversing them back-to-front. */
typedef struct edge {
    long v1, v2;		/* the vertices at either end */
    int style;			/* linetype index */
    struct lp_style_type *lp;	/* line/point style attributes */
    long next;			/* index of next edge in z-sorted list */
} edge;
typedef edge GPHUGE *p_edge;


/* One polygon (actually: always a triangle) of the surface mesh(es).
 * To allow easier take-over of old routines, I've left the 'n'
 * structure element in. FIXME: that should be changed, sometime soon.
 * */
typedef struct polygon {
    int n;			/* # of vertices. always 3 */
    long vertex[3];		/* The vertices (indices on vlist) */
    coordval xmin, xmax, ymin, ymax, zmin, zmax;
    /* min/max in all three directions */
    t_plane plane;		/* the plane coefficients */
    TBOOLEAN frontfacing;	/* is polygon facing front- or backwards? */
    long next;			/* index of next polygon in z-sorted list */
#if TEST_GRIDBOX
    unsigned long xbits;	/* x coverage mask of bounding box */
    unsigned long ybits;	/* y coverage mask of bounding box */
#endif
} polygon;
typedef polygon GPHUGE *p_polygon;

/* Enumeration of possible types of line, for use with the
 * store_edge() function. Influences the position in the grid the
 * second vertex will be put to, relative to the one that is passed
 * in, as another argument to that function. edir_none is for
 * single-pixel 'blind' edges, which exist only to facilitate output
 * of 'points' style splots.
 *
 * Directions are interpreted in a pseudo-geographical coordinate
 * system of the data grid: within the isoline, we count from left to
 * right (west to east), and the isolines themselves are counted from
 * top to bottom, described as north and south. */
typedef enum edge_direction {
    edir_west, edir_north, edir_NW, edir_NE, edir_impulse, edir_point
} edge_direction;

/* direction into which the polygon is facing (the corner with the
 * right angle, inside the mesh, that is). The reference identifiying
 * the whole cell is always the lower right, i.e. southeast one. */
typedef enum polygon_direction {
    pdir_NE, pdir_SE, pdir_SW, pdir_NW
} polygon_direction;

/* Three dynamical arrays that describe what we have to plot: */
static dynarray vertices, edges, polygons;

/* convenience #defines to make the generic vector useable as typed arrays */
#define vlist ((p_vertex) vertices.v)
#define plist ((p_polygon) polygons.v)
#define elist ((p_edge) edges.v)

static long pfirst;		/* first polygon in zsorted chain */
static long efirst;		/* first edges in zsorted chain */

/* macros for (somewhat) safer handling of the dynamic array of
 * polygons, 'plist[]' */
#define EXTEND_PLIST() extend_dynarray(&polygons, 100)

#define SETBIT(a,i,set) a= (set) ?  (a) | (1L<<(i)) : (a) & ~(1L<<(i))
#define GETBIT(a,i) (((a)>>(i))&1L)

/* Prototypes for internal functions of this module. */
static long int store_vertex __PROTO((struct coordinate GPHUGE * point,
				      int pointtype));
static long int make_edge __PROTO((long int vnum1, long int vnum2,
				   struct lp_style_type * lp,
				   int style, int next));
static long int store_edge __PROTO((long int vnum1, edge_direction direction,
				    long int crvlen, struct lp_style_type * lp,
				    int style));
static GP_INLINE double eval_plane_equation __PROTO((t_plane p, p_vertex v));
static long int store_polygon __PROTO((long int vnum1,
				       polygon_direction direction,
				       long int crvlen));
static void color_edges __PROTO((long int new_edge, long int old_edge,
				 long int new_poly, long int old_poly,
				 int style_above, int style_below));
static void build_networks __PROTO((struct surface_points * plots,
				    int pcount));
static int compare_edges_by_zmin __PROTO((const void *p1, const void *p2));
static int compare_polys_by_zmax __PROTO((const void *p1, const void *p2));
static void sort_edges_by_z __PROTO((void));
static void sort_polys_by_z __PROTO((void));
static TBOOLEAN get_plane __PROTO((p_polygon p, t_plane plane));
static long split_line_at_ratio __PROTO((p_edge e, double w));
static GP_INLINE double area2D __PROTO((p_vertex v1, p_vertex v2,
					p_vertex v3));
static void draw_edge __PROTO((p_edge e));
static TBOOLEAN in_front __PROTO((long int edgenum, long int *firstpoly));


/* Set the options for hidden3d. To be called from set.c, when the
 * user has begun a command with 'set hidden3d', to parse the rest of
 * that command */
void
set_hidden3doptions()
{
    struct value t;
    int tmp;

    while (!END_OF_COMMAND) {
	switch (lookup_table(&set_hidden3d_tbl[0], c_token)) {
	case S_HI_DEFAULTS:
	    /* reset all parameters to defaults */
	    hiddenBacksideLinetypeOffset = BACKSIDE_LINETYPE_OFFSET;
	    hiddenTriangleLinesdrawnPattern = TRIANGLE_LINESDRAWN_PATTERN;
	    hiddenHandleUndefinedPoints = HANDLE_UNDEFINED_POINTS;
	    hiddenShowAlternativeDiagonal = SHOW_ALTERNATIVE_DIAGONAL;
	    hiddenHandleBentoverQuadrangles = HANDLE_BENTOVER_QUADRANGLES;
	    c_token++;
	    if (!END_OF_COMMAND)
		int_error(c_token,
			  "No further options allowed after 'defaults'");
	    return;
	    break;
	case S_HI_OFFSET:
	    c_token++;
	    hiddenBacksideLinetypeOffset = (int) real(const_express(&t));
	    c_token--;
	    break;
	case S_HI_NOOFFSET:
	    hiddenBacksideLinetypeOffset = 0;
	    break;
	case S_HI_TRIANGLEPATTERN:
	    c_token++;
	    hiddenTriangleLinesdrawnPattern = (int) real(const_express(&t));
	    c_token--;
	    break;
	case S_HI_UNDEFINED:
	    c_token++;
	    tmp = (int) real(const_express(&t));
	    if (tmp <= 0 || tmp > UNHANDLED)
		tmp = UNHANDLED;
	    hiddenHandleUndefinedPoints = tmp;
	    c_token--;
	    break;
	case S_HI_NOUNDEFINED:
	    hiddenHandleUndefinedPoints = UNHANDLED;
	    break;
	case S_HI_ALTDIAGONAL:
	    hiddenShowAlternativeDiagonal = 1;
	    break;
	case S_HI_NOALTDIAGONAL:
	    hiddenShowAlternativeDiagonal = 0;
	    break;
	case S_HI_BENTOVER:
	    hiddenHandleBentoverQuadrangles = 1;
	    break;
	case S_HI_NOBENTOVER:
	    hiddenHandleBentoverQuadrangles = 0;
	    break;
	case S_HI_INVALID:
	    int_error(c_token, "No such option to hidden3d (or wrong order)");
	default:
	    break;
	}
	c_token++;
    }
}

void
show_hidden3doptions()
{
    fprintf(stderr, "\
\t  Back side of surfaces has linestyle offset of %d\n\
\t  Bit-Mask of Lines to draw in each triangle is %ld\n\
\t  %d: ",
	    hiddenBacksideLinetypeOffset, hiddenTriangleLinesdrawnPattern,
	    hiddenHandleUndefinedPoints);

    switch (hiddenHandleUndefinedPoints) {
    case OUTRANGE:
	fputs("Outranged and undefined datapoints are omitted from the surface.\n", stderr);
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

    fprintf(stderr, "\
\t  Will %suse other diagonal if it gives a less jaggy outline\n\
\t  Will %sdraw diagonal visibly if quadrangle is 'bent over'\n",
	    hiddenShowAlternativeDiagonal ? "" : "not ",
	    hiddenHandleBentoverQuadrangles ? "" : "not ");
}

/* Implements proper 'save'ing of the new hidden3d options... */
void
save_hidden3doptions(fp)
FILE *fp;
{
    if (!hidden3d) {
	fputs("unset hidden3d\n", fp);
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
init_hidden_line_removal()
{
    /* Check for some necessary conditions to be set elsewhere: */
    /* HandleUndefinedPoints mechanism depends on these: */
    assert(OUTRANGE == 1);
    assert(UNDEFINED == 2);

    /* '3' makes the test easier in the critical section */
    if (hiddenHandleUndefinedPoints < OUTRANGE)
	hiddenHandleUndefinedPoints = UNHANDLED;

    init_dynarray(&vertices, sizeof(vertex), 100, 100);
    init_dynarray(&edges, sizeof(edge), 100, 100);
    init_dynarray(&polygons, sizeof(polygon), 100, 100);

}

/* Reset the hidden line data to a fresh start. */
void
reset_hidden_line_removal()
{
    vertices.end = 0;
    edges.end = 0;
    polygons.end = 0;
}


/* Terminates the hidden line removal process.                  */
/* Free any memory allocated by init_hidden_line_removal above. */
void
term_hidden_line_removal()
{
    free_dynarray(&polygons);
    free_dynarray(&edges);
    free_dynarray(&vertices);
}


/* Maps from normalized space to terminal coordinates */
#define TERMCOORD(v,xvar,yvar) \
    xvar = ((int)((v)->x * xscaler)) + xmiddle, \
		yvar = ((int)((v)->y * yscaler)) + ymiddle;

/* These macros to map from user 3D space into normalized -1..1 */
/* FIXME: duplicated from graph3d.c, should be centralized, or removed */
#define map_x3d(x) ((x-min_array[FIRST_X_AXIS])*xscale3d-1.0)
#define map_y3d(y) ((y-min_array[FIRST_Y_AXIS])*yscale3d-1.0)
#define map_z3d(z) ((z-floor_z)*zscale3d-1.0)

/* Performs transformation from 'user coordinates' to a normalized
 * vector in 'graph coordinates' (-1..1 in all three directions) */
void
map3d_xyz(x, y, z, v)
double x, y, z;			/* user coordinates */
p_vertex v;			/* the point in normalized space */
{
    int i, j;
    double V[4], Res[4];	/* Homogeneous coords. vectors. */

    /* Normalize object space to -1..1 */
    V[0] = map_x3d(x);
    V[1] = map_y3d(y);
    V[2] = map_z3d(z);
    V[3] = 1.0;

    /* Res[] = V[] * trans_mat[][] (uses row-vectors) */
    for (i = 0; i < 4; i++) {
	Res[i] = trans_mat[3][i];	/* V[3] is 1. anyway */
	for (j = 0; j < 3; j++)
	    Res[i] += V[j] * trans_mat[j][i];
    }

    if (Res[3] == 0)
	Res[3] = 1.0e-5;

    v->x = Res[0] / Res[3];
    v->y = Res[1] / Res[3];
    v->z = Res[2] / Res[3];
#ifdef PM3D
    /* store z for later color calculation */
    v->real_z = z;
#endif
}


/* Gets Minimum 'var' value of polygon 'poly' into variable 'min. C is
 * one of x, y, or z: */
#define GET_MIN(poly, var, min) \
 do { \
  int i; \
  long *v = poly->vertex; \
  \
  min = vlist[*v++].var; \
  for (i = 1; i< poly->n; i++, v++) \
   if (vlist[*v].var < min) \
    min = vlist[*v].var; \
 } while (0)

/* Gets Maximum 'var' value of polygon 'poly', as with GET_MIN */
#define GET_MAX(poly, var, max) \
 do { \
  int i; \
  long *v = poly->vertex; \
  \
  max = vlist[*v++].var; \
  for (i = 1; i< poly->n; i++, v++) \
   if (vlist[*v].var > max) \
    max = vlist[*v].var; \
 } while (0)

/* get the amount of curves in a plot */
#define GETNCRV(NCRV) \
 do { \
  if (this_plot->plot_type == FUNC3D) \
   for (icrvs = this_plot->iso_crvs,NCRV = 0; \
        icrvs;icrvs = icrvs->next,NCRV++) \
    ; \
  else if(this_plot->plot_type == DATA3D) \
   NCRV = this_plot->num_iso_read; \
  else { \
   graph_error("Plot type is neither function nor data"); \
   return; \
  } \
 } while (0)

/* Do we see the top or bottom of the polygon, or is it 'on edge'? */
#define GET_SIDE(vlst,csign) \
 do { \
  double ctmp = \
   vlist[vlst[0]].x * (vlist[vlst[1]].y - vlist[vlst[2]].y) + \
   vlist[vlst[1]].x * (vlist[vlst[2]].y - vlist[vlst[0]].y) + \
   vlist[vlst[2]].x * (vlist[vlst[0]].y - vlist[vlst[1]].y); \
  csign = SIGN (ctmp); \
 } while (0)

static long int
store_vertex(point, pointtype)
struct coordinate GPHUGE *point;
int pointtype;
{
    p_vertex thisvert = nextfrom_dynarray(&vertices);

    thisvert->style = pointtype;
    if ((int) point->type >= hiddenHandleUndefinedPoints) {
	FLAG_VERTEX_AS_UNDEFINED(*thisvert);
	return (-1);
    }
    map3d_xyz(point->x, point->y, point->z, thisvert);
    return (thisvert - vlist);
}

/* A part of store_edge that does the actual storing. Used by
 * in_front(), as well, so I separated it out. */
static long int 
make_edge(vnum1, vnum2, lp, style, next)
long int vnum1, vnum2;
struct lp_style_type *lp;
int style;
int next;
{
    p_edge thisedge = nextfrom_dynarray(&edges);
    p_vertex v1 = vlist + vnum1;
    p_vertex v2 = vlist + vnum2;

    /* ensure z ordering inside each edge */
    if (v1->z >= v2->z) {
	thisedge->v1 = vnum1;
	thisedge->v2 = vnum2;
    } else {
	thisedge->v1 = vnum2;
	thisedge->v2 = vnum1;
    }

    thisedge->style = style;
    thisedge->lp = lp;
    thisedge->next = next;

    return thisedge - elist;
}

/* store the edge from vnum1 to vnum2 into the edge list. Ensure that
 * the vertex with higher z is stored in v1, to ease sorting by zmax */
static long int
store_edge(vnum1, direction, crvlen, lp, style)
long int vnum1;
edge_direction direction;
long int crvlen;
struct lp_style_type *lp;
int style;
{
    p_vertex v1 = vlist + vnum1;
    p_vertex v2 = NULL;		/* just in case: initialize... */
    long int vnum2;
    unsigned int drawbits = (0x1 << direction);

    switch (direction) {
    case edir_west:
	v2 = v1 - 1;
	break;
    case edir_north:
	v2 = v1 - crvlen;
	break;
    case edir_NW:
	v2 = v1 - crvlen - 1;
	break;
    case edir_NE:
	v2 = v1 - crvlen;
	v1 -= 1;
	drawbits >>= 1;		/* altDiag is handled like normal NW one */
	break;
    case edir_impulse:
	v2 = v1 - 1;
	drawbits = 0;		/* don't care about the triangle pattern */
	break;
    case edir_point:
	v2 = v1;
	drawbits = 0;		/* nothing to draw, but disable check */
	break;
    }

    vnum2 = v2 - vlist;

    if (VERTEX_IS_UNDEFINED(*v1)
	|| VERTEX_IS_UNDEFINED(*v2)) {
	return -2;
    }
    if (drawbits &&		/* no bits set: 'blind' edge --> no test! */
	!(hiddenTriangleLinesdrawnPattern & drawbits)
	)
	style = -3;

    return make_edge(vnum1, vnum2, lp, style, -1);
}


/* Calculate the normal equation coefficients of the plane of polygon
 * 'p'. Uses is the 'signed projected area' method. Its benefit is
 * that it doesn't rely on only three of the vertices of 'p', as the
 * naive cross product method does. */
static TBOOLEAN
get_plane(poly, plane)
p_polygon poly;
t_plane plane;
{
    int i;
    p_vertex v1, v2;
    double x, y, z, s;
    TBOOLEAN frontfacing = 1;

    /* calculate the signed areas of the polygon projected onto the
     * planes x=0, y=0 and z=0, respectively. The three areas form
     * the components of the plane's normal vector: */
    v1 = vlist + poly->vertex[poly->n - 1];
    v2 = vlist + poly->vertex[0];
    plane[0] = (v1->y - v2->y) * (v1->z + v2->z);
    plane[1] = (v1->z - v2->z) * (v1->x + v2->x);
    plane[2] = (v1->x - v2->x) * (v1->y + v2->y);
    for (i = 1; i < poly->n; i++) {
	v1 = v2;
	v2 = vlist + poly->vertex[i];
	plane[0] += (v1->y - v2->y) * (v1->z + v2->z);
	plane[1] += (v1->z - v2->z) * (v1->x + v2->x);
	plane[2] += (v1->x - v2->x) * (v1->y + v2->y);
    }

    /* Normalize the resulting normal vector */
    s = sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);

    if (GE(0.0, s)) {
	/* The normal vanishes, i.e. the polygon is degenerate. We build
	 * another vector that is orthogonal to the line of the polygon */
	v1 = vlist + poly->vertex[0];
	for (i = 1; i < poly->n; i++) {
	    v2 = vlist + poly->vertex[i];
	    if (!V_EQUAL(v1, v2))
		break;
	}

	/* build (x,y,z) that should be linear-independant from <v1, v2> */
	x = v1->x;
	y = v1->y;
	z = v1->z;
	if (EQ(y, v2->y))
	    y += 1.0;
	else
	    x += 1.0;

	/* Re-do the signed area computations */
	plane[0] = v1->y * (v2->z - z) + v2->y * (z - v1->z) + y * (v1->z - v2->z);
	plane[1] = v1->z * (v2->x - x) + v2->z * (x - v1->x) + z * (v1->x - v2->x);
	plane[2] = v1->x * (v2->y - y) + v2->x * (y - v1->y) + x * (v1->y - v2->y);
	s = sqrt(plane[0] * plane[0] + plane[1] * plane[1] + plane[2] * plane[2]);
    }
    /* ensure that normalized c is > 0 */
    if (plane[2] < 0.0) {
	s *= -1.0;
	frontfacing = 0;
    }
    plane[0] /= s;
    plane[1] /= s;
    plane[2] /= s;

    /* Now we have the normalized normal vector, insert one of the
     * vertices into the equation to get 'd'. For an even better result,
     * an average over all the vertices might be used */
    plane[3] = -plane[0] * v1->x - plane[1] * v1->y - plane[2] * v1->z;

    return frontfacing;
}


/* Evaluate the plane equation represented a four-vector for the given
 * vector. For points in the plane, this should result in values ==0.
 * < 0 is 'away' from the polygon, > 0 is infront of it */
static GP_INLINE double
eval_plane_equation(p, v)
t_plane p;
p_vertex v;
{
    return (p[0] * v->x + p[1] * v->y + p[2] * v->z + p[3]);
}


/* Build the data structure for this polygon. The return value is the
 * index of the newly generated polygon. This is memorized for access
 * to polygons in the previous isoline, from the next-following
 * one. */
static long int
store_polygon(vnum1, direction, crvlen)
long int vnum1;
polygon_direction direction;
long int crvlen;
{
    long int v[3];
    p_vertex v1, v2, v3;
    p_polygon p;

    switch (direction) {
    case pdir_NE:
	v[0] = vnum1;
	v[2] = vnum1 - crvlen;
	v[1] = v[2] - 1;
	break;
    case pdir_SW:
	/* triangle points southwest, here */
	v[0] = vnum1;
	v[1] = vnum1 - 1;
	v[2] = v[1] - crvlen;
	break;
    case pdir_SE:
	/* alt-diagonal, case 1: southeast triangle: */
	v[0] = vnum1;
	v[2] = vnum1 - crvlen;
	v[1] = vnum1 - 1;
	break;
    case pdir_NW:
	v[2] = vnum1 - crvlen;
	v[0] = vnum1 - 1;
	v[1] = v[0] - crvlen;
	break;
    }

    v1 = vlist + v[0];
    v2 = vlist + v[1];
    v3 = vlist + v[2];

    if (VERTEX_IS_UNDEFINED(*v1)
	|| VERTEX_IS_UNDEFINED(*v2)
	|| VERTEX_IS_UNDEFINED(*v3)
	)
	return (-2);

    /* All else OK, fill in the polygon: */

    p = nextfrom_dynarray(&polygons);

    p->n = 3;			/* FIXME: shouldn't be needed */
    memcpy(p->vertex, v, sizeof(v));
    p->next = -1;
    GET_MIN(p, x, p->xmin);
    GET_MIN(p, y, p->ymin);
    GET_MIN(p, z, p->zmin);
    GET_MAX(p, x, p->xmax);
    GET_MAX(p, y, p->ymax);
    GET_MAX(p, z, p->zmax);

#if TEST_GRIDBOX
# define UINT_BITS (CHAR_BIT * sizeof(unsigned int))
# define COORD_TO_BITMASK(x,shift) \
  (~0U << (unsigned int) (((x) + 1.0) / 2.0 * UINT_BITS + (shift)))

    p->xbits = (~COORD_TO_BITMASK(p->xmax, 1)) & COORD_TO_BITMASK(p->xmin, 0);
    p->ybits = (~COORD_TO_BITMASK(p->ymax, 1)) & COORD_TO_BITMASK(p->ymin, 0);
#endif

    p->frontfacing = get_plane(p, p->plane);

    return (p - plist);
}

/* color edges, based on the orientation of polygon(s). One of the two
 * edges passed in is a new one, meaning there is no other polygon
 * sharing it, yet. The other, 'old' edge is common to the new polygon
 * and another one, which was created earlier on. If these two polygon
 * differ in their orientation (one front-, the other backsided to the
 * viewer), this routine has to resolve that conflict.  Edge colours
 * are changed only if the edge wasn't invisible, before */
/* FIXME: I don't know how to do that conflict resolution, yet. For
 * now all such edges are treated as 'frontfacing' */
static void
color_edges(new_edge, old_edge, new_poly, old_poly, above, below)
long int new_edge;		/* index of 'new', conflictless edge */
long int old_edge;		/* index of 'old' edge, may conflict */
long int new_poly;		/* index of current polygon */
long int old_poly;		/* index of poly sharing old_edge */
int above;			/* style number for front of polygons */
int below;			/* style number for backside of polys */
{
    static int cc0 = 0, cc1 = 0, cc2 = 0, cc3 = 0;	/* case counting variables...  */
    int casenumber;

    if (new_poly > -2) {
	/* new polygon was built successfully */
	if (old_poly <= -2)
	    /* old polygon doesn't exist. Use new_polygon for both: */
	    old_poly = new_poly;

	casenumber =
	    (plist[new_poly].frontfacing != 0)
	    + 2 * (plist[old_poly].frontfacing != 0);
	switch (casenumber) {
	case 0:
	    /* both backfacing */
	    cc0++;
	    if (elist[new_edge].style >= -2)
		elist[new_edge].style = below;
	    if (elist[old_edge].style >= -2)
		elist[old_edge].style = below;
	    break;
	case 2:
	    if (elist[new_edge].style >= -2)
		elist[new_edge].style = below;
	    /* FALLTHROUGH */
	case 1:
	    /* new front-, old one backfacing, or */
	    /* new back-, old one frontfacing */
	    cc1++;
	    if (((new_edge == old_edge)
		 && hiddenHandleBentoverQuadrangles)	/* a diagonal edge! */
		||(elist[old_edge].style >= -2)) {
		/* conflict has occured: two polygons meet here, with opposige
		 * sides being shown. What's to do?
		 * 1) find a vertex of one polygon outside this common edge
		 * 2) check wether it's in front of or behind the other
		 *    polygon's plane
		 * 3) if in front, color the edge accoring to the vertex'
		 *    polygon, otherwise, color like the other polygon */
		long int vnum1 = elist[old_edge].v1;
		long int vnum2 = elist[old_edge].v2;
		p_polygon p = plist + new_poly;
		long int pvert = -1;
		double point_to_plane;

		if (p->vertex[0] == vnum1) {
		    if (p->vertex[1] == vnum2) {
			pvert = p->vertex[2];
		    } else if (p->vertex[2] == vnum2) {
			pvert = p->vertex[1];
		    }
		} else if (p->vertex[1] == vnum1) {
		    if (p->vertex[0] == vnum2) {
			pvert = p->vertex[2];
		    } else if (p->vertex[2] == vnum2) {
			pvert = p->vertex[0];
		    }
		} else if (p->vertex[2] == vnum1) {
		    if (p->vertex[0] == vnum2) {
			pvert = p->vertex[1];
		    } else if (p->vertex[1] == vnum2) {
			pvert = p->vertex[0];
		    }
		}
		assert(pvert >= 0);

		point_to_plane =
		    eval_plane_equation(plist[old_poly].plane, vlist + pvert);

		if (point_to_plane > 0) {
		    /* point in new_poly is in front of old_poly plane */
		    elist[old_edge].style = p->frontfacing ? above : below;
		} else {
		    elist[old_edge].style =
			plist[old_poly].frontfacing ? above : below;
		}
	    }
	    break;
	case 3:
	    /* both frontfacing: nothing to do */
	    cc2++;
	    break;
	}			/* switch */
    } else {
	/* Ooops? build_networks() must have guessed incorrectly that this
	 * polygon should exist. */
	cc3++;
    }
}


/* This somewhat monstrous routine fills the vlist, elist and plist
 * dynamic arrays with values from all those plots. It strives to
 * respect all the topological linkage between vertices, edges and
 * polygons. E.g., it has to find the correct color for each edge,
 * based on the orientation of the two polygons sharing it, WRT both
 * the observer and each other. */
static void
build_networks(plots, pcount)
struct surface_points *plots;
int pcount;
{
    long int i;
    struct surface_points *this_plot;
    int surface;		/* count the surfaces (i.e. sub-plots) */
    long int crv, ncrvs;	/* count isolines */
    long int max_crvlen;	/* maximal length of an isoline, for all
				   * plots */
    long int nv, ne, np;	/* local poly/edge/vertex counts */
    long int *north_polygons;	/* stores polygons of isoline above */
    long int *these_polygons;	/* same, being built for use by next turn */
    long int *north_edges;	/* stores edges of polyline above */
    long int *these_edges;	/* same, being built for use by next turn */
    struct iso_curve *icrvs;
    int above = -3, below;	/* line type for edges of front/back side */
    struct lp_style_type *lp;	/* pointer to line and point properties */

    /* Count out the initial sizes needed for the polygon and vertex
     * lists. */
    nv = ne = np = 0;
    max_crvlen = -1;

    for (this_plot = plots, surface = 0;
	 surface < pcount;
	 this_plot = this_plot->next_sp, surface++) {
	long int crvlen = this_plot->iso_crvs->p_count;

	if (crvlen > max_crvlen)	/* register maximal isocurve length */
	    max_crvlen = crvlen;

	/* count 'curves' (i.e. isolines) in this plot */
	GETNCRV(ncrvs);

	/* To avoid possibly suprising error messages, several 2d-only
	 * plot styles are mapped to others, that are genuinely available
	 * in 3d. */
	switch (this_plot->plot_style) {
	case LINESPOINTS:
	case STEPS:
	case FSTEPS:
	case HISTEPS:
	case LINES:
	    nv += ncrvs * crvlen;
	    ne += 3 * ncrvs * crvlen - 2 * ncrvs - 2 * crvlen + 1;
	    np += 2 * (ncrvs - 1) * (crvlen - 1);
	    break;
	case BOXES:
	case IMPULSES:
	    nv += 2 * ncrvs * crvlen;
	    ne += ncrvs * crvlen;
	    break;
	case POINTSTYLE:
	default:		/* treat all remaining ones like 'points' */
	    nv += ncrvs * crvlen;
	    ne += ncrvs * crvlen;	/* a 'phantom edge' per isolated point (???) */
	    break;
	}			/* switch */
    }				/* for (plots) */

    /* allocate all the lists to the size we need: */
    resize_dynarray(&vertices, nv);
    resize_dynarray(&edges, ne);
    resize_dynarray(&polygons, np);

    /* allocate the storage for polygons and edges of the isoline just above
     * the current one, to allow easy access to them from the current isoline
     */
    north_polygons = gp_alloc(2 * max_crvlen * sizeof(long int),
			      "hidden nort_polys");
    these_polygons = gp_alloc(2 * max_crvlen * sizeof(long int),
			      "hidden these_polys");
    north_edges = gp_alloc(3 * max_crvlen * sizeof(long int),
			   "hidden nort_edges");
    these_edges = gp_alloc(3 * max_crvlen * sizeof(long int),
			   "hidden these_edges");

    /* initialize the lists, all in one large loop. This is different from
     * the previous approach, which went over the vertices, first, and only
     * then, in new loop, built polygons */
    for (this_plot = plots, surface = 0;
	 surface < pcount;
	 this_plot = this_plot->next_sp, surface++) {
	long int crvlen = this_plot->iso_crvs->p_count;
	int pointtype = -1;

	lp = &(this_plot->lp_properties);
	above = this_plot->lp_properties.l_type;
	below = this_plot->lp_properties.l_type + hiddenBacksideLinetypeOffset;

	/* calculate the point symbol type: */
	if (this_plot->plot_style & 1) {
	    /* Plot style contains point symbols */
	    /* FIXME: would be easier if the p_style field of lp_properties
	     * were always filled with a usable value (i.e.: -1 if nothing
	     * to draw) It may also be clearer to move this down, into the
	     * other switch structure.  */
	    pointtype = this_plot->lp_properties.p_type;
	}
	GETNCRV(ncrvs);

	/* initialize stored indices of north-of-this-isoline polygons and
	 * edges properly */
	for (i = 0; i < this_plot->iso_crvs->p_count; i++) {
	    north_polygons[2 * i] = north_polygons[2 * i + 1]
		= north_edges[3 * i] = north_edges[3 * i + 1]
		= north_edges[3 * i + 2]
		= -3;
	}

	for (crv = 0, icrvs = this_plot->iso_crvs;
	     crv < ncrvs && icrvs;
	     crv++, icrvs = icrvs->next) {
	    struct coordinate GPHUGE *points = icrvs->points;

	    for (i = 0; i < icrvs->p_count; i++) {
		long int thisvertex, basevertex;
		long int e1, e2, e3;
		long int pnum;

		thisvertex = store_vertex(points + i, pointtype);

		/* Preset the pointers to the polygons and edges belonging to
		 * this isoline */
		these_polygons[2 * i] = these_polygons[2 * i + 1]
		    = these_edges[3 * i] = these_edges[3 * i + 1]
		    = these_edges[3 * i + 2]
		    = -3;

		switch (this_plot->plot_style) {
		case LINESPOINTS:
		case STEPS:
		case FSTEPS:
		case HISTEPS:
		case LINES:
		    if (i > 0) {
			/* not first point, so we might want to set up the
			 * edge(s) to the left of this vertex */
			if (thisvertex < 0) {
			    if ((crv > 0)
				&& (hiddenShowAlternativeDiagonal)
				) {
				/* this vertex is invalid, but the other three
				 * might still form a valid triangle, facing
				 * northwest to do that, we'll need the
				 * 'wrong' diagonal, which goes from
				 * SW to NE: */
				these_edges[i * 3 + 2] = e3
				    = store_edge(vertices.end - 1, edir_NE,
						 crvlen, lp, above);
				if (e3 > -2) {
				    /* don't store this polygon for later: it
				     * doesn't share edges with any others to
				     * the south or east, so there's need to */
				    pnum = store_polygon(vertices.end - 1,
							 pdir_NW, crvlen);
				    /* The other two edges of this polygon
				     * need to be checked against the
				     * neighboring polygons'
				     * orientations, before being coloured */
				    color_edges(e3, these_edges[3 * (i - 1) + 1],
						pnum,
						these_polygons[2 * (i - 1) + 1],
						above, below);
				    color_edges(e3, north_edges[3 * i],
						pnum,
						north_polygons[2 * i],
						above, below);
				}
			    }
			    break;	/* nothing else to do for invalid vertex */
			}
			/* Coming here means that the current vertex is valid:
			 * check the other three of this cell, by trying to
			 * set up the edges from this one to there */
			these_edges[i * 3] = e1
			    = store_edge(thisvertex, edir_west, crvlen, lp,
					 above);

			if (crv > 0) {	/* vertices to the north exist */
			    these_edges[i * 3 + 1] = e2
				= store_edge(thisvertex, edir_north, crvlen,
					     lp, above);
			    these_edges[i * 3 + 2] = e3
				= store_edge(thisvertex, edir_NW, crvlen,
					     lp, above);
			    if (e3 > -2) {
				/* diagonal edge of this cell is OK, so try to
				 * build both the polygons: */
				if (e1 > -2) {
				    /* one pair of edges is valid: put first
				     * polygon, which points towards the
				     * southwest */
				    these_polygons[2 * i] = pnum
					= store_polygon(thisvertex, pdir_SW,
							crvlen);
				    color_edges(e1, these_edges[3 * (i - 1) + 1],
						pnum,
						these_polygons[2 * (i - 1) + 1],
						above, below);
				}
				if (e2 > -2) {
				    /* other pair of two is fine, put the
				     * northeast polygon: */
				    these_polygons[2 * i + 1] = pnum
					= store_polygon(thisvertex, pdir_NE,
							crvlen);
				    color_edges(e2, north_edges[3 * i],
						pnum, north_polygons[2 * i],
						above, below);
				}
				/* In case these two new polygons differ in
				 * orientation, find good coloring of the
				 * diagonal */
				color_edges(e3, e3, these_polygons[2 * i],
					    these_polygons[2 * i + 1], above,
					    below);
			    }
			    /* if e3 valid */ 
			    else if ((e1 > -2) && (e2 > -2)
				     && hiddenShowAlternativeDiagonal) {
				/* looks like all but the north-west vertex are
				 * usable, so we set up the southeast-pointing
				 * triangle, using the 'wrong' diagonal: */
				these_edges[3 * i + 2] = e3
				    = store_edge(thisvertex, edir_NE, crvlen,
						 lp, above);
				if (e3 > -2) {
				    /* fill this polygon into *both* polygon
				     * places for this quadrangle, as this
				     * triangle coincides with both edges that
				     * will be used by later polygons */
				    these_polygons[2 * i] =
					these_polygons[2 * i + 1] = pnum =
					store_polygon(thisvertex, pdir_SE,
						      crvlen);
				    /* This case is somewhat special: all
				     * edges are new, so there is no other
				     * polygon orientation to consider */
				    if (!plist[pnum].frontfacing)
					elist[e1].style = elist[e2].style =
					    elist[e3].style = below;
				}
			    }
			}
		    } else if ((crv > 0)
			       && (thisvertex >= 0)) {
			/* We're at the west border of the grid, but not on the
			 * north one: put vertical end-wall edge:*/
			these_edges[3 * i + 1] =
			    store_edge(thisvertex, edir_north, crvlen, lp,
				       above);
		    }
		    break;

		case BOXES:
		case IMPULSES:
		    if (thisvertex < 0)
			break;

		    /* set second vertex to the low end of zrange */
		    {
			coordval remember_z = points[i].z;

			points[i].z = min_array[FIRST_Z_AXIS];
			basevertex = store_vertex(points + i, pointtype);
			points[i].z = remember_z;
		    }
		    if (basevertex > 0)
			store_edge(thisvertex, edir_impulse, crvlen, lp, above);
		    break;

		case POINTSTYLE:
		default:	/* treat all the others like 'points' */
		    store_edge(thisvertex, edir_point, crvlen, lp, above);
		    break;
		}		/* switch */
	    }			/* for(i) */

	    /* Swap the 'north' lists of polygons and edges with 'these' ones,
	     * which have been filled in the pass through this isocurve */
	    {
		long int *temp = north_polygons;
		north_polygons = these_polygons;
		these_polygons = temp;

		temp = north_edges;
		north_edges = these_edges;
		these_edges = temp;
	    }
	}			/* for(isocrv) */
    }				/* for(plot) */
    free(these_polygons);
    free(north_polygons);
    free(these_edges);
    free(north_edges);
}

/* Sort the elist in order of growing zmax. Uses qsort on an array of
 * plist indices, and then fills in the 'next' fields in struct
 * polygon to store the resulting order inside the plist */
static int
compare_edges_by_zmin(p1, p2)
const void *p1, *p2;
{
    return SIGN(vlist[elist[*(const long *) p1].v2].z
		- vlist[elist[*(const long *) p2].v2].z);
}

static void
sort_edges_by_z()
{
    long *sortarray, i;
    p_edge this;

    if (!edges.end)
	return;

    sortarray = (long *) gp_alloc(sizeof(long) * edges.end,
				  "hidden sort edges");
    /* initialize sortarray with an identity mapping */
    for (i = 0; i < edges.end; i++)
	sortarray[i] = i;
    /* sort it */
    qsort(sortarray, (size_t) edges.end, sizeof(long), compare_edges_by_zmin);

    /* traverse plist in the order given by sortarray, and set the
     * 'next' pointers */
    this = elist + sortarray[0];
    for (i = 1; i < edges.end; i++) {
	this->next = sortarray[i];
	this = elist + sortarray[i];
    }
    this->next = -1L;

    /* 'efirst' is the index of the leading element of plist */
    efirst = sortarray[0];

    free(sortarray);
}

static int
compare_polys_by_zmax(p1, p2)
const void *p1, *p2;
{
    return (SIGN(plist[*(const long *) p1].zmax
		 - plist[*(const long *) p2].zmax));
}

static void
sort_polys_by_z()
{
    long *sortarray, i;
    p_polygon this;

    if (!polygons.end)
	return;

    sortarray = (long *) gp_alloc(sizeof(long) * polygons.end,
				  "hidden sortarray");

    /* initialize sortarray with an identity mapping */
    for (i = 0; i < polygons.end; i++)
	sortarray[i] = i;
    /* sort it */
    qsort(sortarray, (size_t) polygons.end, sizeof(long), compare_polys_by_zmax);

    /* traverse plist in the order given by sortarray, and set the
     * 'next' pointers */
    this = plist + sortarray[0];
    for (i = 1; i < polygons.end; i++) {
	this->next = sortarray[i];
	this = plist + sortarray[i];
    }
    this->next = -1L;
    /* 'pfirst' is the index of the leading element of plist */
    pfirst = sortarray[0];
    free(sortarray);
}


#define LASTBITS (USHRT_BITS -1)	/* ????? */

/*************************************************************/
/*************************************************************/
/*******   The depth sort algorithm (in_front) and its  ******/
/*******   whole lot of helper functions                ******/
/*************************************************************/
/*************************************************************/

/* Split a given line segment into two at an inner point. The inner
 * point is specified as a fraction of the line-length (0 is V1, 1 is
 * V2) */
static long
split_line_at_ratio(e, w)
p_edge e;			/* pointer to line to be split */
double w;			/* where to split it */
{
    p_vertex v, v1, v2;

    if (EQ(w, 0.0))
	return e->v1;
    if (EQ(w, 1.0))
	return e->v2;

    /* Create a new vertex */
    v = nextfrom_dynarray(&vertices);

    v1 = vlist + e->v1;
    v2 = vlist + e->v2;

    v->x = (v2->x - v1->x) * w + v1->x;
    v->y = (v2->y - v1->y) * w + v1->y;
    v->z = (v2->z - v1->z) * w + v1->z;
    v->style = -1;

    /* additional checks to prevent adding unnecessary vertices */
    if (V_EQUAL(v, v1)) {
	droplast_dynarray(&vertices);
	return e->v1;
    }
    if (V_EQUAL(v, v2)) {
	droplast_dynarray(&vertices);
	return e->v2;
    }
    return (v - vlist);
}


/* Compute the 'signed area' of 3 points in their 2d projection
 * to the x-y plane. Essentially the z component of the crossproduct.
 * Should come out positive if v1, v2, v3 are ordered counter-clockwise */

static GP_INLINE double
area2D(v1, v2, v3)
p_vertex v1, v2, v3;		/* The vertices */
{
    register double
     dx12 = v2->x - v1->x,	/* x/y components of (v2-v1) and (v3-v1) */
     dx13 = v3->x - v1->x, dy12 = v2->y - v1->y, dy13 = v3->y - v1->y;
    return (dx12 * dy13 - dy12 * dx13);
}

/* Utility routing: takes an edge and makes a new one, which is a fragment
 * of the old one. The fragment inherits the line style and stuff of the
 * given edge, only the two new vertices are different */
static void handle_edge_fragment __PROTO((p_edge e,
					  long int vnum1,
					  long int vnum2,
					  long int firstpoly));

static void
handle_edge_fragment(e, vnum1, vnum2, firstpoly)
p_edge e;
long int vnum1, vnum2;
long int firstpoly;
{
    long int thisedge = make_edge(vnum1, vnum2, e->lp, e->style, -1);
    in_front(thisedge, &firstpoly);

    /* this fragment is handled, release it again */
    droplast_dynarray(&edges);
}


/*********************************************************************/
/* the actual heart of all this: determines is edge at index 'edgenum'
 * of the elist is in_front of all the polygons, or not */
/*********************************************************************/

static TBOOLEAN
in_front(edgenum, firstpoly)
long int edgenum;
long int *firstpoly;
{
    p_polygon p;
    long int polynum;
    long int vnum1, vnum2;
    p_edge e = elist + edgenum;
    p_vertex v1, v2;

    coordval xmin, xmax;	/* all of these are for the edge */
    coordval ymin, ymax;
    coordval zmin, zmax;
#if TEST_GRIDBOX
    unsigned int xextent;
    unsigned int yextent;
# define SET_XEXTENT xextent = \
                     (~COORD_TO_BITMASK(xmax,1)) & COORD_TO_BITMASK(xmin,0);
# define SET_YEXTENT yextent = \
                     (~COORD_TO_BITMASK(ymax,1)) & COORD_TO_BITMASK(ymin,0);
#else
# define SET_XEXTENT		/* nothing */
# define SET_YEXTENT		/* nothing */
#endif
    /* zmin of the edge, as it started out. This is needed separately to
     * allow modifying '*firstpoly', without moving it too far to the
     * front. */
    coordval first_zmin;

    /* macro to facilitate tail-recursion inside in_front: when the
     * current is modified, recompute all function-wide status
     * variables. */
#define setup_edge(vert1, vert2) \
 do { \
  vnum1 = vert1; \
  vnum2 = vert2; \
  v1 = vlist + (vert1); \
  v2 = vlist + (vert2); \
  \
  if (v1->z < v2->z) { \
   v1 = vlist + (vert1); \
   v2 = vlist + (vert2); \
  } \
  \
  zmax = v1->z; \
  zmin = v2->z; \
  \
  if (v1->x > v2->x) { \
   xmin = v2->x; \
   xmax = v1->x; \
  } else { \
   xmin = v1->x; \
   xmax = v2->x; \
  } \
  SET_XEXTENT; \
  \
  if (v1->y > v2->y) { \
   ymin = v2->y; \
   ymax = v1->y; \
  } else { \
   ymin = v1->y; \
   ymax = v2->y; \
  } \
  SET_YEXTENT; \
 }while (0)

    /* use the macro, for initial setup */
    setup_edge(e->v1, e->v2);

    first_zmin = zmin;

    /* loop over all the polygons in the sorted list, until they are
     * uninteresting because the z extent of the edge is spent */
    for (polynum = *firstpoly, p = plist + polynum;
	 polynum >= 0;
	 polynum = p->next, p = plist + polynum
	) {
	/* shortcut variables for the three vertices of 'p': */
	p_vertex w1, w2, w3;
	/* signed areas of each of e's vertices wrt. the edges of p. I
	 * store them explicitly, because the are used more than once,
	 * eventually */
	double e_side[2][3];
	/* signed areas of each of p's vertices wrt. to edge e. Stored
	 * for re-use in intersection calculations, as well */
	double p_side[3];
	/* values got from throwing the edge vertices into the plane
	 * equation of the polygon. Used for testing if the vertices are
	 * in front of or behind the plane, and also to determine the
	 * point where the edge goes through the polygon, by
	 * interpolation. */
	double v1_rel_pplane, v2_rel_pplane;
	/* Flags: are edge vertices found inside the triangle? */
	TBOOLEAN v1_inside_p, v2_inside_p;
	/* Orientation of polygon wrt. to the eye: front or back side
	 * visible? */
	coordval polyfacing;
	int whichside;
	/* stores classification of cases as 4 2-bit patterns, mainly */
	unsigned int classification[4];

	/* OK, off we go with the real work. This algo is mainly following
	 * the one of 'HLines.java', as described in the book 'Computer
	 * Graphics for Java Programmers', by Dutch professor Leen
	 * Ammeraal, published by J.Wiley & Sons, ISBN 0 471 98142 7. It
	 * happens to the only(!) actual code or algorithm example I got
	 * out of a question to the Newsgroup comp.graphics.algorithms,
	 * asking for a hidden *line* removal algorithm. */

	/* Test 1 (2D): minimax tests. Do x/y ranges of polygon and edge have
	 * any overlap? */
	if (0
#if TEST_GRIDBOX
	/* First, check by comparing the extent bit patterns: */
	    || (!(xextent & p->xbits))
	    || (!(yextent & p->ybits))
#endif
	    || (p->xmax < xmin)
	    || (p->xmin > xmax)
	    || (p->ymax < ymin)
	    || (p->ymin > ymax)
	    )
	    continue;

	/* Test 3 (3D): Is edge completely in front of polygon? */
	if (p->zmax < zmin) {
	    /* Polygon completely behind this edge. Move start of relevant
	     * plist to this point, to speed up next run. This makes use of the
	     * fact that elist is also kept in upwardly sorted order of zmin,
	     * i.e. the condition found here will also hold for all coming 
	     * edges in the list */
	    if (p->zmax < first_zmin)
		*firstpoly = polynum;	/* TESTEST: activated this, for now */
	    continue;		/* this polygon is done with */
	}
	/* Test 2 (0D): does edge belong to this very polygon? */
	if (1
	    && (0
		|| (p->vertex[0] == e->v1)
		|| (p->vertex[1] == e->v1)
		|| (p->vertex[2] == e->v1)
	    )
	    && (0
		|| (p->vertex[0] == e->v2)
		|| (p->vertex[1] == e->v2)
		|| (p->vertex[2] == e->v2)
	    )
	    )
	    continue;

	w1 = vlist + p->vertex[0];
	w2 = vlist + p->vertex[1];
	w3 = vlist + p->vertex[2];


	/* Test 4 (2D): Is edge fully on the 'wrong side' of one of the
	 * polygon edges?
	 *
	 * Done by comparing signed areas (cross-product z components of
	 * both edge endpoints) to that of the polygon itself (as given by
	 * the plane normal z component). If both v1 and v2 are found on
	 * the 'outside' side of a polygon edge (interpreting it as
	 * infinite line), the polygon can't obscure this edge. All the
	 * signed areas are remembered, for later tests. */
	polyfacing = p->plane[2];
	whichside = SIGN(polyfacing);
	if (!whichside)
	    /* We're looking at the border of this polygon. It looks like an
	     * infitisemally thin line, i.e. it'll be practically
	     * invisible*/
	    /* FIXME: should such a polygon have been stored in the plist,
	     * in the first place??? */
	    continue;

	/* p->plane[2] is now forced >=0, so it doesn't tell anything
	 * about which side the polygon is facing */
	whichside = (p->frontfacing) ? -1 : 1;

#if 0
	/* Just make sure I got this the right way round: */
	if (p->frontfacing)
	    assert(GE(area2D(w1, w2, w3), 0));
	else
	    assert(GE(0, area2D(w1, w2, w3)));
#endif

	/* classify both endpoints of the test edge, by their position
	 * relative to some plane that defines the volume shadowed by a
	 * triangle. */
#define classify(par1, par2, classnum) \
 do { \
  if (GR(par1, 0)) { \
   /* v1 strictly outside --> v2 in-plane is enough, already : */ \
   classification[classnum] = (1 << 0) | ((GE(par2, 0)) << 1); \
  } else if (GR(par2, 0)) { \
   /* v2 strictly outside --> v1 in-plane is enough, already : */ \
   classification[classnum] = ((GE(par1, 0)) << 0) | (1 << 1); \
  } else { \
   /* as neither one of them is strictly outside, there's no need \
    * for any edge-splitting, at all */ \
   classification[classnum] = 0; \
  } \
 } while (0)

	/* code block used thrice, so put into a macro. The 'area2D' calls
	 * return <0, ==0 or >0 if the three vertices passed to it are in
	 * clockwise order, colinear, or in conter-clockwise order,
	 * respectively. So the sign of this can be used to determine if a
	 * point is inside or outside the shadow volume. */
#define checkside(a,b,s) \
 e_side[0][s] = whichside * area2D((a), (b), v1); \
 e_side[1][s] = whichside * area2D((a), (b), v2); \
 classify(e_side[0][s], e_side[1][s], s); \
 if (classification[s] == 3) \
  continue;

	checkside(w1, w2, 0);
	checkside(w2, w3, 1);
	checkside(w3, w1, 2);
#undef checkside		/* get rid of that macro, again */


	/* Test 5 (2D): Does the whole polygon lie on one and the same
	 * side of the tested edge? Again, area2D is used to determine on
	 * which side of a given edge a vertex lies. */
	p_side[0] = area2D(v1, v2, w1);
	p_side[1] = area2D(v1, v2, w2);
	p_side[2] = area2D(v1, v2, w3);

	if (0
	    || (GE(p_side[0], 0) && GE(p_side[1], 0) && GE(p_side[2], 0))
	    || (GE(0, p_side[0]) && GE(0, p_side[1]) && GE(0, p_side[2]))
	    )
	    continue;


	/* Test 6 (3D): does the whole edge lie on the viewer's side of
	 * the polygon's plane? */
	v1_rel_pplane = eval_plane_equation(p->plane, v1);
	v2_rel_pplane = eval_plane_equation(p->plane, v2);

/*              printf("e%ld/p%ld: v_rel_pplane: %g/%g\n", */
/*                                       edgenum, polynum, v1_rel_pplane, v2_rel_pplane); */

	/* Condense into classification bit pattern for v1 and v2 wrt.
	 * this plane: */
	classify(v1_rel_pplane, v2_rel_pplane, 3);

	if (classification[3] == 3)
	    continue;		/* edge completely in front of polygon */

	/* Test 7 (2D): is edge completely inside the triangle? The stored
	 * results from Test 4 make this rather easy to check: if a point
	 * is oriented equally wrt. all three edges, and also to the
	 * triangle itself, the point is inside the 2D triangle: */

	/* search for any one bits (--> they mean 'outside') */
	v1_inside_p = ~(classification[0]
			| classification[1]
			| classification[2]);
	/* properly distribute the two bits to v2_in_p and v1_in_p: */
	v2_inside_p = (v1_inside_p >> 1) & 0x1;
	v1_inside_p = v1_inside_p & 0x1;

/*              printf("e%ld/p%ld: v_in_p: %d/%d\n", */
/*                                       edgenum, polynum, v1_inside_p, v2_inside_p); */

	if ((v1_inside_p) && (v2_inside_p)) {
	    /* Edge is completely inside the polygon's 2D projection. Unlike
	     * Ammeraal, I have to check for edges penetrating polygons, as
	     * well. */
/*                      printf("-->T7 says: edge found to be hidden\n"); */

	    /* check if edge really doesn't intersect the triangle (it
	     * isn't, in roughly one third of the cases. */
	    if (!classification[3])
		/* If both edges have been classified as 'behind', the result
		 * is true even in the case of possibly intersecting polygons
		 * */
		return 0;
#if 0
	    else
		printf("!!! this is *not* correct\n");
#endif
	}
	/* Test 8 (3D): If no edge intersects any polygon, the edge must
	 * lie infront of the poly if the 'inside' vertex in front of it.
	 * This test doesn't work if lines might intersect polygons, so I
	 * took it out of my version of Ammeraal's algorithm. */


	/* Test 9 (3D): The final 'catch-all' handler. We now know that
	 * the edges passes through the walls of the 'cylinder' stamped
	 * out by the polygon, somewhere, so we have to compute the
	 * intersection points and check whether they are infront of or
	 * behind the polygon, to isolate the visible fragments of the
	 * line. Method is to calc. the [0..1] parameter that describes
	 * the way from V1 to V2, for all intersections of the edge with
	 * any of the planes that define the 'shadow volume' of this
	 * polygon, and deciding what to do, with all this.
	 *
	 * The remaining work can be classified by the values of the
	 * variables calculated in previous tests: v{1/2}_rel_pplane,
	 * e_side[2][3], p_side[3]. Setting aside cases where one of them
	 * is zero, this leaves a total of 2^11 = 2048 separate cases, in
	 * principal. Several of those have been sorted out, already, by
	 * the previous tests. That leaves us with 3*27 cases,
	 * essentially. The '3' cases differ by which of the three
	 * vertices of the polygon is alone on its side of the test edge.
	 * That already fixes which two edges might intersect the test
	 * edge.  The 27 cases differ by which of the edge points lies on
	 * which of the three planes in space (two sides, and the polygon
	 * plane) that remain interesting for the tests.  That would
	 * normally make 2^2 = 4 cases per plane. But the 'both outside'
	 * cases for all three planes have already been taken care of,
	 * earlier, leaving 3 of those cases to be handled. This is the
	 * same for all three relevant planes, so we get 3^3 = 27
	 * cases. */

	{
	    /* the interception point parameters: */
	    double front_hit, this_hit, hit_in_poly;
	    long newvert[2];
	    int side1, side2;
	    double hit1, hit2;
	    unsigned int full_class;

	    /* find out which of the three edges would be intersected: */
	    if ((p_side[0] > 0) == (p_side[1] > 0)) {
		/* first two vertices on the same side, the third is on the
		 * 'other' side of the test edge */
		side1 = 1;
		side2 = 2;
	    } else if ((p_side[0] > 0) == (p_side[2] > 0)) {
		/* vertex '1' on the other side */
		side1 = 0;
		side2 = 1;
	    } else {
		/* we now know that it must be the first vertex that is on the
		 * 'other' side, as not all three can be on different
		 * sides. */
		side1 = 0;
		side2 = 2;
	    }

	    /* Carry out the classification into those 27 cases, based upon
	     * classification bits precomputed above, and calculate the
	     * intersection parameters, as appropriate: */

	    /* Classify wrt the polygons plane: '1' means point is in front of
	     * plane */

#define cleanup_hit(hit) \
 do { \
 if (EQ(hit,0)) \
  hit=0; \
 else if (EQ(hit,1)) \
  hit=1; \
 } while (0)

#define hit_in_edge(hit) ((hit >= 0) && (hit <= 1))

	    /* find the intersection point through the front plane, if any: */
	    front_hit = 1e10;
	    if (classification[3]) {
		if (SIGN(v1_rel_pplane - v2_rel_pplane)) {
		    front_hit = v1_rel_pplane / (v1_rel_pplane - v2_rel_pplane);
		    cleanup_hit(front_hit);
		}
		if (hit_in_edge(front_hit)) {
/*                                      printf("e%ld/p%ld: front_hit = %g\n",    edgenum, polynum, front_hit); */
		    /* do nothing */
		} else {
		    front_hit = 1e10;
		    classification[3] = 0;	/* not really intersecting */
		}
	    }
	    /* Find intersection points with the two relevant side planes of
	     * the shadow volume. A macro creates the code for side1 and
	     * side2, to reduce code overhead: */

#define check_out_hit(hit, side) \
 { \
  hit = 1e10;/* default is 'way off' */ \
  if (classification[side]) { \
   /* Not both inside, i.e. there has to be an intersection \
    * point: */ \
   hit_in_poly = - whichside * p_side[side] \
                 / (e_side[0][side] - e_side[1][side]); \
   cleanup_hit(hit_in_poly); \
   if (hit_in_edge(hit_in_poly)) { \
    this_hit = e_side[0][side] \
               / (e_side[0][side] - e_side[1][side]); \
    cleanup_hit(this_hit); \
    if (hit_in_edge(this_hit)) { \
     /*printf("e%ld/p%ld: side hit %g (h_in_p = %g)\n", */ \
     /* edgenum, polynum, this_hit, hit_in_poly);*/ \
     hit = this_hit; \
    } \
   } \
  } \
  if (hit == 1e10) \
   classification[side] = 0;/* doesn't really intersect! */ \
 }

	    check_out_hit(hit1, side1);
	    check_out_hit(hit2, side2);
#undef check_out_hit

	    /* OK, now we know the position of all the hits that might be
	     * needed, given by their positions between the test edge's
	     * vertices. Combine all the classifications into a single
	     * classification bitpattern, for easier listing of cases */

#define makeclass(s2, s1, p) (16 * s2 + 4 * s1 + p)

	    full_class = makeclass(classification[side2],
				   classification[side1],
				   classification[3]);

	    if (!full_class)
		/* all suspected intersections cleared, already! */
		continue;

#if 0
	    printf("--> T9: classification is %d/%d/%d = %d\n",
		   classification[side2],
		   classification[side1],
		   classification[3],
		   full_class);
#endif

	    /* first sort out all cases where one endpoint is attributed to
	     * more than one intersecting plane. This cuts down the 27 to 13
	     * cases (000, 001, 002, 012, and their permutations). Start
	     * with v1: */
	    switch (full_class & makeclass(1, 1, 1)) {
	    case makeclass(0, 1, 1):
		/* v1 is outside both 'p' and 'side1' */
		if (front_hit < hit1)
		    classification[3] = 0;
		else
		    classification[side1] = 0;
		break;

	    case makeclass(1, 0, 1):
		/* v1 outside 'side2' and 'p' planes */
		if (front_hit < hit2)
		    classification[3] = 0;
		else
		    classification[side2] = 0;
		break;

		/* FIXME: find out why the following two cases never
		 * trigger. At least not in a full run through all.dem. The
		 * corresponding 2 cases for the vertex2 classification don't
		 * either. */
	    case makeclass(1, 1, 0):
		/* v1 out both sides, but behind the front */
		if (hit1 < hit2)
		    /* hit2 is closer to the hidden vertex v2, so it's the
		     * relevant intersection: */
		    classification[side1] = 0;
		else
		    classification[side2] = 0;
		break;

	    case makeclass(1, 1, 1):
		/* v1 out both sides, and in front of p (--> v2 is hidden) */
		if (front_hit < hit1) {
		    classification[3] = 0;
		    if (hit1 < hit2)
			classification[side1] = 0;
		    else
			classification[side2] = 0;
		} else {
		    classification[side1] = 0;
		    if (front_hit < hit2)
			classification[3] = 0;
		    else
			classification[side2] = 0;
		}
		break;

	    default:
		break;
		/* do nothing */
	    }			/* switch(v1 classes) */


	    /* And the same, for v2 */
	    switch (full_class & makeclass(2, 2, 2)) {
	    case makeclass(0, 2, 2):
		/* v2 is outside both 'p' and 'side1' */
		if (front_hit > hit1)
		    classification[3] = 0;
		else
		    classification[side1] = 0;
		break;

	    case makeclass(2, 0, 2):
		/* v2 outside 'side2' and 'p' planes */
		if (front_hit > hit2)
		    classification[3] = 0;
		else
		    classification[side2] = 0;
		break;

	    case makeclass(2, 2, 0):
		/* v2 out both sides, but behind the front */
		if (hit1 > hit2)
		    /* hit2 is closer to the hidden vertex v2, so it's the
		     * relevant intersection: */
		    classification[side1] = 0;
		else
		    classification[side2] = 0;
		break;

	    case makeclass(2, 2, 2):
		/* v2 out both sides, and in front of p (--> v2 is hidden) */
		if (front_hit > hit1) {
		    classification[3] = 0;
		    if (hit1 > hit2)
			classification[side1] = 0;
		    else
			classification[side2] = 0;
		} else {
		    classification[side1] = 0;
		    if (front_hit > hit2)
			classification[3] = 0;
		    else
			classification[side2] = 0;
		}
		break;

	    default:
		break;
		/* do nothing */
	    }			/* switch(v2 classes) */


	    /* recalculate full_class after all these possible changes: */
	    full_class = makeclass(classification[side2],
				   classification[side1],
				   classification[3]);

	    /* This monster switch catches all the 13 remaining cases
	     * individually.  Currently, I still have pairs of them cast
	     * into common code, but that will probably change, as soon as I
	     * find time to do it more properly */

	    switch (full_class) {
	    default:
		printf("--> T9: class %d is illegal. Should never happen.\n",
		       full_class);
		break;
	    case makeclass(0, 0, 0):
		/* can happen, by resetting of classification bits inside this
		 * test */
/*                              printf("--> T9: splitting is cancelled already\n"); */
		break;

/*--------- The simplest cases first: only one intersection point -----*/

	    case makeclass(0, 0, 1):
		/* v1 is in front of poly: */
		newvert[0] = split_line_at_ratio(e, front_hit);
/*                              printf("--> T9: split e by p plane (case = %d)\n", full_class); */
		setup_edge(vnum1, newvert[0]);
		break;

	    case makeclass(0, 0, 2):
		/* v2 is in front of poly: */
		newvert[0] = split_line_at_ratio(e, front_hit);
/*                              printf("--> T9: split e by p plane (case = %d)\n", full_class); */
		setup_edge(newvert[0], vnum2);
		break;

	    case makeclass(0, 1, 0):
		/* v1 is out side1: */
		newvert[0] = split_line_at_ratio(e, hit1);
/*                              printf("--> T9: split e by side1 plane (case = %d)\n", full_class); */
		setup_edge(vnum1, newvert[0]);
		break;

	    case makeclass(0, 2, 0):
		/* v2 is out side2: */
		newvert[0] = split_line_at_ratio(e, hit1);
/*                              printf("--> T9: split e by side1 plane (case = %d)\n", full_class); */
		setup_edge(newvert[0], vnum2);
		break;

	    case makeclass(1, 0, 0):
		/* v1 is out side2 */
		newvert[0] = split_line_at_ratio(e, hit2);
/*                              printf("--> T9: split e by side2 plane (case = %d)\n", full_class); */
		setup_edge(vnum1, newvert[0]);
		break;

	    case makeclass(2, 0, 0):
		/* v2 is out side2 */
		newvert[0] = split_line_at_ratio(e, hit2);
/*                              printf("--> T9: split e by side2 plane (case = %d)\n", full_class); */
		setup_edge(newvert[0], vnum2);
		break;


/*----------- cases with 2 certain intersections: --------------*/
	    case makeclass(1, 2, 0):
	    case makeclass(2, 1, 0):
		/* both behind the front, v1 out one side, v2 out the other */
		/* FIXME: is that 'if' necessary? or can it be replaced by
		 * splitting this two-case mix? */
		newvert[0] = split_line_at_ratio(e, hit1);
		newvert[1] = split_line_at_ratio(e, hit2);
/*                              printf("--> T9: split e by side1 plane (case = %d)\n", full_class); */
		if (hit1 < hit2) {
		    /* the fragments are v1--hit1 and hit2--v2 */
		    handle_edge_fragment(e, newvert[1], vnum2, polynum);
		    e = elist + edgenum;	/* elist might have moved  */
		    setup_edge(vnum1, newvert[0]);
		} else {
		    /* the fragments are v2--hit1 and hit2--v1 */
		    handle_edge_fragment(e, vnum1, newvert[1], polynum);
		    e = elist + edgenum;	/* elist might have moved  */
		    setup_edge(newvert[0], vnum2);
		}
		/* return 0; */
		break;

/*----------- cases with either 2 or no intersections: --------------*/
/* CODEME: second hit is ignored, for all of these: */

	    case makeclass(0, 1, 2):
		/* v1 out side1 and in back, v2 in front */
		if (hit1 < front_hit) {
		    /* hit1 is closer to v1, front_hit is closer is closer to
		     * v2. --> we have two intersection points... */
		    newvert[0] = split_line_at_ratio(e, hit1);
		    newvert[1] = split_line_at_ratio(e, front_hit);
/*                                      printf("--> T9: split e by side1 plane (case= %d)\n", full_class); */
		    handle_edge_fragment(e, newvert[1], vnum2, polynum);
		    e = elist + edgenum;	/* elist might have moved  */
		    setup_edge(vnum1, newvert[0]);
		}
		/* otherwise, the edge missed the shadow volume */
		break;

	    case makeclass(0, 2, 1):
		/* v2 out side1 and in back, v1 in front */
		if (hit1 > front_hit) {
		    /* hit1 is closer to v2, front_hit is closer is closer to
		     * v1. --> we have two intersection points... */
		    newvert[0] = split_line_at_ratio(e, hit1);
		    newvert[1] = split_line_at_ratio(e, front_hit);
/*                                      printf("--> T9: split e by side1 plane (case= %d)\n", full_class); */
		    handle_edge_fragment(e, newvert[1], vnum2, polynum);
		    e = elist + edgenum;	/* elist might have moved  */
		    setup_edge(vnum1, newvert[0]);
		}
		/* otherwise, the edge missed the shadow volume */
		break;

	    case makeclass(1, 0, 2):
		if (hit2 < front_hit) {
		    /* hit2 is closer to v1, front_hit is closer is closer to
		     * v2. --> we have two intersection points... */
		    newvert[0] = split_line_at_ratio(e, hit2);
		    newvert[1] = split_line_at_ratio(e, front_hit);
/*                                      printf("--> T9: split e by side1 plane (case= %d)\n", full_class); */
		    handle_edge_fragment(e, newvert[1], vnum2, polynum);
		    e = elist + edgenum;	/* elist might have moved  */
		    setup_edge(vnum1, newvert[0]);
		}
		/* otherwise, the edge missed the shadow volume */
		break;

	    case makeclass(2, 0, 1):
		/* one is outside 'side2', the other in front of 'p' */
		if (hit2 > front_hit) {
		    /* hit2 is closer to v2, front_hit is closer is closer to
		     * v1. --> we have two intersection points... */
		    newvert[0] = split_line_at_ratio(e, hit2);
		    newvert[1] = split_line_at_ratio(e, front_hit);
/*                                      printf("--> T9: split e by side1 plane (case= %d)\n", full_class); */
		    handle_edge_fragment(e, newvert[1], vnum2, polynum);
		    e = elist + edgenum;	/* elist might have moved  */
		    setup_edge(vnum1, newvert[0]);
		}
		/* otherwise, the edge missed the shadow volume */
		break;

	    }			/* switch through the cases */
	    e->v1 = vnum1;
	    e->v2 = vnum2;
	}			/* if splitting necessary */
    }				/* for (polygons in list) */

    /* Came here, so there's something left of this polygon, to be drawn.
     * But the vertices are different, now, so copy our new vertices back
     * into 'e' */
    e->v1 = vnum1;
    e->v2 = vnum2;

    draw_edge(e);

    return 1;
}


/************************************************/
/*******            Drawing the polygons ********/
/************************************************/
/* Draw a line into the hl_buffer after clipping it to the visible
 * range. Uses Bresenham algorithm. The line is not drawn to the
 * terminal, only to the hl_buffer.  */

#define DRAW_vertex(v, x, y) \
 do { \
 if ((v)->style >= 0 && \
     !clip_point(x,y)  && \
     IS_UNSET(XREDUCE(x)-XREDUCE(xleft),YREDUCE(y)-YREDUCE(ybot))) \
  (*t->point)(x,y, v->style); \
  (v)->style = -1; \
 } while (0)

/* Externally callable function to draw a line, but hide it behind the
 * visible surface. Visibility check is done against the coverage
 * bitmap in 'pnt', using the 'IS_UNSET' macro. The line ist traced by
 * the classical 'Bresenham' algo, and only the visible portions are
 * passed on to the terminal driver as move/vector commands */

void
draw_line_hidden(x1, y1, x2, y2)
unsigned int x1, y1, x2, y2;
{
    /* FIXME: this is an enormous kludge, and will need
     * re-implementation. To work, draw_line_hidden would have to pass
     * in *3D* vertices. */
    (*term->move) (x1, y1);
    (*term->vector) (x2, y2);
}


/* The function that finally does the actual drawing of the visible
 * portions of lines */
static void
draw_edge(e)
p_edge e;
{
    p_vertex v1 = vlist + e->v1;
    p_vertex v2 = vlist + e->v2;
    unsigned int x1, y1, x2, y2;

    assert(e >= elist && e < elist + edges.end);

    TERMCOORD(v1, x1, y1);
    TERMCOORD(v2, x2, y2);
    term_apply_lp_properties(e->lp);
#ifdef PM3D
    if (e->lp->use_palette) {
	double z =  (v1->real_z + v2->real_z) * .5;
	set_color(z2gray(z));
    } else
#endif
    (term->linetype) (e->style);
    (*term->move) (x1, y1);
    (*term->vector) (x2, y2);
}


/***********************************************************************
 * and, finally, the 'mother function' that uses all these lots of tools
 ***********************************************************************/
void
plot3d_hidden(plots, pcount)
struct surface_points *plots;
int pcount;
{
    /* make vertices, edges and polygons out of all the plots */
    build_networks(plots, pcount);

    if (!edges.end) {
	/* No drawable edges found. Free all storage and bail out. */
	term_hidden_line_removal();
	graph_error("*All* edges undefined or out of range, thus no plot.");
    }
    if (!polygons.end) {
	/* No polygons anything could be hidden behind... */
	/* FIXME: put out a warning message ??? */
	int i;

	for (i = 0; i < edges.end; i++) {
	    draw_edge(elist + i);
	}
    } else {
	/* Presort edges in z order */
	sort_edges_by_z();
	/* Presort polygons in z order */
	sort_polys_by_z();

	while (efirst >= 0) {
	    if (elist[efirst].style >= -2)	/* skip invisible edges */
		in_front(efirst, &pfirst);
	    /* draw_edge(elist + efirst); */
	    efirst = elist[efirst].next;
	}
    }

    /* Free memory */
    /* FIXME: anything to free? */
}

/* Emacs editing help for HBB:
 * Local Variables: ***
 * tab-width:2 ***
 * End: ***
 */
