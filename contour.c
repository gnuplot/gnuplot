/* GNUPLOT - contour.c */
/*
 * Copyright (C) 1986, 1987, 1990, 1991   Thomas Williams, Colin Kelley
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
 *   Original Software:
 *       Gershon Elber
 * 
 * Send your comments or suggestions to 
 *  pixar!info-gnuplot@sun.com.
 * This is a mailing list; to join it send a note to 
 *  pixar!info-gnuplot-request@sun.com.  
 * Send bug reports to
 *  pixar!bug-gnuplot@sun.com.
 */

#include <stdio.h>
#include "plot.h"

#define DEFAULT_NUM_OF_ZLEVELS  10  /* Some dflt values (setable via flags). */
#define DEFAULT_NUM_APPROX_PTS  5
#define DEFAULT_BSPLINE_ORDER  3
#define MAX_NUM_OF_ZLEVELS      100 /* Some max. values (setable via flags). */
#define MAX_NUM_APPROX_PTS      100
#define MAX_BSPLINE_ORDER      10

#define INTERP_NOTHING   0            /* Kind of interpolations on contours. */
#define INTERP_CUBIC     1                           /* Cubic spline interp. */
#define APPROX_BSPLINE   2                         /* Bspline interpolation. */

#define ACTIVE   1                    /* Status of edges at certain Z level. */
#define INACTIVE 2

#define OPEN_CONTOUR     1                                 /* Contour kinds. */
#define CLOSED_CONTOUR   2

#define EPSILON  1e-5              /* Used to decide if two float are equal. */
#define INFINITY 1e10

#ifndef TRUE
#define TRUE     -1
#define FALSE    0
#endif

#define DEFAULT_NUM_CONTOURS	10
#define MAX_POINTS_PER_CNTR 	100
#define SHIFT_Z_EPSILON		0.000301060 /* Dec. change of poly bndry hit.*/

#define abs(x)  ((x) > 0 ? (x) : (-(x)))
#define sqr(x)  ((x) * (x))

typedef double tri_diag[3];         /* Used to allocate the tri-diag matrix. */
typedef double table_entry[4];	       /* Cubic spline interpolation 4 coef. */

struct vrtx_struct {
    double X, Y, Z;                       /* The coordinates of this vertex. */
    struct vrtx_struct *next;                             /* To chain lists. */
};

struct edge_struct {
    struct poly_struct *poly[2];   /* Each edge belongs to up to 2 polygons. */
    struct vrtx_struct *vertex[2]; /* The two extreme points of this vertex. */
    struct edge_struct *next;                             /* To chain lists. */
    int status, /* Status flag to mark edges in scanning at certain Z level. */
	boundary;                   /* True if this edge is on the boundary. */
};

struct poly_struct {
    struct edge_struct *edge[3];           /* As we do triangolation here... */
    struct poly_struct *next;                             /* To chain lists. */
};

struct cntr_struct {	       /* Contours are saved using this struct list. */
    double X, Y;                          /* The coordinates of this vertex. */
    struct cntr_struct *next;                             /* To chain lists. */
};

static int test_boundary;    /* If TRUE look for contours on boundary first. */
static struct gnuplot_contours *contour_list = NULL;
static double crnt_cntr[MAX_POINTS_PER_CNTR * 2];
static int crnt_cntr_pt_index = 0;
static double contour_level = 0.0;
static table_entry *hermit_table = NULL;    /* Hold hermite table constants. */
static int num_of_z_levels = DEFAULT_NUM_OF_ZLEVELS;  /* # Z contour levels. */
static int num_approx_pts = DEFAULT_NUM_APPROX_PTS;/* # pts per approx/inter.*/
static int bspline_order = DEFAULT_BSPLINE_ORDER;   /* Bspline order to use. */
static int interp_kind = INTERP_NOTHING;  /* Linear, Cubic interp., Bspline. */

static void gen_contours();
static int update_all_edges();
static struct cntr_struct *gen_one_contour();
static struct cntr_struct *trace_contour();
static struct cntr_struct *update_cntr_pt();
static int fuzzy_equal();
static void gen_triangle();
static struct vrtx_struct *gen_vertices();
static struct edge_struct *gen_edges_middle();
static struct edge_struct *gen_edges();
static struct poly_struct *gen_polys();
static void free_contour();
static void put_contour();
static put_contour_nothing();
static put_contour_cubic();
static put_contour_bspline();
static calc_tangent();
static int count_contour();
static complete_spline_interp();
static calc_hermit_table();
static hermit_interp();
static prepare_spline_interp();
static int solve_tri_diag();
static gen_bspline_approx();
static double fetch_knot();
static eval_bspline();

/*
 * Entry routine to this whole set of contouring module.
 */
struct gnuplot_contours *contour(num_isolines, iso_lines,
				 ZLevels, approx_pts, kind, order1)
int num_isolines;
struct iso_curve *iso_lines;
int ZLevels, approx_pts, kind, order1;
{
    int i;
    struct poly_struct *p_polys, *p_poly;
    struct edge_struct *p_edges, *p_edge;
    struct vrtx_struct *p_vrts, *p_vrtx;
    double x_min, y_min, z_min, x_max, y_max, z_max, z, dz, z_scale = 1.0;

    num_of_z_levels = ZLevels;
    num_approx_pts = approx_pts;
    bspline_order = order1 - 1;
    interp_kind = kind;

    contour_list = NULL;

    if (interp_kind == INTERP_CUBIC) calc_hermit_table();

    gen_triangle(num_isolines, iso_lines, &p_polys, &p_edges, &p_vrts,
		&x_min, &y_min, &z_min, &x_max, &y_max, &z_max);
    dz = (z_max - z_min) / (num_of_z_levels+1);
    /* Step from z_min+dz upto z_max-dz in num_of_z_levels times. */
    z = z_min + dz;
    crnt_cntr_pt_index = 0;

    for (i=0; i<num_of_z_levels; i++, z += dz) {
	contour_level = z;
	gen_contours(p_edges, z + dz * SHIFT_Z_EPSILON, x_min, x_max,
							y_min, y_max);
    }

    /* Free all contouring related temporary data. */
    while (p_polys) {
	p_poly = p_polys -> next;
	free (p_polys);
	p_polys = p_poly;
    }
    while (p_edges) {
	p_edge = p_edges -> next;
	free (p_edges);
	p_edges = p_edge;
    }
    while (p_vrts) {
	p_vrtx = p_vrts -> next;
	free (p_vrts);
	p_vrts = p_vrtx;
    }

    if (interp_kind == INTERP_CUBIC) free(hermit_table);

    return contour_list;
}

/*
 * Adds another point to the currently build contour.
 */
add_cntr_point(x, y)
double x, y;
{
    int index;

    if (crnt_cntr_pt_index >= MAX_POINTS_PER_CNTR-1) {
	index = crnt_cntr_pt_index - 1;
	end_crnt_cntr();
	crnt_cntr[0] = crnt_cntr[index * 2];
	crnt_cntr[1] = crnt_cntr[index * 2 + 1];
	crnt_cntr_pt_index = 1; /* Keep the last point as first of this one. */
    }
    crnt_cntr[crnt_cntr_pt_index * 2] = x;
    crnt_cntr[crnt_cntr_pt_index * 2 + 1] = y;
    crnt_cntr_pt_index++;
}

/*
 * Done with current contour - create gnuplot data structure for it.
 */
end_crnt_cntr()
{
    int i;
    struct gnuplot_contours *cntr = (struct gnuplot_contours *)
					alloc(sizeof(struct gnuplot_contours),
					      "gnuplot_contour");

    cntr->coords = (struct coordinate *) alloc(sizeof(struct coordinate) *
							  crnt_cntr_pt_index,
					       "contour coords");
    for (i=0; i<crnt_cntr_pt_index; i++) {
	cntr->coords[i].x = crnt_cntr[i * 2];
	cntr->coords[i].y = crnt_cntr[i * 2 + 1];
	cntr->coords[i].z = contour_level;
    }
    cntr->num_pts = crnt_cntr_pt_index;

    cntr->next = contour_list;
    contour_list = cntr;

    crnt_cntr_pt_index = 0;
}

/*
 * Generates all contours by tracing the intersecting triangles.
 */
static void gen_contours(p_edges, z_level, x_min, x_max, y_min, y_max)
struct edge_struct *p_edges;
double z_level, x_min, x_max, y_min, y_max;
{
    int num_active,                        /* Number of edges marked ACTIVE. */
	contour_kind;                /* One of OPEN_CONTOUR, CLOSED_CONTOUR. */
    struct cntr_struct *p_cntr;

    num_active = update_all_edges(p_edges, z_level);           /* Do pass 1. */

    test_boundary = TRUE;        /* Start to look for contour on boundaries. */

    while (num_active > 0) {                                   /* Do Pass 2. */
        /* Generate One contour (and update MumActive as needed): */
	p_cntr = gen_one_contour(p_edges, z_level, &contour_kind, &num_active);
	put_contour(p_cntr, z_level, x_min, x_max, y_min, y_max,
			      contour_kind); /* Emit it in requested format. */
    }
}

/*
 * Does pass 1, or marks the edges which are active (crosses this z_level)
 * as ACTIVE, and the others as INACTIVE:
 * Returns number of active edges (marked ACTIVE).
 */
static int update_all_edges(p_edges, z_level)
struct edge_struct *p_edges;
double z_level;
{
    int count = 0;

    while (p_edges) {
	if (((p_edges -> vertex[0] -> Z >= z_level) &&
	     (p_edges -> vertex[1] -> Z <= z_level)) ||
	    ((p_edges -> vertex[1] -> Z >= z_level) &&
	     (p_edges -> vertex[0] -> Z <= z_level))) {
	    p_edges -> status = ACTIVE;
	    count++;
	}
	else p_edges -> status = INACTIVE;
	p_edges = p_edges -> next;
    }

    return count;
}

/*
 * Does pass 2, or find one complete contour out of the triangolation data base:
 * Returns a pointer to the contour (as linked list), contour_kind is set to
 * one of OPEN_CONTOUR, CLOSED_CONTOUR, and num_active is updated.
 */
static struct cntr_struct *gen_one_contour(p_edges, z_level, contour_kind,
								num_active)
struct edge_struct *p_edges;
double z_level;
int *contour_kind, *num_active;
{
    struct edge_struct *pe_temp;

    if (test_boundary) {    /* Look for something to start with on boundary: */
	pe_temp = p_edges;
	while (pe_temp) {
	    if ((pe_temp -> status == ACTIVE) && (pe_temp -> boundary)) break;
	    pe_temp = pe_temp -> next;
	}
	if (!pe_temp) test_boundary = FALSE;/* No more contours on boundary. */
	else {
	    *contour_kind = OPEN_CONTOUR;
	    return trace_contour(pe_temp, z_level, num_active, *contour_kind);
	}
    }

    if (!test_boundary) {        /* Look for something to start with inside: */
	pe_temp = p_edges;
	while (pe_temp) {
	    if ((pe_temp -> status == ACTIVE) && (!(pe_temp -> boundary)))
		break;
	    pe_temp = pe_temp -> next;
	}
	if (!pe_temp) {
	    *num_active = 0;
	    return NULL;
	}
	else {
	    *contour_kind = CLOSED_CONTOUR;
	    return trace_contour(pe_temp, z_level, num_active, *contour_kind);
	}
    }
    return NULL;		     /* We should never be here, but lint... */
}

/*
 * Search the data base along a contour starts at the edge pe_start until
 * a boundary edge is detected or until we close the loop back to pe_start.
 * Returns a linked list of all the points on the contour
 * Also decreases num_active by the number of points on contour.
 */
static struct cntr_struct *trace_contour(pe_start, z_level, num_active,
								contour_kind)
struct edge_struct *pe_start;
double z_level;
int *num_active, contour_kind;
{
    int i, in_middle;       /* If TRUE the z_level is in the middle of edge. */
    struct cntr_struct *p_cntr, *pc_tail;
    struct edge_struct *p_edge = pe_start, *p_next_edge;
    struct poly_struct *p_poly, *PLastpoly = NULL;

    /* Generate the header of the contour - the point on pe_start. */
    if (contour_kind == OPEN_CONTOUR) pe_start -> status = INACTIVE;
    (*num_active)--;
    p_cntr = pc_tail = update_cntr_pt(pe_start, z_level, &in_middle);
    if (!in_middle) {
	return NULL;
    }

    do {
	/* Find polygon to continue (Not where we came from - PLastpoly): */
	if (p_edge -> poly[0] == PLastpoly) p_poly = p_edge -> poly[1];
	else p_poly = p_edge -> poly[0];
	p_next_edge = NULL;		  /* In case of error, remains NULL. */
	for (i=0; i<3; i++)              /* Test the 3 edges of the polygon: */
	    if (p_poly -> edge[i] != p_edge)
	        if (p_poly -> edge[i] -> status == ACTIVE)
		    p_next_edge = p_poly -> edge[i];
	if (!p_next_edge) {
	    pc_tail -> next = NULL;
	    free_contour(p_cntr);
	    return NULL;
	}
	p_edge = p_next_edge;
	PLastpoly = p_poly;
	p_edge -> status = INACTIVE;
	(*num_active)--;
	pc_tail -> next = update_cntr_pt(p_edge, z_level, &in_middle);
	if (!in_middle) {
	    pc_tail -> next = NULL;
	    free_contour(p_cntr);
	    return NULL;
	}
        pc_tail = pc_tail -> next;
    }
    while ((pe_start != p_edge) && (!p_edge -> boundary));
    pc_tail -> next = NULL;

    return p_cntr;
}

/*
 * Allocates one contour location and update it to to correct position
 * according to z_level and edge p_edge. if z_level is found to be at
 * one of the extreme points nothing is allocated (NULL is returned)
 * and in_middle is set to FALSE.
 */
static struct cntr_struct *update_cntr_pt(p_edge, z_level, in_middle)
struct edge_struct *p_edge;
double z_level;
int *in_middle;
{
    double t;
    struct cntr_struct *p_cntr;

    t = (z_level - p_edge -> vertex[0] -> Z) /
	(p_edge -> vertex[1] -> Z - p_edge -> vertex[0] -> Z);

    if (fuzzy_equal(t, 1.0) || fuzzy_equal(t, 0.0)) {
        *in_middle = FALSE;
        return NULL;
    }
    else {
	*in_middle = TRUE;
	p_cntr = (struct cntr_struct *) alloc(sizeof(struct cntr_struct),
							"contour cntr_struct");
	p_cntr -> X = p_edge -> vertex[1] -> X * t +
		     p_edge -> vertex[0] -> X * (1-t);
	p_cntr -> Y = p_edge -> vertex[1] -> Y * t +
		     p_edge -> vertex[0] -> Y * (1-t);
	return p_cntr;
    }
}

/*
 * Simple routine to decide if two real values are equal by simply
 * calculating the relative/absolute error between them (< EPSILON).
 */
static int fuzzy_equal(x, y)
double x, y;
{
    if (abs(x) > EPSILON)			/* Calculate relative error: */
        return (abs((x - y) / x) < EPSILON);
    else					/* Calculate absolute error: */
	return (abs(x - y) < EPSILON);
}

/*
 * Generate the triangles.
 * Returns the lists (vrtxs edges & polys) via pointers to their heads.
 */
static void gen_triangle(num_isolines, iso_lines, p_polys, p_edges,
	p_vrts, x_min, y_min, z_min, x_max, y_max, z_max)
int num_isolines;
struct iso_curve *iso_lines;
struct poly_struct **p_polys;
struct edge_struct **p_edges;
struct vrtx_struct **p_vrts;
double *x_min, *y_min, *z_min, *x_max, *y_max, *z_max;
{
    int i, grid_x_max = iso_lines->p_count;
    struct vrtx_struct *p_vrtx1, *p_vrtx2, *pv_temp;
    struct edge_struct *p_edge1, *p_edge2, *pe_tail1, *pe_tail2, *pe_temp,
		      *p_edge_middle, *pe_m_tail;
    struct poly_struct *p_poly, *pp_tail;

    *p_polys = NULL;
    *p_edges = NULL;
    *p_vrts = NULL;
    *z_min = INFINITY;
    *y_min = INFINITY;
    *x_min = INFINITY;
    *z_max = -INFINITY;
    *y_max = -INFINITY;
    *x_max = -INFINITY;

    /* Read 1st row. */
    p_vrtx1 = gen_vertices(grid_x_max, iso_lines->points,
			   x_min, y_min, z_min, x_max, y_max, z_max);
    *p_vrts = p_vrtx1;
    /* Gen. its edges.*/
    pe_temp = p_edge1 = gen_edges(grid_x_max, p_vrtx1, &pe_tail1);
    for (i = 1; i < grid_x_max; i++) {/* Mark one side of edges as boundary. */
	pe_temp -> poly[1] = NULL;
	pe_temp = pe_temp -> next;
    }
    for (i = 1; i < num_isolines; i++) { /* Read next column and gen. polys. */
	iso_lines = iso_lines->next;
    	/* Get row into list. */
        p_vrtx2 = gen_vertices(grid_x_max, iso_lines->points,
			       x_min, y_min, z_min, x_max, y_max, z_max);
    	/* Generate its edges. */
        p_edge2 = gen_edges(grid_x_max, p_vrtx2, &pe_tail2);
	/* Generate edges from one vertex list to the other one: */
	p_edge_middle = gen_edges_middle(grid_x_max, p_vrtx1, p_vrtx2,
								 &pe_m_tail);

	/* Now we can generate the polygons themselves (triangles). */
	p_poly = gen_polys(grid_x_max, p_edge1, p_edge_middle, p_edge2,
								 &pp_tail);
        pe_tail1 -> next = (*p_edges);      /* Chain new edges to main list. */
        pe_m_tail -> next = p_edge1;
	*p_edges = p_edge_middle;
	pe_tail1 = pe_tail2;
	p_edge1 = p_edge2;

	pv_temp = p_vrtx2;
	while (pv_temp -> next) pv_temp = pv_temp -> next;
	pv_temp -> next = *p_vrts;
	*p_vrts = p_vrtx1 = p_vrtx2;

        pp_tail -> next = (*p_polys);       /* Chain new polys to main list. */
	*p_polys = p_poly;
    }

    pe_temp = p_edge1;
    for (i = 1; i < grid_x_max; i++) {/* Mark one side of edges as boundary. */
	pe_temp -> poly[0] = NULL;
	pe_temp = pe_temp -> next;
    }

    pe_tail1 -> next = (*p_edges);    /* Chain last edges list to main list. */
    *p_edges = p_edge1;

    /* Update the boundary flag, saved in each edge, and update indexes: */
    pe_temp = (*p_edges);
    i = 1;

    while (pe_temp) {
	pe_temp -> boundary = (!(pe_temp -> poly[0])) ||
			      (!(pe_temp -> poly[1]));
	pe_temp = pe_temp -> next;
    }
}

/*
 * Handles grid_x_max 3D points (One row) and generate linked list for them.
 */
static struct vrtx_struct *gen_vertices(grid_x_max, points,
				      x_min, y_min, z_min, x_max, y_max, z_max)
int grid_x_max;
struct coordinate *points;
double *x_min, *y_min, *z_min, *x_max, *y_max, *z_max;
{
    int i;
    struct vrtx_struct *p_vrtx, *pv_tail, *pv_temp;

    for (i=0; i<grid_x_max; i++) {/* Get a point and generate the structure. */
        pv_temp = (struct vrtx_struct *) alloc(sizeof(struct vrtx_struct),
						"contour vertex");
	pv_temp -> X = points[i].x;
	pv_temp -> Y = points[i].y;
	pv_temp -> Z = points[i].z;

	if (pv_temp -> X > *x_max) *x_max = pv_temp -> X; /* Update min/max. */
	if (pv_temp -> Y > *y_max) *y_max = pv_temp -> Y;
	if (pv_temp -> Z > *z_max) *z_max = pv_temp -> Z;
	if (pv_temp -> X < *x_min) *x_min = pv_temp -> X;
	if (pv_temp -> Y < *y_min) *y_min = pv_temp -> Y;
	if (pv_temp -> Z < *z_min) *z_min = pv_temp -> Z;

	if (i == 0)                              /* First vertex in row: */
	    p_vrtx = pv_tail = pv_temp;
	else {
	    pv_tail -> next = pv_temp;   /* Stick new record as last one. */
	    pv_tail = pv_tail -> next;    /* And continue to last record. */
	}
    }
    pv_tail -> next = NULL;

    return p_vrtx;
}

/*
 * Combines N vertices in pair to form N-1 edges.
 * Returns pointer to the edge list (pe_tail will point on last edge in list).
 */
static struct edge_struct *gen_edges(grid_x_max, p_vrtx, pe_tail)
int grid_x_max;
struct vrtx_struct *p_vrtx;
struct edge_struct **pe_tail;
{
    int i;
    struct edge_struct *p_edge, *pe_temp;

    for (i=0; i<grid_x_max-1; i++) {         /* Generate grid_x_max-1 edges: */
	pe_temp = (struct edge_struct *) alloc(sizeof(struct edge_struct),
						"contour edge");
	pe_temp -> vertex[0] = p_vrtx;              /* First vertex of edge. */
	p_vrtx = p_vrtx -> next;                     /* Skip to next vertex. */
	pe_temp -> vertex[1] = p_vrtx;             /* Second vertex of edge. */
        if (i == 0)                                    /* First edge in row: */
	    p_edge = (*pe_tail) = pe_temp;
	else {
	    (*pe_tail) -> next = pe_temp;   /* Stick new record as last one. */
	    *pe_tail = (*pe_tail) -> next;   /* And continue to last record. */
 	}
    }
    (*pe_tail) -> next = NULL;

    return p_edge;
}

/*
 * Combines 2 lists of N vertices each into edge list:
 * The dots (.) are the vertices list, and the              .  .  .  .
 *  edges generated are alternations of vertical edges      |\ |\ |\ |
 *  (|) and diagonal ones (\).                              | \| \| \|
 *  A pointer to edge list (alternate | , \) is returned    .  .  .  .
 * Note this list will have (2*grid_x_max-1) edges (pe_tail points on last
 * record).
 */
static struct edge_struct *gen_edges_middle(grid_x_max, p_vrtx1, p_vrtx2,
								pe_tail)
int grid_x_max;
struct vrtx_struct *p_vrtx1, *p_vrtx2;
struct edge_struct **pe_tail;
{
    int i;
    struct edge_struct *p_edge, *pe_temp;

    /* Gen first (|). */
    pe_temp = (struct edge_struct *) alloc(sizeof(struct edge_struct),
							"contour edge");
    pe_temp -> vertex[0] = p_vrtx2;                 /* First vertex of edge. */
    pe_temp -> vertex[1] = p_vrtx1;                /* Second vertex of edge. */
    p_edge = (*pe_tail) = pe_temp;

    /* Advance in vrtx list grid_x_max-1 times, and gen. 2 edges /| for each.*/
    for (i=0; i<grid_x_max-1; i++) {
	/* The / edge. */
	pe_temp = (struct edge_struct *) alloc(sizeof(struct edge_struct),
							"contour edge");
	pe_temp -> vertex[0] = p_vrtx1;             /* First vertex of edge. */
	pe_temp -> vertex[1] = p_vrtx2 -> next;    /* Second vertex of edge. */
        (*pe_tail) -> next = pe_temp;       /* Stick new record as last one. */
	*pe_tail = (*pe_tail) -> next;       /* And continue to last record. */

	/* The | edge. */
	pe_temp = (struct edge_struct *) alloc(sizeof(struct edge_struct),
							"contour edge");
	pe_temp -> vertex[0] = p_vrtx2 -> next;     /* First vertex of edge. */
	pe_temp -> vertex[1] = p_vrtx1 -> next;    /* Second vertex of edge. */
        (*pe_tail) -> next = pe_temp;       /* Stick new record as last one. */
	*pe_tail = (*pe_tail) -> next;       /* And continue to last record. */

        p_vrtx1 = p_vrtx1 -> next;   /* Skip to next vertices in both lists. */
        p_vrtx2 = p_vrtx2 -> next;
    }
    (*pe_tail) -> next = NULL;

    return p_edge;
}

/*
 * Combines 3 lists of edges into triangles:
 * 1. p_edge1: Top horizontal edge list:        -----------------------
 * 2. p_edge_middge: middle edge list:         |\  |\  |\  |\  |\  |\  |
 *                                             |  \|  \|  \|  \|  \|  \|
 * 3. p_edge2: Bottom horizontal edge list:     -----------------------
 * Note that p_edge1/2 lists has grid_x_max-1 edges, while p_edge_middle has
 * (2*grid_x_max-1) edges.
 * The routine simple scans the two list    Upper 1         Lower
 * and generate two triangle upper one        ----         | \
 * and lower one from the lists:             0\   |2      0|   \1
 * (Nums. are edges order in polys)             \ |         ----
 * The routine returns a pointer to a                         2
 * polygon list (pp_tail points on last polygon).          1
 *                                                   -----------
 * In addition, the edge lists are updated -        | \   0     |
 * each edge has two pointers on the two            |   \       |
 * (one active if boundary) polygons which         0|1   0\1   0|1
 * uses it. These two pointer to polygons           |       \   |
 * are named: poly[0], poly[1]. The diagram         |    1    \ |
 * on the right show how they are used for the       -----------
 * upper and lower polygons.                             0
 */
static struct poly_struct *gen_polys(grid_x_max, p_edge1, p_edge_middle,
							p_edge2, pp_tail)
int grid_x_max;
struct edge_struct *p_edge1, *p_edge_middle, *p_edge2;
struct poly_struct **pp_tail;
{
    int i;
    struct poly_struct *p_poly, *pp_temp;

    p_edge_middle -> poly[0] = NULL;			    /* Its boundary! */

    /* Advance in vrtx list grid_x_max-1 times, and gen. 2 polys for each. */
    for (i=0; i<grid_x_max-1; i++) {
	/* The Upper. */
	pp_temp = (struct poly_struct *) alloc(sizeof(struct poly_struct),
							"contour poly");
	/* Now update polys about its edges, and edges about the polygon. */
	pp_temp -> edge[0] = p_edge_middle -> next;
	p_edge_middle -> next -> poly[1] = pp_temp;
	pp_temp -> edge[1] = p_edge1;
	p_edge1 -> poly[0] = pp_temp;
	pp_temp -> edge[2] = p_edge_middle -> next -> next;
	p_edge_middle -> next -> next -> poly[0] = pp_temp;
	if (i == 0)				   /* Its first one in list: */
	    p_poly = (*pp_tail) = pp_temp;
	else {
	    (*pp_tail) -> next = pp_temp;
	    *pp_tail = (*pp_tail) -> next;
	}

	/* The Lower. */
	pp_temp = (struct poly_struct *) alloc(sizeof(struct poly_struct),
							"contour poly");
	/* Now update polys about its edges, and edges about the polygon. */
	pp_temp -> edge[0] = p_edge_middle;
	p_edge_middle -> poly[1] = pp_temp;
	pp_temp -> edge[1] = p_edge_middle -> next;
	p_edge_middle -> next -> poly[0] = pp_temp;
	pp_temp -> edge[2] = p_edge2;
	p_edge2 -> poly[1] = pp_temp;
	(*pp_tail) -> next = pp_temp;
	*pp_tail = (*pp_tail) -> next;

        p_edge1 = p_edge1 -> next;
        p_edge2 = p_edge2 -> next;
        p_edge_middle = p_edge_middle -> next -> next;
    }
    p_edge_middle -> poly[1] = NULL;			    /* Its boundary! */
    (*pp_tail) -> next = NULL;

    return p_poly;
}

/*
 * Calls the (hopefully) desired interpolation/approximation routine.
 */
static void put_contour(p_cntr, z_level, x_min, x_max, y_min, y_max, contr_kind)
struct cntr_struct *p_cntr;
double z_level, x_min, x_max, y_min, y_max;
int contr_kind;
{
    if (!p_cntr) return;            /* Nothing to do if it is empty contour. */

    switch (interp_kind) {
	case INTERP_NOTHING:              /* No interpolation/approximation. */
	    put_contour_nothing(p_cntr);
	    break;
	case INTERP_CUBIC:                    /* Cubic spline interpolation. */
	    put_contour_cubic(p_cntr, z_level, x_min, x_max, y_min, y_max,
								contr_kind);
	    break;
	case APPROX_BSPLINE:                       /* Bspline approximation. */
	    put_contour_bspline(p_cntr, z_level, x_min, x_max, y_min, y_max,
    								contr_kind);
	    break;
    }

    free_contour(p_cntr);
}

/*
 * Simply puts contour coordinates in order with no interpolation or
 * approximation.
 */
static put_contour_nothing(p_cntr)
struct cntr_struct *p_cntr;
{
    while (p_cntr) {
	add_cntr_point(p_cntr -> X, p_cntr -> Y);
	p_cntr = p_cntr -> next;
    }
    end_crnt_cntr();
}

/*
 * Find Complete Cubic Spline Interpolation.
 */
static put_contour_cubic(p_cntr, z_level, x_min, x_max, y_min, y_max,
								 contr_kind)
struct cntr_struct *p_cntr;
double z_level, x_min, x_max, y_min, y_max;
int contr_kind;
{
    int num_pts, i;
    double tx1, ty1, tx2, ty2;                    /* Tangents at end points. */
    struct cntr_struct *pc_temp;

    num_pts = count_contour(p_cntr);         /* Number of points in contour. */

    if (num_pts > 2) {  /* Take into account 3 points in tangent estimation. */
	calc_tangent(3, p_cntr -> X, p_cntr -> next -> X,
			p_cntr -> next -> next -> X,
			p_cntr -> Y, p_cntr -> next -> Y,
			p_cntr -> next -> next -> Y, &tx1, &ty1);
	pc_temp = p_cntr;
	for (i=3; i<num_pts; i++) pc_temp = pc_temp -> next;/* Go to the end.*/
	calc_tangent(3, pc_temp -> next -> next -> X,
 			pc_temp -> next -> X, pc_temp -> X,
			pc_temp -> next -> next -> Y,
			pc_temp -> next -> Y, pc_temp -> Y, &tx2, &ty2);
        tx2 = (-tx2);   /* Inverse the vector as we need opposite direction. */
        ty2 = (-ty2);
    }
    /* If following (num_pts > 1) is TRUE then exactly 2 points in contour.  */
    else if (num_pts > 1) {/* Take into account 2 points in tangent estimat. */
	calc_tangent(2, p_cntr -> X, p_cntr -> next -> X, 0.0,
			p_cntr -> Y, p_cntr -> next -> Y, 0.0, &tx1, &ty1);
	calc_tangent(2, p_cntr -> next -> X, p_cntr -> X, 0.0,
			p_cntr -> next -> Y, p_cntr -> Y, 0.0, &tx2, &ty2);
        tx2 = (-tx2);   /* Inverse the vector as we need opposite direction. */
        ty2 = (-ty2);
    }
    else return;			/* Only one point (???) - ignore it. */

    switch (contr_kind) {
	case OPEN_CONTOUR:
	    break;
	case CLOSED_CONTOUR:
	    tx1 = tx2 = (tx1 + tx2) / 2.0;	     /* Make tangents equal. */
	    ty1 = ty2 = (ty1 + ty2) / 2.0;
	    break;
    }
    complete_spline_interp(p_cntr, num_pts, 0.0, 1.0, tx1, ty1, tx2, ty2);
    end_crnt_cntr();
}

/*
 * Find Bspline approximation for this data set.
 * Uses global variable num_approx_pts to determine number of samples per
 * interval, where the knot vector intervals are assumed to be uniform, and
 * Global variable bspline_order for the order of Bspline to use.
 */
static put_contour_bspline(p_cntr, z_level, x_min, x_max, y_min, y_max,
								contr_kind)
struct cntr_struct *p_cntr;
double z_level, x_min, x_max,  y_min, y_max;
int contr_kind;
{
    int num_pts, i, order = bspline_order;
    struct cntr_struct *pc_temp;

    num_pts = count_contour(p_cntr);         /* Number of points in contour. */
    if (num_pts < 2) return;     /* Cannt do nothing if empty or one points! */
    /* Order must be less than number of points in curve - fix it if needed. */
    if (order > num_pts - 1) order = num_pts - 1;

    gen_bspline_approx(p_cntr, num_pts, order, contr_kind);
    end_crnt_cntr();
}

/*
 * Estimate the tangents according to the n last points where n might be
 * 2 or 3 (if 2 onlt x1, x2).
 */
static calc_tangent(n, x1, x2, x3, y1, y2, y3, tx, ty)
int n;
double x1, x2, x3, y1, y2, y3, *tx, *ty;
{
    double v1[2], v2[2], v1_magnitude, v2_magnitude;

    switch (n) {
	case 2:
	    *tx = (x2 - x1) * 0.3;
	    *ty = (y2 - y1) * 0.3;
	    break;
	case 3:
	    v1[0] = x2 - x1;   v1[1] = y2 - y1;
	    v2[0] = x3 - x2;   v2[1] = y3 - y2;
	    v1_magnitude = sqrt(sqr(v1[0]) + sqr(v1[1]));
	    v2_magnitude = sqrt(sqr(v2[0]) + sqr(v2[1]));
	    *tx = (v1[0] / v1_magnitude) - (v2[0] / v2_magnitude) * 0.1;
	    *tx *= v1_magnitude * 0.1;  /* Make tangent less than magnitude. */
	    *ty = (v1[1] / v1_magnitude) - (v2[1] / v2_magnitude) * 0.1;
	    *ty *= v1_magnitude * 0.1;  /* Make tangent less than magnitude. */
	    break;
	default:				       /* Should not happen! */
	    (*ty) = 0.1;
	    *tx = 0.1;
	    break;
    }
}

/*
 * Free all elements in the contour list.
 */
static void free_contour(p_cntr)
struct cntr_struct *p_cntr;
{
    struct cntr_struct *pc_temp;

    while (p_cntr) {
	pc_temp = p_cntr;
	p_cntr = p_cntr -> next;
	free((char *) pc_temp);
    }
}

/*
 * Counts number of points in contour.
 */
static int count_contour(p_cntr)
struct cntr_struct *p_cntr;
{
    int count = 0;

    while (p_cntr) {
	count++;
	p_cntr = p_cntr -> next;
    }
    return count;
}

/*
 * Interpolate given point list (defined via p_cntr) using Complete
 * Spline interpolation.
 */
static complete_spline_interp(p_cntr, n, t_min, t_max, tx1, ty1, tx2, ty2)
struct cntr_struct *p_cntr;
int n;
double t_min, t_max, tx1, ty1, tx2, ty2;
{
    double dt, *tangents_x, *tangents_y;
    int i;

    tangents_x = (double *) alloc((unsigned) (sizeof(double) * n),
						"contour c_s_intr");
    tangents_y = (double *) alloc((unsigned) (sizeof(double) * n),
						"contour c_s_intr");

    if (n > 1) prepare_spline_interp(tangents_x, tangents_y, p_cntr, n,
					t_min, t_max, tx1, ty1, tx2, ty2);
    else {
	free((char *) tangents_x);
	free((char *) tangents_y);
	return;
    }

    dt = (t_max-t_min)/(n-1);

    add_cntr_point(p_cntr -> X, p_cntr -> Y);	       /* First point. */

    for (i=0; i<n-1; i++) {
        hermit_interp(p_cntr -> X, p_cntr -> Y,
                     tangents_x[i], tangents_y[i],
		     p_cntr -> next -> X, p_cntr -> next -> Y,
                     tangents_x[i+1], tangents_y[i+1], dt);

        p_cntr = p_cntr -> next;
    }

    free((char *) tangents_x);
    free((char *) tangents_y);
}

/*
 * Routine to calculate intermidiate value of the Hermit Blending function:
 * This routine should be called only ONCE at the beginning of the program.
 */
static calc_hermit_table()
{
    int i;
    double t, dt;

    hermit_table = (table_entry *) alloc ((unsigned) (sizeof(table_entry) *
						(num_approx_pts + 1)),
						"contour hermit table");
    t = 0;
    dt = 1.0/num_approx_pts;
    for (i=0; i<=num_approx_pts; i++) {
        hermit_table[i][0] = (t-1)*(t-1)*(2*t+1);		     /* h00. */
        hermit_table[i][1] = t*t*(-2*t+3);			     /* h10. */
        hermit_table[i][2] = t*(t-1)*(t-1);			     /* h01. */
        hermit_table[i][3] = t*t*(t-1);				     /* h11. */
        t = t + dt;
    }
}

/*
 * Routine to generate an hermit interpolation between two points given as
 * two InterpStruct structures. Assume hermit_table is already calculated.
 * Currently the points generated are printed to stdout as two reals (X, Y).
 */
static hermit_interp(x1, y1, tx1, ty1, x2, y2, tx2, ty2, dt)
double x1, y1, tx1, ty1, x2, y2, tx2, ty2, dt;
{
    int i;
    double x, y, vec_size, tang_size;

    tx1 *= dt;  ty1 *= dt; /* Normalize the tangents according to param. t.  */
    tx2 *= dt;  ty2 *= dt;

    /* Normalize the tangents so that their magnitude will be 1/3 of the     */
    /* segment length. This tumb rule guaranteed no cusps or loops!          */
    /* Note that this normalization keeps continuity to be G1 (but not C1).  */
    vec_size = sqrt(sqr(x1 - x2) + sqr(y2 - y1));
    tang_size = sqrt(sqr(tx1) + sqr(ty1));                  /* Normalize T1. */
    if (tang_size * 3 > vec_size) {
	tx1 *= vec_size / (tang_size * 3);
	ty1 *= vec_size / (tang_size * 3);
    }
    tang_size = sqrt(sqr(tx2) + sqr(ty2));                  /* Normalize T2. */
    if (tang_size * 3 > vec_size) {
	tx2 *= vec_size / (tang_size * 3);
	ty2 *= vec_size / (tang_size * 3);
    }

    for (i=1; i<=num_approx_pts; i++) {      /* Note we start from 1 - first */
        x = hermit_table[i][0] * x1 +       /* point is not printed as it is */
            hermit_table[i][1] * x2 +   /* redundent (last on last section). */
            hermit_table[i][2] * tx1 +
            hermit_table[i][3] * tx2;
        y = hermit_table[i][0] * y1 +
            hermit_table[i][1] * y2 +
            hermit_table[i][2] * ty1 +
            hermit_table[i][3] * ty2;
	add_cntr_point(x, y);
    }
}

/*
 * Routine to Set up the 3*N mat for solve_tri_diag routine used in the
 * Complete Spline Interpolation. Returns TRUE of calc O.K.
 * Gets the points list in p_cntr (Of length n) and with tangent vectors tx1,
 * ty1 at starting point and tx2, ty2 and end point.
 */
static prepare_spline_interp(tangents_x, tangents_y, p_cntr, n, t_min, t_max,
			   tx1, ty1, tx2, ty2)
double tangents_x[], tangents_y[];
struct cntr_struct *p_cntr;
int n;
double t_min, t_max, tx1, ty1, tx2, ty2;
{
    int i;
    double *r, t, dt;
    tri_diag *m;		   /* The tri-diagonal matrix is saved here. */
    struct cntr_struct *p;

    m = (tri_diag *) alloc((unsigned) (sizeof(tri_diag) * n),
						"contour tri_diag");
    r = (double *) alloc((unsigned) (sizeof(double) * n),
						"contour tri_diag2");
    n--;

    p = p_cntr;
    m[0][0] = 0.0;    m[0][1] = 1.0;    m[0][2] = 0.0;
    m[n][0] = 0.0;    m[n][1] = 1.0;    m[n][2] = 0.0;
    r[0] = tx1;					       /* Set start tangent. */
    r[n] = tx2;						 /* Set end tangent. */
    t = t_min;
    dt = (t_max-t_min)/n;
    for (i=1; i<n; i++) {
       t = t + dt;
       m[i][0] = dt;
       m[i][2] = dt;
       m[i][1] = 2 * (m[i][0] + m[i][2]);
       r[i] = m[i][0] * ((p -> next -> X) - (p -> X)) / m[i][2]
            + m[i][2] * ((p -> next -> next -> X) - 
                         (p -> next -> X)) / m[i][0];
       r[i] *= 3.0;
       p = p -> next;
    }

    if (!solve_tri_diag(m, r, tangents_x, n+1)) { /* Find the X(t) tangents. */
    	free((char *) m);
    	free((char *) r);
	int_error("Cannt interpolate X using complete splines", NO_CARET);
    }

    p = p_cntr;
    m[0][0] = 0.0;    m[0][1] = 1.0;    m[0][2] = 0.0;
    m[n][0] = 0.0;    m[n][1] = 1.0;    m[n][2] = 0.0;
    r[0] = ty1;					       /* Set start tangent. */
    r[n] = ty2;						 /* Set end tangent. */
    t = t_min;
    dt = (t_max-t_min)/n;
    for (i=1; i<n; i++) {
       t = t + dt;
       m[i][0] = dt;
       m[i][2] = dt;
       m[i][1] = 2 * (m[i][0] + m[i][2]);
       r[i] = m[i][0] * ((p -> next -> Y) - (p -> Y)) / m[i][2]
            + m[i][2] * ((p -> next -> next -> Y) -
                         (p -> next -> Y)) / m[i][0];
       r[i] *= 3.0;
       p = p -> next;
    }

    if (!solve_tri_diag(m, r, tangents_y, n+1)) { /* Find the Y(t) tangents. */
    	free((char *) m);
    	free((char *) r);
	int_error("Cannt interpolate Y using complete splines", NO_CARET);
    }
    free((char *) m);
    free((char *) r);
}

/*
 * Solve tri diagonal linear system equation. The tri diagonal matrix is
 * defined via matrix M, right side is r, and solution X i.e. M * X = R.
 * Size of system given in n. Return TRUE if solution exist.
 */
static int solve_tri_diag(m, r, x, n)
tri_diag m[];
double r[], x[];
int n;
{
    int i;
    double t;

    for (i=1; i<n; i++) {   /* Eliminate element m[i][i-1] (lower diagonal). */
	if (m[i-1][1] == 0) return FALSE;
	t = m[i][0] / m[i-1][1];        /* Find ratio between the two lines. */
	m[i][0] = m[i][0] - m[i-1][1] * t;
	m[i][1] = m[i][1] - m[i-1][2] * t;
	r[i] = r[i] - r[i-1] * t;
    }
    /* Now do back subtitution - update the solution vector X: */
    if (m[n-1][1] == 0) return FALSE;
    x[n-1] = r[n-1] / m[n-1][1];		       /* Find last element. */
    for (i=n-2; i>=0; i--) {
	if (m[i][1] == 0) return FALSE;
	x[i] = (r[i] - x[i+1] * m[i][2]) / m[i][1];
    }
    return TRUE;
}

/*
 * Generate a Bspline curve defined by all the points given in linked list p:
 * Algorithm: using deBoor algorithm
 * Note: if Curvekind is OPEN_CONTOUR than Open end knot vector is assumed,
 *       else (CLOSED_CONTOUR) Float end knot vector is assumed.
 * It is assumed that num_of_points is at list 2, and order of Bspline is less
 * than num_of_points!
 */
static gen_bspline_approx(p_cntr, num_of_points, order, contour_kind)
struct cntr_struct *p_cntr;
int num_of_points, order, contour_kind;
{
    int i, knot_index = 0, pts_count = 1;
    double dt, t, next_t, t_min, t_max, x, y;
    struct cntr_struct *pc_temp = p_cntr, *pc_tail;

    /* If the contour is Closed one we must update few things:               */
    /* 1. Make the list temporary circular, so we can close the contour.     */
    /* 2. Update num_of_points - increase it by "order-1" so contour will be */
    /*    closed. This will evaluate order more sections to close it!        */
    if (contour_kind == CLOSED_CONTOUR) {
	pc_tail = p_cntr;
	while (pc_tail -> next) pc_tail = pc_tail -> next;/* Find last point.*/
	pc_tail -> next = p_cntr;   /* Close contour list - make it circular.*/
	num_of_points += order;
    }

    /* Find first (t_min) and last (t_max) t value to eval: */
    t = t_min = fetch_knot(contour_kind, num_of_points, order, order);
    t_max = fetch_knot(contour_kind, num_of_points, order, num_of_points);
    next_t = t_min + 1.0;
    knot_index = order;
    dt = 1.0/num_approx_pts;            /* Number of points per one section. */


    while (t<t_max) {
	if (t > next_t) {
	    pc_temp = pc_temp -> next;     /* Next order ctrl. pt. to blend. */
            knot_index++;
	    next_t += 1.0;
	}
        eval_bspline(t, pc_temp, num_of_points, order, knot_index,
    					contour_kind, &x, &y);   /* Next pt. */
	add_cntr_point(x, y);
	pts_count++;
	/* As we might have some real number round off problems we must      */
	/* test if we dont produce too many points here...                   */
	if (pts_count + 1 == num_approx_pts * (num_of_points - order) + 1)
    		break;
        t += dt;
    }

    eval_bspline(t_max - EPSILON, pc_temp, num_of_points, order, knot_index,
		contour_kind, &x, &y);
    /* If from round off errors we need more than one last point: */
    for (i=pts_count; i<num_approx_pts * (num_of_points - order) + 1; i++)
	add_cntr_point(x, y);			    /* Complete the contour. */

    if (contour_kind == CLOSED_CONTOUR)     /* Update list - un-circular it. */
	pc_tail -> next = NULL;
}

/*
 * The recursive routine to evaluate the B-spline value at point t using
 * knot vector PKList, and the control points Pdtemp. Returns x, y after the
 * division by the weight w. Note that Pdtemp points on the first control
 * point to blend with. The B-spline is of order order.
 */
static eval_bspline(t, p_cntr, num_of_points, order, j, contour_kind, x, y)
double t;
struct cntr_struct *p_cntr;
int num_of_points, order, j, contour_kind;
double *x, *y;
{
    int i, p;
    double ti, tikp, *dx, *dy;      /* Copy p_cntr into it to make it faster. */

    dx = (double *) alloc((unsigned) (sizeof(double) * (order + j)),
						"contour b_spline");
    dy = (double *) alloc((unsigned) (sizeof(double) * (order + j)),
						"contour b_spline");
    /* Set the dx/dy - [0] iteration step, control points (p==0 iterat.): */
    for (i=j-order; i<=j; i++) {
        dx[i] = p_cntr -> X;
        dy[i] = p_cntr -> Y;
        p_cntr = p_cntr -> next;
    }

    for (p=1; p<=order; p++) {        /* Iteration (b-spline level) counter. */
	for (i=j; i>=j-order+p; i--) {           /* Control points indexing. */
            ti = fetch_knot(contour_kind, num_of_points, order, i);
            tikp = fetch_knot(contour_kind, num_of_points, order, i+order+1-p);
	    if (ti == tikp) {   /* Should not be a problems but how knows... */
	    }
	    else {
		dx[i] = dx[i] * (t - ti)/(tikp-ti) +         /* Calculate x. */
			dx[i-1] * (tikp-t)/(tikp-ti);
		dy[i] = dy[i] * (t - ti)/(tikp-ti) +         /* Calculate y. */
			dy[i-1] * (tikp-t)/(tikp-ti);
	    }
	}
    }
    *x = dx[j]; *y = dy[j];
    free((char *) dx);
    free((char *) dy);
}

/*
 * Routine to get the i knot from uniform knot vector. The knot vector
 * might be float (Knot(i) = i) or open (where the first and last "order"
 * knots are equal). contour_kind determines knot kind - OPEN_CONTOUR means
 * open knot vector, and CLOSED_CONTOUR selects float knot vector.
 * Note the knot vector is not exist and this routine simulates it existance
 * Also note the indexes for the knot vector starts from 0.
 */
static double fetch_knot(contour_kind, num_of_points, order, i)
int contour_kind, num_of_points, order, i;
{
    switch (contour_kind) {
	case OPEN_CONTOUR:
	    if (i <= order) return 0.0;
	    else if (i <= num_of_points) return (double) (i - order);
		 else return (double) (num_of_points - order);
	case CLOSED_CONTOUR:
	    return (double) i;
	default: /* Should never happen */
	    return 1.0;
    }
}
