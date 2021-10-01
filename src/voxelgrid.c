/* GNUPLOT - voxelgrid.c */

/*[
 * Copyright Ethan A Merritt 2019
 *
 * Gnuplot license:
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
 *
 * Alternative license:
 *
 * As an alternative to distributing code in this file under the gnuplot license,
 * you may instead comply with the terms below. In this case, redistribution and
 * use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.  Redistributions in binary
 * form must reproduce the above copyright notice, this list of conditions and
 * the following disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
]*/

/*
 * This file implements a 3D grid of NxNxN evenly spaced voxels.
 * The command
 *	set vgrid size N
 * frees any previous grid, allocates a new one, and initializes it to zero.
 * The grid range can be set explicitly using
 *	set vxrange [vxmin:vxmax]
 *	set vyrange [vymin:vymax]
 *	set vzrange [vzmin:vzmax]
 * otherwise it inherits the xrange, yrange, and zrange limits present at the
 * time of the first "vclear", "vfill", or "voxel() = " command.
 *
 * Grid voxels can be filled individually using the command
 * 	voxel(x,y,z) = FOO
 * or filled in the neighborhood of a data point using the command
 * 	vfill $FILE using x:y:z:radius:(function)
 * In this case the nearest grid point and all other grid points within
 * a sphere centered about [x,y,z] are incremented by
 *	voxel(vx,vy,vz) += function( sqrt((x-vx)**2 + (y-vy)**2 + (z-vz)**2) )
 * Once loaded the grid can either be referenced by splot commands, e.g.
 * 	splot $FILE using x:y:z:(voxel(x,y,z)) with points lc palette
 * or plotted using new splot options
 * 	splot $GRID with {dots|points}
 * 	splot $GRID with isosurface level <value>
 *
 * The grid can be cleared by
 *	vclear
 * or deallocated by
 * 	unset vgrid
 */

#include "gp_types.h"
#include "alloc.h"
#include "axis.h"
#include "command.h"
#include "datablock.h"
#include "datafile.h"
#include "eval.h"
#include "graphics.h"
#include "graph3d.h"
#include "parse.h"
#include "util.h"
#include "variable.h"
#include "voxelgrid.h"

#ifdef VOXEL_GRID_SUPPORT

static vgrid *current_vgrid = NULL;			/* active voxel grid */
static struct udvt_entry *udv_VoxelDistance = NULL;	/* reserved user variable */
struct isosurface_opt isosurface_options;

/* Internal prototypes */
static void vfill( t_voxel *grid );
static void modify_voxels( t_voxel *grid, double x, double y, double z,
			    double radius, struct at_type *function );
static void vgrid_stats( vgrid *vgrid );

/* Purely local bookkeeping */
static int nvoxels_modified;
static struct at_type *density_function = NULL;

/*
 * called on program entry and by "reset session"
 */
void
init_voxelsupport()
{
    /* Make sure there is a user variable that can be used as a parameter
     * to the function in the 5th spec of "vfill".
     * Scripts can test if (exists("VoxelDistance")) to check for voxel support.
     */
    udv_VoxelDistance = add_udv_by_name("VoxelDistance");
    udv_VoxelDistance->udv_value.type = CMPLX;
    Gcomplex(&udv_VoxelDistance->udv_value, 0.0, 0.0);

    /* default state of other voxel-related structures */
    isosurface_options.inside_offset = 1;	/* inside color = outside + 1 */
    isosurface_options.tessellation = 0;		/* mixed triangles + quadrangles */
}

/*
 * vx vy vz ranges must be established before the grid can be used
 */
void
check_grid_ranges()
{
    if (current_vgrid == NULL)
	int_error(NO_CARET, "vgrid must be set before use");

    if (isnan(current_vgrid->vxmin) || isnan(current_vgrid->vxmax)) {
	if ((axis_array[FIRST_X_AXIS].set_autoscale & AUTOSCALE_BOTH) == AUTOSCALE_NONE) {
	    current_vgrid->vxmin = axis_array[FIRST_X_AXIS].set_min;
	    current_vgrid->vxmax = axis_array[FIRST_X_AXIS].set_max;
	} else
	    int_error(NO_CARET, "grid limits must be set before use");
    }
    if (isnan(current_vgrid->vymin) || isnan(current_vgrid->vymax)) {
	if ((axis_array[FIRST_Y_AXIS].set_autoscale & AUTOSCALE_BOTH) == AUTOSCALE_NONE) {
	    current_vgrid->vymin = axis_array[FIRST_Y_AXIS].set_min;
	    current_vgrid->vymax = axis_array[FIRST_Y_AXIS].set_max;
	} else
	    int_error(NO_CARET, "grid limits must be set before use");
    }
    if (isnan(current_vgrid->vzmin) || isnan(current_vgrid->vzmax)) {
	if ((axis_array[FIRST_Z_AXIS].set_autoscale & AUTOSCALE_BOTH) == AUTOSCALE_NONE) {
	    current_vgrid->vzmin = axis_array[FIRST_Z_AXIS].set_min;
	    current_vgrid->vzmax = axis_array[FIRST_Z_AXIS].set_max;
	} else
	    int_error(NO_CARET, "grid limits must be set before use");
    }

    current_vgrid->vxdelta = (current_vgrid->vxmax - current_vgrid->vxmin)
			   / (current_vgrid->size - 1);
    current_vgrid->vydelta = (current_vgrid->vymax - current_vgrid->vymin)
			   / (current_vgrid->size - 1);
    current_vgrid->vzdelta = (current_vgrid->vzmax - current_vgrid->vzmin)
			   / (current_vgrid->size - 1);
}

/*
 * Initialize vgrid array
 * set vgrid <name> {size <N>}
 * - retrieve existing voxel grid or create a new one
 * - initialize to N (defaults to N=100 for a new grid)
 * - set current_vgrid to this grid.
 */
void
set_vgrid()
{
    struct udvt_entry *grid = NULL;
    int new_size = 100;		/* default size */
    char *name;

    c_token++;
    if (END_OF_COMMAND || !isletter(c_token+1))
	int_error(c_token, "syntax: set vgrid $<gridname> {size N}");

    /* Create or recycle a datablock with the requested name */
    name = parse_datablock_name();
    grid = add_udv_by_name(name);
    if (grid->udv_value.type == VOXELGRID) {
	/* Keep size of existing grid */
	new_size = grid->udv_value.v.vgrid->size;
	current_vgrid = grid->udv_value.v.vgrid;
    } else {
	/* Note: The only other variable type that starts with a $ is DATABLOCK */
	free_value(&grid->udv_value);
	current_vgrid = gp_alloc(sizeof(vgrid), "new vgrid");
	memset(current_vgrid, 0, sizeof(vgrid));
	current_vgrid->vxmin = not_a_number();
	current_vgrid->vxmax = not_a_number();
	current_vgrid->vymin = not_a_number();
	current_vgrid->vymax = not_a_number();
	current_vgrid->vzmin = not_a_number();
	current_vgrid->vzmax = not_a_number();
	grid->udv_value.v.vgrid = current_vgrid;
	grid->udv_value.type = VOXELGRID;
    }

    if (equals(c_token, "size")) {
	c_token++;
	new_size = int_expression();
    }

    /* FIXME: arbitrary limit to reduce chance of memory exhaustion */
    if (new_size < 10 || new_size > 256)
	int_error(NO_CARET, "vgrid size must be an integer between 10 and 256");

    /* Initialize storage for new voxel grid */
    if (current_vgrid->size != new_size) {
	current_vgrid->size = new_size;
	current_vgrid->vdata = gp_realloc(current_vgrid->vdata,
				    new_size*new_size*new_size*sizeof(t_voxel), "voxel array");
	memset(current_vgrid->vdata, 0, new_size*new_size*new_size*sizeof(t_voxel));
    }

}

/*
 * set vxrange [min:max]
 */
void
set_vgrid_range()
{
    double gridmin, gridmax;
    int save_token = c_token++;

    if (!current_vgrid)
	int_error(NO_CARET, "no voxel grid is active");

    if (!equals(c_token,"["))
	return;
    c_token++;
    gridmin = real_expression();
    if (!equals(c_token,":"))
	return;
    c_token++;
    gridmax = real_expression();
    if (!equals(c_token,"]"))
	return;
    c_token++;

    if (almost_equals(save_token, "vxr$ange")) {
	current_vgrid->vxmin = gridmin;
	current_vgrid->vxmax = gridmax;
    }
    if (almost_equals(save_token, "vyr$ange")) {
	current_vgrid->vymin = gridmin;
	current_vgrid->vymax = gridmax;
    }
    if (almost_equals(save_token, "vzr$ange")) {
	current_vgrid->vzmin = gridmin;
	current_vgrid->vzmax = gridmax;
    }
}

/*
 * show state of all voxel grids
 */
void
show_vgrid()
{
    struct udvt_entry *udv;
    vgrid *vgrid;

    for (udv = first_udv; udv != NULL; udv = udv->next_udv) {
	if (udv->udv_value.type == VOXELGRID) {
	    vgrid = udv->udv_value.v.vgrid;

	    fprintf(stderr, "\t%s:", udv->udv_name);
	    if (vgrid == current_vgrid)
		fprintf(stderr, "\t(active)");
	    fprintf(stderr, "\n");
	    fprintf(stderr, "\t\tsize %d X %d X %d\n",
		    vgrid->size, vgrid->size, vgrid->size);
	    if (isnan(vgrid->vxmin) || isnan(vgrid->vxmax) || isnan(vgrid->vymin)
	    ||  isnan(vgrid->vymax) || isnan(vgrid->vzmin) || isnan(vgrid->vzmax)) {
		fprintf(stderr, "\t\tgrid ranges not set\n");
		continue;
	    }
	    fprintf(stderr, "\t\tvxrange [%g:%g]  vyrange[%g:%g]  vzrange[%g:%g]\n",
		vgrid->vxmin, vgrid->vxmax, vgrid->vymin,
		vgrid->vymax, vgrid->vzmin, vgrid->vzmax);
	    vgrid_stats(vgrid);
	    fprintf(stderr, "\t\tnon-zero voxel values:  min %.2g max %.2g  mean %.2g stddev %.2g\n",
		vgrid->min_value, vgrid->max_value, vgrid->mean_value, vgrid->stddev);
	    fprintf(stderr, "\t\tnumber of zero voxels:  %d   (%.2f%%)\n",
		vgrid->nzero,
		100. * (double)vgrid->nzero / (vgrid->size*vgrid->size*vgrid->size));
	}
    }
}

/*
 * run through the whole grid
 * accumulate min/max, mean, and standard deviation of non-zero voxels
 * TODO: median
 */
static void
vgrid_stats(vgrid *vgrid)
{
    double min = VERYLARGE;
    double max = -VERYLARGE;
    double sum = 0;
    int nzero = 0;
    t_voxel *voxel;
    int N = vgrid->size;
    int i;

    /* bookkeeping for standard deviation */
    double num = 0;
    double delta = 0;
    double delta2 = 0;
    double mean = 0;
    double mean2 = 0;

    for (voxel = vgrid->vdata, i=0; i < N*N*N; voxel++, i++) {
	if (*voxel == 0) {
	    nzero++;
	    continue;
	}
	sum += *voxel;
	if (min > *voxel)
	    min = *voxel;
	if (max < *voxel)
	    max = *voxel;

	/* standard deviation */
	num += 1.0;
	delta = *voxel - mean;
	mean += delta/num;
	delta2 = *voxel - mean;
	mean2 += delta * delta2;
    }

    vgrid->min_value = min;
    vgrid->max_value = max;
    vgrid->nzero = nzero;
    if (num < 2) {
	vgrid->mean_value = vgrid->stddev = not_a_number();
    } else {
	vgrid->mean_value = sum / (double)(N*N*N - nzero);
	vgrid->stddev = sqrt(mean2 / (num - 1.0));
    }

    /* all zeros */
    if (nzero == N*N*N) {
	vgrid->min_value = 0;
	vgrid->max_value = 0;
    }
}

udvt_entry *
get_vgrid_by_name(char *name)
{
    struct udvt_entry *vgrid = get_udv_by_name(name);

    if (!vgrid || vgrid->udv_value.type != VOXELGRID)
	return NULL;
    else
	return vgrid;
}

/*
 * initialize content of voxel grid to all zero
 */
void
vclear_command()
{
    vgrid *vgrid = current_vgrid;

    c_token++;
    if (!END_OF_COMMAND && equals(c_token, "$")) {
	char *name = parse_datablock_name();
	udvt_entry *vgrid_udv = get_vgrid_by_name(name);
	if (!vgrid_udv)
	    int_error(c_token, "no such voxel grid");
	vgrid = vgrid_udv->udv_value.v.vgrid;
    }
    if (vgrid && vgrid->size && vgrid->vdata) {
	int size = vgrid->size;
	memset(vgrid->vdata, 0, size*size*size * sizeof(t_voxel));
    }
}

/*
 * deallocate storage for a voxel grid
 */
void
gpfree_vgrid(struct udvt_entry *grid)
{
    if (grid->udv_value.type != VOXELGRID)
	return;
    free(grid->udv_value.v.vgrid->vdata);
    free(grid->udv_value.v.vgrid);
    if (grid->udv_value.v.vgrid == current_vgrid)
	current_vgrid = NULL;
    grid->udv_value.v.vgrid = NULL;
    grid->udv_value.type = NOTDEFINED;
}

/*
 * 'unset $vgrid' command
 */
void
unset_vgrid()
{
    struct udvt_entry *grid = NULL;
    char *name;

    if (END_OF_COMMAND || !equals(c_token,"$"))
	int_error(c_token, "syntax: unset vgrid $<gridname>");

    /* Look for a datablock with the requested name */
    name = parse_datablock_name();
    grid = get_vgrid_by_name(name);
    if (!grid)
	int_error(c_token, "no such vgrid");

    gpfree_vgrid(grid);
}

/*
 * "set isosurface {triangles|mixed}"
 */
void
set_isosurface()
{
    while (!END_OF_COMMAND) {
	c_token++;
	if (almost_equals(c_token, "triang$les")) {
	    c_token++;
	    isosurface_options.tessellation = 1;
	} else if (almost_equals(c_token, "mix$ed")) {
	    c_token++;
	    isosurface_options.tessellation = 0;
	} else if (almost_equals(c_token, "inside$color")) {
	    c_token++;
	    if (END_OF_COMMAND)
		isosurface_options.inside_offset = 1;
	    else
		isosurface_options.inside_offset = int_expression();
	} else if (almost_equals(c_token, "noin$sidecolor")) {
	    c_token++;
	    isosurface_options.inside_offset = 0;
	} else
	    int_error(c_token,"unrecognized option");
    }
}

void
show_isosurface()
{
    c_token++;
    fprintf(stderr,"\tisosurfaces will use %s\n",
	isosurface_options.tessellation != 0 ? "triangles only"
	: "a mixture of triangles and quadrangles");
    fprintf(stderr,"\tinside surface linetype offset by %d\n",
	isosurface_options.inside_offset);
}

/*
 * voxel(x,y,z) = expr()
 */
void
voxel_command()
{
    double vx, vy, vz;
    int ivx, ivy, ivz;
    int N;
    t_voxel *voxel;

    check_grid_ranges();

    c_token++;
    if (!equals(c_token++,"("))
	int_error(c_token-1, "syntax: voxel(x,y,z) = newvalue");
    vx = real_expression();
    if (!equals(c_token++,","))
	int_error(c_token-1, "syntax: voxel(x,y,z) = newvalue");
    vy = real_expression();
    if (!equals(c_token++,","))
	int_error(c_token-1, "syntax: voxel(x,y,z) = newvalue");
    vz = real_expression();
    if (!equals(c_token++,")"))
	int_error(c_token-1, "syntax: voxel(x,y,z) = newvalue");
    if (!equals(c_token++,"="))
	int_error(c_token-1, "syntax: voxel(x,y,z) = newvalue");

    if (vx < current_vgrid->vxmin || vx > current_vgrid->vxmax
    ||  vy < current_vgrid->vymin || vy > current_vgrid->vymax
    ||  vz < current_vgrid->vzmin || vz > current_vgrid->vzmax) {
	int_warn(NO_CARET, "voxel out of range");
	(void)real_expression();
	return;
    }

    ivx = ceil( (vx - current_vgrid->vxmin) / current_vgrid->vxdelta );
    ivy = ceil( (vy - current_vgrid->vymin) / current_vgrid->vydelta );
    ivz = ceil( (vz - current_vgrid->vzmin) / current_vgrid->vzdelta );

    N = current_vgrid->size;
    FPRINTF((stderr,"\tvgrid array index = %d\n",  ivx + ivy * N + ivz * N*N));

    voxel = &current_vgrid->vdata[ ivx + ivy * N + ivz * N*N ];
    *voxel = real_expression();

    return;
}

/*
 * internal look-up function voxel(x,y,z)
 */
t_voxel
voxel( double vx, double vy, double vz )
{
    int ivx, ivy, ivz;
    int N;

    if (!current_vgrid)
	return not_a_number();

    if (vx < current_vgrid->vxmin || vx > current_vgrid->vxmax
    ||  vy < current_vgrid->vymin || vy > current_vgrid->vymax
    ||  vz < current_vgrid->vzmin || vz > current_vgrid->vzmax)
	return not_a_number();

    ivx = ceil( (vx - current_vgrid->vxmin) / current_vgrid->vxdelta );
    ivy = ceil( (vy - current_vgrid->vymin) / current_vgrid->vydelta );
    ivz = ceil( (vz - current_vgrid->vzmin) / current_vgrid->vzdelta );

    N = current_vgrid->size;
    return current_vgrid->vdata[ ivx + ivy * N + ivz * N*N ];
}

/*
 * user-callable retrieval function voxel(x,y,z)
 */
void
f_voxel(union argument *arg)
{
    struct value a;
    double vx, vy, vz;

    (void) arg;			/* avoid -Wunused warning */
    vz = real(pop(&a));
    vy = real(pop(&a));
    vx = real(pop(&a));

    if (!current_vgrid)
	int_error(NO_CARET, "no active voxel grid");

    if (vx < current_vgrid->vxmin || vx > current_vgrid->vxmax
    ||  vy < current_vgrid->vymin || vy > current_vgrid->vymax
    ||  vz < current_vgrid->vzmin || vz > current_vgrid->vzmax) {
	push(&(udv_NaN->udv_value));
	return;
    }

    push( Gcomplex(&a, voxel(vx, vy, vz), 0.0) );
    return;
}

/*
 * "vfill" works very much like "plot" in that it reads from an input stream
 * of data points specified by the same "using" "skip" "every" and other keyword
 * syntax shared with the "plot" "splot" and "stats" commands.
 * Basic example:
 * 	vfill $FILE using x:y:z:radius:(function())
 * However instead of creating a plot immediately, vfill modifies the content
 * of a voxel grid by incrementing the value of each voxel within a radius.
 *	voxel(vx,vy,vz) += function( distance([vx,vy,vz], [x,y,z]) )
 *
 * vfill shares the routines df_open df_readline ... with the plot and splot
 * commands.
 */
void
vfill_command()
{
    if (!current_vgrid)
	int_error(c_token, "No current voxel grid");
    c_token++;
    vfill(current_vgrid->vdata);
}

static void
vfill(t_voxel *grid)
{
    int plot_num = 0;
    struct curve_points dummy_plot;
    struct curve_points *this_plot = &dummy_plot;
    t_value original_value_sample_var;
    char *name_str;

    check_grid_ranges();

    /*
     * This part is modelled on eval_plots()
     */
    plot_iterator = check_for_iteration();
    while (TRUE) {
	int sample_range_token;
	int start_token = c_token;
	int specs;

	/* Forgive trailing comma on a multi-element plot command */
	if (END_OF_COMMAND) {
	    if (plot_num == 0)
		int_error(c_token, "data input source expected");
	    break;
	}
	plot_num++;

	/* Check for a sampling range */
	if (equals(c_token,"sample") && equals(c_token+1,"["))
	    c_token++;
	sample_range_token = parse_range(SAMPLE_AXIS);
	if (sample_range_token != 0)
	    axis_array[SAMPLE_AXIS].range_flags |= RANGE_SAMPLED;

	/* FIXME: allow replacement of dummy variable? */

	name_str = string_or_express(NULL);
	if (!name_str)
	    int_error(c_token, "no input data source");
	if (!strcmp(name_str, "@@"))
	    int_error(c_token-1, "filling from array not supported");
	if (sample_range_token !=0 && *name_str != '+')
	    int_warn(sample_range_token, "Ignoring sample range in non-sampled data plot");

	/* Dummy up a plot structure so that we can share code with plot command */
	memset( this_plot, 0, sizeof(struct curve_points));
	/* FIXME:  what exactly do we need to fill in? */
	this_plot->plot_type = DATA;
	this_plot->noautoscale = TRUE;

	/* Fixed number of input columns x:y:z:radius:(density_function) */
	specs = df_open(name_str, 5, this_plot);

	/* We will invoke density_function in modify_voxels rather than df_readline */
	if (use_spec[4].at == NULL)
	    int_error(NO_CARET, "5th user spec to vfill must be an expression");
	else {
	    free_at(density_function);
	    density_function = use_spec[4].at;
	    use_spec[4].at = NULL;
	    use_spec[4].column = 0;
	}

	/* Initialize user variable VoxelDistance used by density_function() */
	Gcomplex(&udv_VoxelDistance->udv_value, 0.0, 0.0);

	/* Store a pointer to the named variable used for sampling */
	/* Save prior value of sample variables so we can restore them later */
	this_plot->sample_var = add_udv_by_name(c_dummy_var[0]);
	original_value_sample_var = this_plot->sample_var->udv_value;
	this_plot->sample_var->udv_value.type = NOTDEFINED;

	/* We don't support any further options */
	if (almost_equals(c_token, "w$ith"))
	    int_error(c_token, "vfill does not support 'with' options");

	/* This part is modelled on get_data().
	 * However we process each point as we come to it.
	 */
	if (df_no_use_specs == 5) {
	    int j;
	    int ngood;
	    double v[MAXDATACOLS];
	    memset(v, 0, sizeof(v));

	    /* Initialize stats */
	    ngood = 0;
	    nvoxels_modified = 0;
	    fprintf(stderr,"vfill from %s :\n", name_str);

	    /* If the user has set an explicit locale for numeric input, apply it */
	    /* here so that it affects data fields read from the input file.      */
	    set_numeric_locale();

	    /* Initial state */
	    df_warn_on_missing_columnheader = TRUE;

	    while ((j = df_readline(v, 5)) != DF_EOF) {
		switch (j) {

		case 0:
		    df_close();
		    int_error(this_plot->token, "Bad data on line %d of file %s",
			df_line_number, df_filename ? df_filename : "");
		    continue;

		case DF_UNDEFINED:
		case DF_MISSING:
		    continue;

		case DF_FIRST_BLANK:
		case DF_SECOND_BLANK:
		    continue;

		case DF_COLUMN_HEADERS:
		case DF_FOUND_KEY_TITLE:
		case DF_KEY_TITLE_MISSING:
		    continue;

		default:
		    break;	/* Not continue! */
		}

		/* Ignore out of range points */
		/* FIXME: probably should be range + radius! */
		if (v[0] < current_vgrid->vxmin || current_vgrid->vxmax < v[0])
		    continue;
		if (v[1] < current_vgrid->vymin || current_vgrid->vymax < v[1])
		    continue;
		if (v[2] < current_vgrid->vzmin || current_vgrid->vzmax < v[2])
		    continue;

		/* Now we know for sure we will use the data point */
		ngood++;

		/* At this point get_data() would interpret the contents of v[]
		 * according to the current plot style and store it.
		 * vfill() has a fixed interpretation of v[] and will use it to
		 * modify the current voxel grid.
		 */
		modify_voxels( grid, v[0], v[1], v[2], v[3], density_function );

	    } /* End of loop over input data points */

	    df_close();

	    /* We are finished reading user input; return to C locale for internal use */
	    reset_numeric_locale();

	    if (ngood == 0) {
		if (!forever_iteration(plot_iterator))
		    int_warn(NO_CARET,"Skipping data file with no valid points");
		this_plot->plot_type = NODATA;
	    }

	    /* print some basic stats */
	    fprintf(stderr, "\tnumber of points input:    %8d\n", ngood);
	    fprintf(stderr, "\tnumber of voxels modified: %8d\n", nvoxels_modified);

	} else if (specs < 0) {
	    this_plot->plot_type = NODATA;

	} else {
	    int_error(NO_CARET, "vfill requires exactly 5 using specs x:y:z:radius:(func)");
	}

	/* restore original value of sample variables */
	this_plot->sample_var->udv_value = original_value_sample_var;

	/* Iterate-over-plot mechanism */
	if (empty_iteration(plot_iterator) && this_plot)
	    this_plot->plot_type = NODATA;
	if (forever_iteration(plot_iterator) && (this_plot->plot_type == NODATA)) {
	    /* nothing to do here */ ;
	} else if (next_iteration(plot_iterator)) {
	    c_token = start_token;
	    continue;
	}

	plot_iterator = cleanup_iteration(plot_iterator);
	if (equals(c_token, ",")) {
	    c_token++;
	    plot_iterator = check_for_iteration();
	} else
	    break;

    }

    if (plot_num == 0)
	int_error(c_token, "no data to plot");

    plot_token = -1;

}

/* This is called by vfill for every data point.
 * It modifies all voxels within radius of the point coordinates.
 */
static void
modify_voxels( t_voxel *grid, double x, double y, double z,
			double radius, struct at_type *density_function )
{
    struct value a;
    int ix, iy, iz;
    int ivx, ivy, ivz;
    int nvx, nvy, nvz;
    double vx, vy, vz, distance;
    t_voxel *voxel;
    int N;

    if (x < current_vgrid->vxmin || x > current_vgrid->vxmax
    ||  y < current_vgrid->vymin || y > current_vgrid->vymax
    ||  z < current_vgrid->vzmin || z > current_vgrid->vzmax)
	int_error(NO_CARET, "voxel out of range");

    N = current_vgrid->size;

    /* ivx, ivy, ivz are the indices of the nearest grid point */
    ivx = ceil( (x - current_vgrid->vxmin) / current_vgrid->vxdelta );
    ivy = ceil( (y - current_vgrid->vymin) / current_vgrid->vydelta );
    ivz = ceil( (z - current_vgrid->vzmin) / current_vgrid->vzdelta );

    /* nvx, nvy, nvz are the number of grid points within radius */
    nvx = floor( radius / current_vgrid->vxdelta );
    nvy = floor( radius / current_vgrid->vydelta );
    nvz = floor( radius / current_vgrid->vzdelta );

    /* Only print once */
    if (nvoxels_modified == 0)
	fprintf(stderr,
		"\tradius %g gives a brick of %d voxels on x, %d voxels on y, %d voxels on z\n",
		radius, 1+2*nvx, 1+2*nvy, 1+2*nvz);

    /* The iteration covers a cube rather than a sphere */
    evaluate_inside_using = TRUE;
    for (ix = ivx - nvx; ix <= ivx + nvx; ix++) {
	if (ix < 0 || ix >= N)
	    continue;
	for (iy = ivy - nvy; iy <= ivy + nvy; iy++) {
	    if (iy < 0 || iy >= N)
		continue;
	    for (iz = ivz - nvz; iz <= ivz + nvz; iz++) {
		int index;

		if (iz < 0 || iz >= N)
		    continue;
		index = ix + iy * N + iz * N*N;
		if (index < 0 || index > N*N*N)
		    continue;
		voxel = &current_vgrid->vdata[index];

		/* vx, vy, vz are the true coordinates of this voxel */
		vx = current_vgrid->vxmin + ix * current_vgrid->vxdelta;
		vy = current_vgrid->vymin + iy * current_vgrid->vydelta;
		vz = current_vgrid->vzmin + iz * current_vgrid->vzdelta;
		distance = sqrt( (vx-x)*(vx-x) + (vy-y)*(vy-y) + (vz-z)*(vz-z) );

		/* Limit the summation to a sphere rather than a cube */
		/* but always include the voxel nearest the target    */
		if (distance > radius && (ix != ivx || iy != ivy || iz != ivz))
		    continue;

		/* Store in user variable VoxelDistance for use by density_function */
		udv_VoxelDistance->udv_value.v.cmplx_val.real = distance;

		evaluate_at(density_function, &a);
		*voxel += real(&a);

		/* Bookkeeping */
		nvoxels_modified++;
	    }
	}
    }
    evaluate_inside_using = FALSE;

    return;
}

#endif /* VOXEL_GRID_SUPPORT */

#ifndef VOXEL_GRID_SUPPORT
# define NO_SUPPORT int_error(NO_CARET, "this gnuplot does not support voxel grids")

void check_grid_ranges()  { NO_SUPPORT; }
void set_vgrid()          { NO_SUPPORT; }
void set_vgrid_range()    { NO_SUPPORT; }
void show_vgrid()         { NO_SUPPORT; }
void show_isosurface()    { NO_SUPPORT; }
void voxel_command()      { NO_SUPPORT; }
void vfill_command()      { NO_SUPPORT; }
void vclear_command()     {}
void unset_vgrid()        {}
void init_voxelsupport()  {}
void set_isosurface()     {}

udvt_entry *get_vgrid_by_name(char *c) { return NULL; }
void gpfree_vgrid(struct udvt_entry *x) {}

#endif /* no VOXEL_GRID_SUPPORT */
