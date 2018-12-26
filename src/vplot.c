/* GNUPLOT - vplot.c */

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
 * This file implements 3D plotting directly from a voxel grid
 * 	splot $voxelgrid with {dots|points} {above <iso_level>}
 *	splot $voxelgrid with isosurface level <iso_level>
 * For point or dot plots
 * 	points are only drawn if voxel value > iso_level (defaults to 0)
 * For isosurface plots
 *	the isosurface is generated for value == iso_level (default ???)
 */

#include "gp_types.h"
#include "jitter.h"
#include "graph3d.h"
#include "pm3d.h"
#include "util.h"
#include "util3d.h"
#include "voxelgrid.h"
#include "vplot.h"

void
vplot_points (struct surface_points *plot, double level)
{
    int ix, iy, iz;
    double vx, vy, vz;
    struct vgrid *vgrid = plot->vgrid;
    struct termentry *t = term;
    int N = vgrid->size;
    t_voxel *voxel;
    int index;
    int x, y;

    /* dots or points only */
    if (plot->lp_properties.p_type == PT_CHARACTER)
	plot->lp_properties.p_type = -1;
    if (plot->lp_properties.p_type == PT_VARIABLE)
	plot->lp_properties.p_type = -1;

    /* Set whatever we can that applies to every point in the loop */
    if (plot->lp_properties.pm3d_color.type == TC_RGB)
	set_rgbcolor_const( plot->lp_properties.pm3d_color.lt );

    for (ix = 0; ix < N; ix++) {
	for (iy = 0; iy < N; iy++) {
	    for (iz = 0; iz < N; iz++) {

		index = ix + iy * N + iz * N*N;
		voxel = &vgrid->vdata[index];

		if (*voxel <= level)
		    continue;

		/* vx, vy, vz are the true coordinates of this voxel */
		vx = vgrid->vxmin + ix * vgrid->vxdelta;
		vy = vgrid->vymin + iy * vgrid->vydelta;
		vz = vgrid->vzmin + iz * vgrid->vzdelta;

		if (jitter.spread > 0) {
		    vx += jitter.spread * vgrid->vxdelta * ( (double)(random()/(double)RAND_MAX ) - 0.5);
		    vy += jitter.spread * vgrid->vydelta * ( (double)(random()/(double)RAND_MAX ) - 0.5);
		    vz += jitter.spread * vgrid->vzdelta * ( (double)(random()/(double)RAND_MAX ) - 0.5);
		}

		map3d_xy(vx, vy, vz, &x, &y);

		/* the usual variable color array cannot be used for voxel data */
		/* but we can use the voxel value itself as a palette index     */
		if (plot->lp_properties.pm3d_color.type == TC_Z)
		    set_color(cb2gray(*voxel));

		/* This code is also used for "splot ... with dots" */
		if (plot->plot_style == DOTS)
		    (*t->point) (x, y, -1);

		/* The normal case */
		else if (plot->lp_properties.p_type >= 0)
		    (*t->point) (x, y, plot->lp_properties.p_type);

	    }
	}
    }

}

void
vplot_isosurface (struct surface_points *plot, double level)
{
}
