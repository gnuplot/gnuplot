#ifndef lint
static char *RCSid = "$Id: gnubin.c%v 3.50 1993/07/09 05:35:24 woo Exp $";
#endif


/*
 * The addition of gnu_binary_files and binary_files, along with a small patch
 * to command.c, will permit gnuplot to plot binary files.
 * gnu_binary_files  - contains the code that relies on gnuplot include files
 *                     and other definitions
 * binary_files      - contains those things that are independent of those 
 *                     definitions and files
 *
 * With these routines, hidden line removal of your binary data is possible!
 *
 * Last update: 3/3/92 for Gnuplot 3.24.
 * Created from code for written by RKC for gnuplot 2.0b.
 *
 * 19 September 1992  Lawrence Crowl  (crowl@cs.orst.edu)
 * Added user-specified bases for log scaling.
 *
 * Copyright (c) 1991,1992 Robert K. Cunningham, MIT Lincoln Laboratory
 *
 */
#include <stdio.h>
#include <math.h>
#include "plot.h"
#include "setshow.h"

/******************* SHARED INCLUDE FILE--start ***********************
 *  I recommend that these be put into an include file that all
 *  will share -- but I leave it up to the powers that be to do this.
 */
/* Copied from command.c -- this should be put in a shared macro file */
#define inrange(z,min,max) ((min<max) ? ((z>=min)&&(z<=max)) : ((z>=max)&&(z<=min)) )

/* Routines for interfacing with command.c */
float GPFAR *vector();
float GPFAR *extend_vector();
float GPFAR *retract_vector();
float GPFAR * GPFAR *matrix();
float GPFAR * GPFAR *extend_matrix();
float GPFAR * GPFAR *retract_matrix();
void free_matrix();
void free_vector();
/******************* SHARED INCLUDE FILE--end *************************/
/*
  Here we keep putting new plots onto the end of the linked list

  We assume the data's x,y values have x1<x2, x2<x3... and 
                                       y1<y2, y2<y3... .
  Actually, I think the assumption is less stron than that--it looks like
  the direction just has to be the same.

  This routine expects the following to be properly initialized:
      is_log_x, is_log_y, and is_log_z 
      base_log_x, base_log_y, and base_log_z 
      log_base_log_x, log_base_log_y, and log_base_log_z 
      xmin,ymin, and zmin
      xmax,ymax, and zmax
      autoscale_lx, autoscale_ly, and autoscale_lz

*/
int
  get_binary_data(this_plot,fp,p_ret_iso)
struct surface_points *this_plot;
FILE *fp;
struct iso_curve **p_ret_iso;
{
  register int i,j;
  float GPFAR * GPFAR *matrix, GPFAR *rt, GPFAR *ct;
  int nr,nc;
  int ndata;
  struct iso_curve *this_iso;
  float z;

  this_plot->plot_type = DATA3D;
  this_plot->has_grid_topology = TRUE;

  if(!fread_matrix(fp,&matrix,&nr,&nc,&rt,&ct))
    int_error("Binary file read error: format unknown!",NO_CARET);

  /* Now we do some error checking on the x and y axis */
  if(is_log_x)
    for(i=0; i<nc; i++)
      if(ct[i] < 0.0)
	int_error("X value must be above 0 for log scale!",NO_CARET);
      else
	ct[i] = log(ct[i])/log_base_log_x;

  if(is_log_y)
    for(i=0; i<nr; i++)
      if(rt[i] < 0.0)
	int_error("Y value must be above 0 for log scale!",NO_CARET);
      else
	rt[i] = log(rt[i])/log_base_log_y;

  /* Count up the number of used column entries */
  if (autoscale_lx) {
    ndata = nc;
    for(j=0; j< nc; j++){
      if (ct[j] < xmin) xmin = ct[j];
      if (ct[j] > xmax) xmax = ct[j];
    }
  }
  else {
    for(ndata = 0, j = 0; j< nc; j++){
      if (!((ct[j] < xmin) || (ct[j] > xmax)))/*Column is in bounds*/
	ndata++;
    }
  }

  for(i=0; i < nr; i++){
      if (autoscale_ly) {
	if (rt[i] < ymin) ymin = rt[i];
	if (rt[i] > ymax) ymax = rt[i];
      }
      else if ((rt[i] < ymin) || (rt[i] > ymax))/* This row is out of bounds */
	continue;

      this_iso = iso_alloc( ndata );/*Allocate the correct number of entries*/
      for(ndata = 0, j = 0; j< nc; j++){/* Cycle through data */

	if ((ct[j] < xmin) || (ct[j] > xmax))/*Column is out of bounds*/
	  continue;       /* Only affects non-autoscale_lx cases */

	this_iso->points[ndata].x = ct[j];
	this_iso->points[ndata].y = rt[i];
	z = matrix[i][j];
	if(is_log_z)
	  if (z <= 0.0)
	    int_error("Z value must be above 0 for log scale!",NO_CARET);
	  else
	    z = log(z)/log_base_log_z;
	this_iso->points[ndata].z = z;
      
	if (autoscale_lz) {
	  if (z < zmin) zmin = z;
	  if (z > zmax) zmax = z;
	}
	ndata++;
      }
      this_iso->p_count = ndata;
      this_iso->next = this_plot->iso_crvs;
      this_plot->iso_crvs = this_iso;
      this_plot->num_iso_read++;
  }
  
  free_matrix(matrix,0,nr-1,0,nc-1);
  free_vector(rt,0,nr-1);
  free_vector(ct,0,nc-1);
  *p_ret_iso = this_iso;
  return(ndata+1);
}
