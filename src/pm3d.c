#ifndef lint
static char *RCSid() { return RCSid("$Id: pm3d.c,v 1.29 2002/03/09 22:41:45 mikulik Exp $"); }
#endif

/* GNUPLOT - pm3d.c */

/*[
 *
 * Petr Mikulik, since December 1998
 * Copyright: open source as much as possible
 *
 * What is here: global variables and routines for the pm3d splotting mode.
 * This file is included only if PM3D is defined.
 *
]*/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef PM3D

#include "pm3d.h"

#include "alloc.h"
#include "axis.h"
#include "graph3d.h"
#include "hidden3d.h"		/* p_vertex & map3d_xyz() */
#include "plot3d.h"
#include "setshow.h"		/* for surface_rot_z */
#include "term_api.h"		/* for lp_use_properties() */
#include "command.h"		/* for c_token */


/*
  Global options for pm3d algorithm (to be accessed by set / show).
*/

pm3d_struct pm3d = {
    "",				/* where[6] */
    0,				/* no map */
    PM3D_FLUSH_BEGIN,		/* flush */
    0,				/* no flushing triangles */
    PM3D_SCANS_AUTOMATIC,	/* scans direction is determined automatically */
    PM3D_CLIP_1IN,		/* clipping: at least 1 point in the ranges */
    0,				/* no pm3d hidden3d is drawn */
    0,				/* solid (off by default, that means `transparent') */
    PM3D_IMPLICIT,		/* implicit */
    0 				/* use_column */
};

/* Internal prototypes for this module */
static void pm3d_plot __PROTO((struct surface_points *, char));
static void pm3d_option_at_error __PROTO((void));
static void pm3d_rearrange_part __PROTO((struct iso_curve *, const int, struct iso_curve ***, int *));
static void filled_color_contour_plot  __PROTO((struct surface_points *, int));


/*
* Now the routines which are really just those for pm3d.c
*/

/*
 * Rescale z to cb values. Nothing to do if both z and cb are linear or log of the
 * same base, other it has to un-log z and subsequently log it again.
 */
double
z2cb(double z)
{
    if (!Z_AXIS.log && !CB_AXIS.log) /* both are linear */
	return z;
    if (Z_AXIS.log && !CB_AXIS.log) /* log z, linear cb */
	return exp(z * Z_AXIS.log_base); /* unlog(z) */
    if (!Z_AXIS.log && CB_AXIS.log) /* linear z, log cb */
	return (log(z) / CB_AXIS.log_base);
    /* both are log */
    if (Z_AXIS.base==CB_AXIS.base) /* can we compare double numbers like that? */
	return z;
    return z * Z_AXIS.log_base / CB_AXIS.log_base; /* log_cb(unlog_z(z)) */
}


/*
 * Rescale cb value into the interval [0,1].
 * Note that it is OK for logarithmic cb-axis too.
 */
double
cb2gray(double cb)
{
    if (cb <= CB_AXIS.min)
	return 0;
    if (cb >= CB_AXIS.max)
	return 1;
    cb = (cb - CB_AXIS.min)
      / (CB_AXIS.max - CB_AXIS.min);
    return cb;
}


/*
 * Rearrange...
 */
static void
pm3d_rearrange_part(src, len, dest, invert)
    struct iso_curve *src;
    const int len;
    struct iso_curve ***dest;
    int *invert;
{
    struct iso_curve *scanA;
    struct iso_curve *scanB;
    struct iso_curve **scan_array;
    int i, scan;
    int invert_order = 0;

    /* loop over scans in one surface
       Scans are linked from this_plot->iso_crvs in the opposite order than
       they are in the datafile.
       Therefore it is necessary to make vector scan_array of iso_curves.
       Scans are sorted in scan_array according to pm3d.direction (this can
       be PM3D_SCANS_FORWARD or PM3D_SCANS_BACKWARD).
     */
    scan_array = *dest = gp_alloc(len * sizeof(scanA), "pm3d scan array");

    if (pm3d.direction == PM3D_SCANS_AUTOMATIC) {
	int cnt;
	int len2 = len;
	for (scanA = src; scanA && (cnt = scanA->p_count - 1) > 0; scanA = scanA->next, len2--) {

	    int from, i;
	    TBOOLEAN exit_outer_loop = 0;
	    vertex vA, vA2;

	    /* ordering within one scan */
	    for (from=0; from<=cnt; from++) /* find 1st non-undefined point */
		if (scanA->points[from].type != UNDEFINED) {
		    map3d_xyz(scanA->points[from].x, scanA->points[from].y, scanA->points[from].z, &vA);
		    break;
		}
	    for (i=cnt; i>from; i--) /* find the last non-undefined point */
		if (scanA->points[from].type != UNDEFINED) {
		    map3d_xyz(scanA->points[i].x, scanA->points[i].y, scanA->points[i].z, &vA2);
		    break;
		}
	    if (from < i) 
		*invert = (vA2.z > vA.z) ? 0 : 1;
	    else
		continue; /* all points were undefined, so check next scan */


	    /* check the z ordering between scans
	     * Find last scan. If this scan has all points undefined,
	     * find last but one scan, an so on. */

	    for (; len2 >= 3 && !exit_outer_loop; len2--) {
		for (scanB = scanA-> next, i = len2 - 2; i && scanB; i--)
		    scanB = scanB->next; /* skip over to last scan */
		if (scanB && scanB->p_count) {
		    vertex vB;
		    for (i=0; i<scanB->p_count; i++) { /* find 1st non-undefined point */
			if (scanB->points[i].type != UNDEFINED) {
			    map3d_xyz(scanB->points[i].x, scanB->points[i].y, scanB->points[i].z, &vB);
			    if (from<=cnt) {
				invert_order = (vB.z > vA.z) ? 0 : 1;
			    }
			    exit_outer_loop = 1;
			    break;
			}
		    }
		}
	    }
	}
    }

    for (scanA = src, scan = len - 1, i = 0; scan >= 0; --scan, i++) {
	if (pm3d.direction == PM3D_SCANS_AUTOMATIC) {
	    switch (invert_order) {
	    case 1:
		scan_array[scan] = scanA;
		break;
	    case 0:
	    default:
		scan_array[i] = scanA;
		break;
	    }
	} else if (pm3d.direction == PM3D_SCANS_FORWARD)
	    scan_array[scan] = scanA;
	else			/* PM3D_SCANS_BACKWARD: i counts scans */
	    scan_array[i] = scanA;
	scanA = scanA->next;
    }
}


/*
 * Rearrange scan array
 * 
 * Allocates *first_ptr (and eventually *second_ptr)
 * which must be freed by the caller
 */
void
pm3d_rearrange_scan_array(struct surface_points *this_plot,
			  struct iso_curve ***first_ptr, int *first_n, int *first_invert,
			  struct iso_curve ***second_ptr, int *second_n, int *second_invert)
{
    if (first_ptr) {
	pm3d_rearrange_part(this_plot->iso_crvs, this_plot->num_iso_read, first_ptr, first_invert);
	*first_n = this_plot->num_iso_read;
    }
    if (second_ptr) {
	struct iso_curve *icrvs = this_plot->iso_crvs;
	struct iso_curve *icrvs2;
	int i;
	/* advance until second part */
	for (i = 0; i < this_plot->num_iso_read; i++)
	    icrvs = icrvs->next;
	/* count the number of scans of second part */
	for (i = 0, icrvs2 = icrvs; icrvs2; icrvs2 = icrvs2->next)
	    i++;
	if (i > 0) {
	    *second_n = i;
	    pm3d_rearrange_part(icrvs, i, second_ptr, second_invert);
	} else {
	    *second_ptr = (struct iso_curve **) 0;
	}
    }
}


/*
 * Now the implementation of the pm3d (s)plotting mode
 */
static void
pm3d_plot(this_plot, at_which_z)
    struct surface_points *this_plot;
    char at_which_z;
{
    int j, i, i1, ii, ii1, from, curve, scan, up_to, up_to_minus, invert = 0;
    int go_over_pts, max_pts;
    int are_ftriangles, ftriangles_low_pt, ftriangles_high_pt;
    struct iso_curve *scanA, *scanB;
    struct coordinate GPHUGE *pointsA, *pointsB;
    struct iso_curve **scan_array;
    int scan_array_n;
    double avgC, gray;
    gpdPoint corners[4];
#ifdef EXTENDED_COLOR_SPECS
    gpiPoint icorners[4];
#endif

    /* just a shortcut */
    int color_from_column = this_plot->pm3d_color_from_column;

    if (this_plot == NULL)
	return;

    if (at_which_z != PM3D_AT_BASE && at_which_z != PM3D_AT_TOP && at_which_z != PM3D_AT_SURFACE)
	return;

    /* return if the terminal does not support filled polygons */
    if (!term->filled_polygon)
	return;

    switch (at_which_z) {
    case PM3D_AT_BASE:
	corners[0].z = corners[1].z = corners[2].z = corners[3].z = base_z;
	break;
    case PM3D_AT_TOP:
	corners[0].z = corners[1].z = corners[2].z = corners[3].z = ceiling_z;
	break;
	/* the 3rd possibility is surface, PM3D_AT_SURFACE, coded below */
    }

    scanA = this_plot->iso_crvs;
    curve = 0;

    pm3d_rearrange_scan_array(this_plot, &scan_array, &scan_array_n, &invert, (struct iso_curve ***) 0, (int *) 0, (int *) 0);
    /* pm3d_rearrange_scan_array(this_plot, (struct iso_curve***)0, (int*)0, &scan_array, &invert); */

#if 0
    /* debugging: print scan_array */
    for (scan = 0; scan < this_plot->num_iso_read; scan++) {
	printf("**** SCAN=%d  points=%d\n", scan, scan_array[scan]->p_count);
    }
#endif

#if 0
    /* debugging: this loop prints properties of all scans */
    for (scan = 0; scan < this_plot->num_iso_read; scan++) {
	struct coordinate GPHUGE *points;
	scanA = scan_array[scan];
	printf("\n#IsoCurve = scan nb %d, %d points\n#x y z type(in,out,undef)\n", scan, scanA->p_count);
	for (i = 0, points = scanA->points; i < scanA->p_count; i++) {
	    printf("%g %g %g %c\n",
		   points[i].x, points[i].y, points[i].z, points[i].type == INRANGE ? 'i' : points[i].type == OUTRANGE ? 'o' : 'u');
	    /* Note: INRANGE, OUTRANGE, UNDEFINED */
	}
    }
    printf("\n");
#endif


    /*
     * this loop does the pm3d draw of joining two curves
     *
     * How the loop below works:
     * - scanB = scan last read; scanA = the previous one
     * - link the scan from A to B, then move B to A, then read B, then draw
     */
    for (scan = 0; scan < this_plot->num_iso_read - 1; scan++) {
	scanA = scan_array[scan];
	scanB = scan_array[scan + 1];
#if 0
	printf("\n#IsoCurveA = scan nb %d has %d points   ScanB has %d points\n", scan, scanA->p_count, scanB->p_count);
#endif
	pointsA = scanA->points;
	pointsB = scanB->points;
	/* if the number of points in both scans is not the same, then the
	 * starting index (offset) of scan B according to the flushing setting
	 * has to be determined
	 */
	from = 0;		/* default is pm3d.flush==PM3D_FLUSH_BEGIN */
	if (pm3d.flush == PM3D_FLUSH_END)
	    from = abs(scanA->p_count - scanB->p_count);
	else if (pm3d.flush == PM3D_FLUSH_CENTER)
	    from = abs(scanA->p_count - scanB->p_count) / 2;
	/* find the minimal number of points in both scans */
	up_to = GPMIN(scanA->p_count, scanB->p_count) - 1;
	up_to_minus = up_to - 1; /* calculate only once */
	are_ftriangles = pm3d.ftriangles && (scanA->p_count != scanB->p_count);
	if (!are_ftriangles)
	    go_over_pts = up_to;
	else {
	    max_pts = GPMAX(scanA->p_count, scanB->p_count);
	    go_over_pts = max_pts - 1;
	    /* the j-subrange of quadrangles; in the remaing of the interval 
	     * [0..up_to] the flushing triangles are to be drawn */
	    ftriangles_low_pt = from;
	    ftriangles_high_pt = from + up_to_minus;
	}
	/* Go over 
	 *   - the minimal number of points from both scans, if only quadrangles.
	 *   - the maximal number of points from both scans if flush triangles
	 *     (the missing points in the scan of lower nb of points will be 
	 *     duplicated from the begin/end points).
	 *
	 * Notice: if it would be once necessary to go over points in `backward'
	 * direction, then the loop body below would require to replace the data
	 * point indices `i' by `up_to-i' and `i+1' by `up_to-i-1'.
	 */
	for (j = 0; j < go_over_pts; j++) {
	    /* Now i be the index of the scan with smaller number of points,
	     * ii of the scan with larger number of points. */
	    if (are_ftriangles && (j < ftriangles_low_pt || j > ftriangles_high_pt)) {
		i = (j <= ftriangles_low_pt) ? 0 : ftriangles_high_pt-from+1;
		ii = j;
		i1 = i;
		ii1 = ii + 1;
	    } else {
		int jj = are_ftriangles ? j - from : j;
		i = jj;
		if (PM3D_SCANS_AUTOMATIC == pm3d.direction && invert)
		    i = up_to_minus - jj;
		ii = i + from;
		i1 = i + 1;
		ii1 = ii + 1;
	    }
	    /* From here, i is index to scan A, ii to scan B */
	    if (scanA->p_count > scanB->p_count) {
	        int itmp = i;
		i = ii;
		ii = itmp;
		itmp = i1;
		i1 = ii1;
		ii1 = itmp;
	    }
#if 0
	    fprintf(stderr,"j=%i:  i=%i i1=%i  [%i]   ii=%i ii1=%i  [%i]\n",j,i,i1,scanA->p_count,ii,ii1,scanB->p_count);
#endif
	    /* choose the clipping method */
	    if (pm3d.clip == PM3D_CLIP_4IN) {
		/* (1) all 4 points of the quadrangle must be in x and y range */
		if (!(pointsA[i].type == INRANGE && pointsA[i1].type == INRANGE &&
		      pointsB[ii].type == INRANGE && pointsB[ii1].type == INRANGE))
		    continue;
	    } else {		/* (pm3d.clip == PM3D_CLIP_1IN) */
		/* (2) all 4 points of the quadrangle must be defined */
		if (pointsA[i].type == UNDEFINED || pointsA[i1].type == UNDEFINED ||
		    pointsB[ii].type == UNDEFINED || pointsB[ii1].type == UNDEFINED)
		    continue;
		/* and at least 1 point of the quadrangle must be in x and y range */
		if (pointsA[i].type == OUTRANGE && pointsA[i1].type == OUTRANGE &&
		    pointsB[ii].type == OUTRANGE && pointsB[ii1].type == OUTRANGE)
		    continue;
	    }
#ifdef EXTENDED_COLOR_SPECS
	    if (!supply_extended_color_specs) {
#endif
		/* get the gray as the average of the corner z positions (note: log already in)
		   I always wonder what is faster: d*0.25 or d/4? Someone knows? -- 0.25 (joze) */
		if (color_from_column)
		    /* ylow is set in plot3d.c:get_3ddata() */
		    avgC = (pointsA[i].ylow + pointsA[i1].ylow + pointsB[ii].ylow + pointsB[ii1].ylow) * 0.25;
		else
		    avgC = (z2cb(pointsA[i].z) + z2cb(pointsA[i1].z) + z2cb(pointsB[ii].z) + z2cb(pointsB[ii1].z)) * 0.25;
		/* transform z value to gray, i.e. to interval [0,1] */
		gray = cb2gray(avgC);
#if 0
		/* print the quadrangle with the given color */
		printf("averageColor %g\tgray=%g\tM %g %g L %g %g L %g %g L %g %g\n",
		       avgColor,
		       gray,
		       pointsA[i].x, pointsA[i].y, pointsB[ii].x, pointsB[ii].y,
		       pointsB[ii1].x, pointsB[ii1].y, pointsA[i1].x, pointsA[i1].y);
#endif
		/* set the color */
		set_color(gray);
#ifdef EXTENDED_COLOR_SPECS
	    }
#endif
	    corners[0].x = pointsA[i].x;
	    corners[0].y = pointsA[i].y;
	    corners[1].x = pointsB[ii].x;
	    corners[1].y = pointsB[ii].y;
	    corners[2].x = pointsB[ii1].x;
	    corners[2].y = pointsB[ii1].y;
	    corners[3].x = pointsA[i1].x;
	    corners[3].y = pointsA[i1].y;

	    if (at_which_z == PM3D_AT_SURFACE) {
		/* always supply the z value if
		 * EXTENDED_COLOR_SPECS is defined
		 */
		corners[0].z = pointsA[i].z;
		corners[1].z = pointsB[ii].z;
		corners[2].z = pointsB[ii1].z;
		corners[3].z = pointsA[i1].z;
	    }
#ifdef EXTENDED_COLOR_SPECS
	    if (supply_extended_color_specs) {
		if (color_from_column) {
		    icorners[0].z = pointsA[i].ylow;
		    icorners[1].z = pointsB[ii].ylow;
		    icorners[2].z = pointsB[ii1].ylow;
		    icorners[3].z = pointsA[i1].ylow;
		} else {
		    /* the target wants z and gray value */
		    icorners[0].z = pointsA[i].z;
		    icorners[1].z = pointsB[ii].z;
		    icorners[2].z = pointsB[ii1].z;
		    icorners[3].z = pointsA[i1].z;
		}
		for (i = 0; i < 4; i++) {
		    icorners[i].spec.gray = 
			cb2gray( color_from_column ? icorners[i].z : z2cb(icorners[i].z) );
		}
	    }
	    filled_quadrangle(corners, icorners);
#else
	    /* filled_polygon( 4, corners ); */
	    filled_quadrangle(corners);
#endif
	} /* loop quadrangles over points of two subsequent scans */
    } /* loop over scans */

    /* free memory allocated by scan_array */
    free(scan_array);

}				/* end of pm3d splotting mode */


/*
 *  Now the implementation of the filled color contour plot
*/
static void
filled_color_contour_plot(this_plot, contours_where)
    struct surface_points *this_plot;
    int contours_where;
{
    double gray;
    struct gnuplot_contours *cntr;

    /* just a shortcut */
    int color_from_column = this_plot->pm3d_color_from_column;

    if (this_plot == NULL || this_plot->contours == NULL)
	return;
    if (contours_where != CONTOUR_SRF && contours_where != CONTOUR_BASE)
	return;

    /* return if the terminal does not support filled polygons */
    if (!term->filled_polygon)
	return;

    /* TODO: CHECK FOR NUMBER OF POINTS IN CONTOUR: IF TOO SMALL, THEN IGNORE! */
    cntr = this_plot->contours;
    while (cntr) {
	printf("# Contour: points %i, z %g, label: %s\n", cntr->num_pts, cntr->coords[0].z, (cntr->label) ? cntr->label : "<no>");
	if (cntr->isNewLevel) {
	    printf("\t...it isNewLevel\n");
	    /* contour split across chunks */
	    /* fprintf(gpoutfile, "\n# Contour %d, label: %s\n", number++, c->label); */
	    /* What is the color? */
	    /* get the z-coordinate */
	    /* transform contour z-coordinate value to gray, i.e. to interval [0,1] */
	    if (color_from_column)
		gray = cb2gray(cntr->coords[0].ylow);
	    else
		gray = cb2gray( z2cb(cntr->coords[0].z) );
	    set_color(gray);
	}
	/* draw one countour */
	if (contours_where == CONTOUR_SRF)	/* at CONTOUR_SRF */
	    filled_polygon_3dcoords(cntr->num_pts, cntr->coords);
	else			/* at CONTOUR_BASE */
	    filled_polygon_3dcoords_zfixed(cntr->num_pts, cntr->coords, base_z);
	/* next contour */
	cntr = cntr->next;
    }
}				/* end of filled color contour plot splot mode */


/*
 * unset pm3d for the reset command
 */
void
pm3d_reset(void)
{
    pm3d.where[0] = 0;
    pm3d.map = 0;
    pm3d.flush = PM3D_FLUSH_BEGIN;
    pm3d.ftriangles = 0;
    pm3d.direction = PM3D_SCANS_AUTOMATIC;
    pm3d.clip = PM3D_CLIP_1IN;
    pm3d.hidden3d_tag = 0;
    pm3d.solid = 0;
    pm3d.implicit = PM3D_IMPLICIT;
}


/* 
 * Draw (one) PM3D color surface.
 */
void
pm3d_draw_one(struct surface_points *plot)
{
    int i = 0;
    char *where = plot->pm3d_where[0] ? plot->pm3d_where : pm3d.where;
	/* Draw either at 'where' option of the given surface or at pm3d.where
	 * global option. */

    if (!where[0]) {
	return;
    }

    /* for pm3dCompress.awk */
    if (postscript_gpoutfile)
	fprintf(gpoutfile, "%%pm3d_map_begin\n");

    for (; where[i]; i++) {
	pm3d_plot(plot, where[i]);
    }

    if (strchr(where, 'C') != NULL) {
	/* !!!!! FILLED COLOR CONTOURS, *UNDOCUMENTED*
	   !!!!! LATER CHANGE TO STH LIKE 
	   !!!!!   (if_filled_contours_requested)
	   !!!!!      ...
	   Currently filled color contours do not work because gnuplot generates
	   open contour lines, i.e. not closed on the graph boundary.
	 */
	if (draw_contour & CONTOUR_SRF)
	    filled_color_contour_plot(plot, CONTOUR_SRF);
	if (draw_contour & CONTOUR_BASE)
	    filled_color_contour_plot(plot, CONTOUR_BASE);
    }

    /* for pm3dCompress.awk */
    if (postscript_gpoutfile)
	fprintf(postscript_gpoutfile, "%%pm3d_map_end\n");

    /* release the palette we have made use of (some terminals may need this)
       ...no, remove this, also remove it from plot.h !!!!
     */
    if (term->previous_palette)
	term->previous_palette();
}


/* Display an error message for the routine get_pm3d_at_option() below.
 */

static void
pm3d_option_at_error(void)
{
    int_error(c_token,"parameter to `pm3d at` requires combination of up to 6 characters b,s,t\n\t(drawing at bottom, surface, top)");
}


/* Read the option for 'pm3d at' command.
 * Used by 'set pm3d at ...' or by 'splot ... with pm3d at ...'.
 * If no option given, then returns empty string, otherwise copied there.
 * The string is unchanged on error, and 1 is returned.
 * On success, 0 is returned.
 */
int
get_pm3d_at_option(char *pm3d_where)
{
    char* c;
    if (END_OF_COMMAND || token[c_token].length >= sizeof(pm3d.where)) {
	pm3d_option_at_error();
	return 1;
    }
    memcpy(pm3d_where, input_line + token[c_token].start_index, token[c_token].length);
    pm3d_where[ token[c_token].length ] = 0;
    /* verify the parameter */
    for (c = pm3d_where; *c; c++) {
	if (*c != 'C') /* !!!!! CONTOURS, UNDOCUMENTED, THIS LINE IS TEMPORARILY HERE !!!!! */
	    if (*c != PM3D_AT_BASE && *c != PM3D_AT_TOP && *c != PM3D_AT_SURFACE) {
		pm3d_option_at_error();
		return 1;
	}
    }
    c_token++;
    return 0;
}


#endif
