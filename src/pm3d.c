/* GNUPLOT - pm3d.c */

/*[
 *
 * Petr Mikulik, December 1998 -- November 1999
 * Copyright: open source as much as possible
 *
 * 
 * What is here: global variables and routines for the pm3d splotting mode
 * This file is included only if PM3D is defined
 *
]*/

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#ifdef PM3D

#include "plot.h"
#include "graph3d.h"
#include "pm3d.h"
#include "setshow.h" /* for surface_rot_z */
#include "term_api.h" /* for lp_use_properties() */

#include "hidden3d.h" /* p_vertex & map3d_xyz() */


/********************************************************************/

/*
  Global options for pm3d algorithm (to be accessed by set / show)
*/

pm3d_struct pm3d = {
    "",                   /* where[6] */
    PM3D_FLUSH_BEGIN,     /* flush */
    PM3D_SCANS_AUTOMATIC, /* scans direction is determined automatically */
    PM3D_CLIP_1IN,        /* clipping: at least 1 point in the ranges */
    0, 0,                 /* use zmin, zmax from `set zrange` */
    0.0, 100.0,           /* pm3d's zmin, zmax */
    0,                    /* no pm3d hidden3d is drawn */
    0,                    /* solid (off by default, that means `transparent') */
};



/* global variables */
double used_pm3d_zmin, used_pm3d_zmax;

/* trick for rotating the ylabel in 'set pm3d map' */
int pm3d_map_rotate_ylabel = 0;

/****************************************************************/
/* Now the routines which are really those exactly for pm3d.c
*/

/* declare variables and routines from external files */
extern struct surface_points *first_3dplot;


extern double min_array[], max_array[];


/*
   Check and set the z-range for use by pm3d
   Return 0 on wrong range, otherwise 1
 */
int set_pm3d_zminmax ()
{
    extern double log_base_array[];
    extern TBOOLEAN is_log_z;
    if (!pm3d.pm3d_zmin)
	used_pm3d_zmin = min_array[FIRST_Z_AXIS];
    else {
	used_pm3d_zmin = pm3d.zmin;
	if (is_log_z) {
	    if (used_pm3d_zmin<0) {
		fprintf(stderr,"pm3d: log of negative z-min?\n");
		return 0;
	    }
	    used_pm3d_zmin = log(used_pm3d_zmin)/log_base_array[FIRST_Z_AXIS];
	}
    }
    if (!pm3d.pm3d_zmax)
	used_pm3d_zmax = max_array[FIRST_Z_AXIS];
    else {
	used_pm3d_zmax = pm3d.zmax;
	if (is_log_z) {
	    if (used_pm3d_zmax<0) {
		fprintf(stderr,"p3md: log of negative z-max?\n");
		return 0;
	    }
	    used_pm3d_zmax = log(used_pm3d_zmax)/log_base_array[FIRST_Z_AXIS];
	}
    }
    if (used_pm3d_zmin == used_pm3d_zmax) {
	fprintf(stderr,"pm3d: colouring requires not equal zmin and zmax\n");
	return 0;
    }
    if (used_pm3d_zmin > used_pm3d_zmax) { /* exchange min and max values */
	double tmp = used_pm3d_zmax;
	used_pm3d_zmax = used_pm3d_zmin;
	used_pm3d_zmin = tmp;
    }
    return 1;
}


/*
   Rescale z into the interval [0,1]. It's OK also for logarithmic z axis too
 */
double z2gray ( double z )
{
    if ( z <= used_pm3d_zmin ) return 0;
    if ( z >= used_pm3d_zmax ) return 1;
    z = ( z - used_pm3d_zmin ) / ( used_pm3d_zmax - used_pm3d_zmin );
    return z;
}

static void
pm3d_rearrange_part(struct iso_curve* src, const int len, struct iso_curve*** dest, int* invert)
{
    struct iso_curve* scanA;
    struct iso_curve* scanB;
    struct iso_curve **scan_array;
    int i, scan;
    int invert_order = 0;

    scanA = src;

    /* loop over scans in one surface
       Scans are linked from this_plot->iso_crvs in the opposite order than
       they are in the datafile.
       Therefore it is necessary to make vector scan_array of iso_curves.
       Scans are sorted in scan_array according to pm3d.direction (this can
       be PM3D_SCANS_FORWARD or PM3D_SCANS_BACKWARD).
     */
    scan_array = *dest = malloc(len * sizeof(scanA));

    if (pm3d.direction == PM3D_SCANS_AUTOMATIC) {
	int cnt;
	if (scanA && (cnt = scanA->p_count - 1) > 0) {

	    vertex vA, vA2;

	    /* ordering within one scan */
	    map3d_xyz(scanA->points[0].x, scanA->points[0].y, scanA->points[0].z, &vA);
	    map3d_xyz(scanA->points[cnt].x, scanA->points[cnt].y, scanA->points[cnt].z, &vA2);
	    if (vA2.z > vA.z)
		*invert = 0;
	    else
		*invert = 1;
	    scanB = scanA->next;

	    /* check the z ordering between scans */
	    /* find last scan */
	    for (scanB = scanA->next, i = len - 2; i; i--)
		scanB = scanB->next;
	    if (scanB && scanB->p_count) {
		vertex vB;
		map3d_xyz(scanB->points[0].x, scanB->points[0].y, scanB->points[0].z, &vB);
		if (vB.z > vA.z)
		    invert_order = 0;
		else
		    invert_order = 1;
	    }
	}
    }

    for (scan = len - 1, i = 0; scan >= 0; --scan, i++) {
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
	}
	else if (pm3d.direction == PM3D_SCANS_FORWARD)
	    scan_array[scan] = scanA;
	else /* PM3D_SCANS_BACKWARD: i counts scans */
	    scan_array[i] = scanA;
	scanA = scanA->next;
    }
}

/* allocates *first_ptr (and eventually *second_ptr)
 * which must be freed by the caller */
void
pm3d_rearrange_scan_array(struct surface_points* this_plot,
    struct iso_curve*** first_ptr, int* first_n, int* first_invert,
    struct iso_curve*** second_ptr, int* second_n, int* second_invert)
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
	    *second_ptr = (struct iso_curve**)0;
	}
    }
}



/*
   Now the implementation of the pm3d (s)plotting mode
 */

void
pm3d_plot(struct surface_points* this_plot, char at_which_z)
{
    int i, j, ii, from, curve, scan, up_to, up_to_minus, invert = 0;
    struct iso_curve *scanA, *scanB;
    struct coordinate GPHUGE *pointsA, *pointsB;
    struct iso_curve **scan_array;
    int scan_array_n;
    double avgZ, gray;
    gpdPoint corners[4];
    extern double base_z, ceiling_z; /* defined in graph3d.c */
#ifdef EXTENDED_COLOR_SPECS
    gpiPoint icorners[4];
#endif

    if (this_plot == NULL)
	return;

    if (at_which_z != PM3D_AT_BASE && at_which_z != PM3D_AT_TOP && at_which_z != PM3D_AT_SURFACE)
	return;

    /* return if the terminal does not support filled polygons */
    if (!term->filled_polygon) return;

    switch (at_which_z) {
	case PM3D_AT_BASE:
	    corners[0].z = corners[1].z = corners[2].z = corners[3].z = base_z;
	    break;
	case PM3D_AT_TOP:
	    corners[0].z = corners[1].z = corners[2].z = corners[3].z = ceiling_z;
	    break;
	    /* the 3rd possibility is surface, PM3D_AT_SURFACE, and it'll come later */
    }

    scanA = this_plot->iso_crvs;
    curve = 0;

    pm3d_rearrange_scan_array(this_plot,
	&scan_array, &scan_array_n, &invert,
	(struct iso_curve***)0, (int*)0, (int*)0);
    /* pm3d_rearrange_scan_array(this_plot, (struct iso_curve***)0, (int*)0, &scan_array, &invert); */

#if 0
    /* debugging: print scan_array */
    for ( scan=0; scan<this_plot->num_iso_read; scan++ ) {
	printf("**** SCAN=%d  points=%d\n", scan, scan_array[scan]->p_count );
    }
#endif

#if 0
    /* debugging: this loop prints properties of all scans */
    for ( scan=0; scan<this_plot->num_iso_read; scan++ ) {
	struct coordinate GPHUGE *points;
	scanA = scan_array[scan];
	printf( "\n#IsoCurve = scan nb %d, %d points\n#x y z type(in,out,undef)\n", scan, scanA->p_count );
	for ( i = 0, points = scanA->points; i < scanA->p_count; i++ ) {
	    printf("%g %g %g %c\n",
		points[i].x, points[i].y, points[i].z,
		points[i].type == INRANGE ? 'i' : points[i].type == OUTRANGE ? 'o' : 'u');
	    /* Note: INRANGE, OUTRANGE, UNDEFINED */
	}
    }
    printf("\n");
#endif


    /*
       this loop does the pm3d draw of joining two curves

       How the loop below works:
     * scanB = scan last read; scanA = the previous one
     * link the scan from A to B, then move B to A, then read B, then draw
     */
    for ( scan=0; scan<this_plot->num_iso_read-1; scan++ ) {
	scanA = scan_array[scan];
	scanB = scan_array[scan+1];
#if 0
	printf( "\n#IsoCurveA = scan nb %d has %d points   ScanB has %d points\n", scan, scanA->p_count, scanB->p_count );
#endif
	pointsA = scanA->points; pointsB = scanB->points;
	/* if the number of points in both scans is not the same, then the starting
	   index (offset) of scan B according to the flushing setting has to be
	   determined
	 */
	from = 0; /* default is pm3d.flush==PM3D_FLUSH_BEGIN */
	if (pm3d.flush == PM3D_FLUSH_END)
	    from = abs( scanA->p_count - scanB->p_count );
	else if (pm3d.flush == PM3D_FLUSH_CENTER)
	    from = abs( scanA->p_count - scanB->p_count ) / 2;
	/* find the minimal number of points in both scans */
	up_to = GPMIN(scanA->p_count,scanB->p_count) - 1;
	/* go over the minimal number of points from both scans.
Notice: if it would be once necessary to go over points in `backward'
direction, then the loop body below would require to replace the data
point indices `i' by `up_to-i' and `i+1' by `up_to-i-1'.
	 */
	up_to_minus = up_to - 1; /* calculate only once */
	for ( j = 0; j < up_to; j++ ) {
	    i = j;
	    if (PM3D_SCANS_AUTOMATIC == pm3d.direction && invert) {
		i = up_to_minus - j;
	    }
	    ii = i + from; /* index to the B array */
	    /* choose the clipping method */
	    if (pm3d.clip == PM3D_CLIP_4IN) {
		/* (1) all 4 points of the quadrangle must be in x and y range */
		if (!( pointsA[i].type == INRANGE && pointsA[i+1].type == INRANGE &&
			pointsB[ii].type == INRANGE && pointsB[ii+1].type == INRANGE ))
		    continue;
	    }
	    else { /* (pm3d.clip == PM3D_CLIP_1IN) */
		/* (2) all 4 points of the quadrangle must be defined */
		if ( pointsA[i].type == UNDEFINED || pointsA[i+1].type == UNDEFINED ||
		    pointsB[ii].type == UNDEFINED || pointsB[ii+1].type == UNDEFINED )
		    continue;
		/* and at least 1 point of the quadrangle must be in x and y range */
		if ( pointsA[i].type == OUTRANGE && pointsA[i+1].type == OUTRANGE &&
		    pointsB[ii].type == OUTRANGE && pointsB[ii+1].type == OUTRANGE )
		    continue;
	    }
#ifdef EXTENDED_COLOR_SPECS
	    if (!supply_extended_color_specs) {
#endif
	    /* get the gray as the average of the corner z positions (note: log already in)
	       I always wonder what is faster: d*0.25 or d/4? Someone knows? -- 0.25 (joze) */
	    avgZ = ( pointsA[i].z + pointsA[i+1].z + pointsB[ii].z + pointsB[ii+1].z ) * 0.25;
	    /* transform z value to gray, i.e. to interval [0,1] */
	    gray = z2gray ( avgZ );
	    /* print the quadrangle with the given colour */
#if 0
	    printf( "averageZ %g\tgray=%g\tM %g %g L %g %g L %g %g L %g %g\n",
		avgZ,
		gray,
		pointsA[i].x, pointsA[i].y,
		pointsB[ii].x, pointsB[ii].y,
		pointsB[ii+1].x, pointsB[ii+1].y,
		pointsA[i+1].x, pointsA[i+1].y );
#endif
	    set_color( gray );
#ifdef EXTENDED_COLOR_SPECS
	    }
#endif
	    corners[0].x = pointsA[i].x;    corners[0].y = pointsA[i].y;
	    corners[1].x = pointsB[ii].x;   corners[1].y = pointsB[ii].y;
	    corners[2].x = pointsB[ii+1].x; corners[2].y = pointsB[ii+1].y;
	    corners[3].x = pointsA[i+1].x;  corners[3].y = pointsA[i+1].y;

	    if (at_which_z == PM3D_AT_SURFACE) {
		/* always supply the z value if
		 * EXTENDED_COLOR_SPECS is defined
		 */
		corners[0].z = pointsA[i].z;
		corners[1].z = pointsB[ii].z;
		corners[2].z = pointsB[ii+1].z;
		corners[3].z = pointsA[i+1].z;
	    }

#ifdef EXTENDED_COLOR_SPECS
	    if (supply_extended_color_specs) {
		/* the target wants z and gray value */
		icorners[0].z = pointsA[i].z;
		icorners[1].z = pointsB[ii].z;
		icorners[2].z = pointsB[ii+1].z;
		icorners[3].z = pointsA[i+1].z;
		for (i = 0; i < 4; i++) {
		    icorners[i].spec.gray = z2gray(icorners[i].z);
		}
	    }
	    filled_quadrangle(corners, icorners);
#else
	    /* filled_polygon( 4, corners ); */
	    filled_quadrangle( corners );
#endif
	} /* loop over points of two subsequent scans */
    } /* loop over scans */
    /* printf("\n"); */

    /* free memory allocated by scan_array */
    free( scan_array );

} /* end of pm3d splotting mode */



/*
   Now the implementation of the filled colour contour plot

contours_where: equals either CONTOUR_SRF or CONTOUR_BASE

Note: z2gray() uses used_pm3d_zmin, used_pm3d_zmax
Check that if accessing this routine otherwise then via `set pm3d at`
code block in graph3d.c
 */

void filled_color_contour_plot ( this_plot, contours_where )
    struct surface_points *this_plot;
    int contours_where;
{
    double gray;
    extern double base_z; /* defined in graph3d.c */
    struct gnuplot_contours *cntr;

    if (this_plot == NULL || this_plot->contours == NULL)
	return;
    if (contours_where != CONTOUR_SRF && contours_where != CONTOUR_BASE)
	return;

    /* return if the terminal does not support filled polygons */
    if (!term->filled_polygon) return;

    /* TODO: CHECK FOR NUMBER OF POINTS IN CONTOUR: IF TOO SMALL, THEN IGNORE! */
    cntr = this_plot->contours;
    while (cntr) {
	printf("# Contour: points %i, z %g, label: %s\n", cntr->num_pts, cntr->coords[0].z, (cntr->label)?cntr->label:"<no>");
	if (cntr->isNewLevel) {
	    printf("\t...it isNewLevel\n");
	    /* contour split across chunks */
	    /* fprintf(gpoutfile, "\n# Contour %d, label: %s\n", number++, c->label); */
	    /* What is the colour? */
	    /* get the z-coordinate */
	    /* transform contour z-coordinate value to gray, i.e. to interval [0,1] */
	    gray = z2gray(cntr->coords[0].z);
	    set_color(gray);
	}
	/* draw one countour */
	if (contours_where == CONTOUR_SRF) /* at CONTOUR_SRF */
	    filled_polygon_3dcoords ( cntr->num_pts, cntr->coords );
	else /* at CONTOUR_BASE */
	    filled_polygon_3dcoords_zfixed ( cntr->num_pts, cntr->coords, base_z );
	/* next contour */
	cntr = cntr->next;
    }
} /* end of filled colour contour plot splot mode */

void
pm3d_reset(void)
{
    pm3d.where[0] = 0;
    pm3d.flush = PM3D_FLUSH_BEGIN;
    pm3d.direction = PM3D_SCANS_AUTOMATIC;
    pm3d.clip = PM3D_CLIP_1IN;
    pm3d.pm3d_zmin = 0;
    pm3d.pm3d_zmax = 0;
    pm3d.zmin = 0.0;
    pm3d.zmax = 100.0;
    pm3d.hidden3d_tag = 0;
    pm3d.solid = 0;
}

/* DRAW PM3D ALL COLOUR SURFACES */
void pm3d_draw_all(struct surface_points* plots, int pcount)
{
    int i = 0;
    int surface;
    extern FILE *gpoutfile;
    struct surface_points *this_plot = NULL;

    /* for pm3dCompress.awk */
    if (!strcmp(term->name,"postscript") || 
	!strcmp(term->name,"pslatex") || !strcmp(term->name,"pstex")) {
	extern FILE *PSLATEX_auxfile;
	fprintf(PSLATEX_auxfile ? PSLATEX_auxfile : gpoutfile,"%%pm3d_map_begin\n");
    }

    for ( ; pm3d.where[i]; i++ ) {
	this_plot = plots;
	for (surface = 0;
	    surface < pcount;
	    this_plot = this_plot->next_sp, surface++)
	    pm3d_plot(this_plot, pm3d.where[i]);
    }

    if (strchr(pm3d.where,'C') != NULL)
	/* !!!!! CONTOURS, UNDOCUMENTED
	   !!!!! LATER CHANGE TO STH LIKE (if_filled_contours_requested)
	   !!!!! ... */
	for (this_plot = plots; this_plot; this_plot = this_plot->next_sp) {
	    if (draw_contour & CONTOUR_SRF)
		filled_color_contour_plot(this_plot, CONTOUR_SRF);
	    if (draw_contour & CONTOUR_BASE)
		filled_color_contour_plot(this_plot, CONTOUR_BASE);
	}

    /* for pm3dCompress.awk */
    if (!strcmp(term->name,"postscript") ||
	!strcmp(term->name,"pslatex") || !strcmp(term->name,"pstex")) {
	extern FILE *PSLATEX_auxfile;
	fprintf(PSLATEX_auxfile ? PSLATEX_auxfile : gpoutfile,"%%pm3d_map_end\n");
    }

    /* release the palette we have made use of (some terminals may need this)
       ...no, remove this, also remove it from plot.h !!!!
     */
    if (term->previous_palette)
	term->previous_palette();
}

/* eof pm3d.c */

#endif
