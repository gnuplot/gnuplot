/*
 * $Id: setshow.h,v 3.26 92/03/24 22:34:15 woo Exp Locker: woo $
 *
 */

/* GNUPLOT - setshow.h */
/*
 * Copyright (C) 1986, 1987, 1990, 1991, 1992   Thomas Williams, Colin Kelley
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
 *     Thomas Williams,  Colin Kelley.
 * 
 *   Gnuplot 2.0 additions:
 *       Russell Lang, Dave Kotz, John Campbell.
 *
 *   Gnuplot 3.0 additions:
 *       Gershon Elber and many others.
 * 
 * Send your comments or suggestions to 
 *  info-gnuplot@ames.arc.nasa.gov.
 * This is a mailing list; to join it send a note to 
 *  info-gnuplot-request@ames.arc.nasa.gov.  
 * Send bug reports to
 *  bug-gnuplot@ames.arc.nasa.gov.
 */

/*
 * global variables to hold status of 'set' options
 *
 */
extern BOOLEAN			autoscale_r;
extern BOOLEAN			autoscale_t;
extern BOOLEAN			autoscale_u;
extern BOOLEAN			autoscale_v;
extern BOOLEAN			autoscale_x;
extern BOOLEAN			autoscale_y;
extern BOOLEAN			autoscale_z;
extern BOOLEAN			autoscale_lt;
extern BOOLEAN			autoscale_lu;
extern BOOLEAN			autoscale_lv;
extern BOOLEAN			autoscale_lx;
extern BOOLEAN			autoscale_ly;
extern BOOLEAN			autoscale_lz;
extern BOOLEAN			clip_points;
extern BOOLEAN			clip_lines1;
extern BOOLEAN			clip_lines2;
extern BOOLEAN			draw_border;
extern BOOLEAN			draw_surface;
extern BOOLEAN			timedate;
extern char			dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];
extern char			xformat[];
extern char			yformat[];
extern char			zformat[];
extern enum PLOT_STYLE data_style, func_style;
extern BOOLEAN			grid;
extern int			key;
extern double			key_x, key_y, key_z; /* user specified position for key */
extern BOOLEAN			log_x, log_y, log_z;
extern FILE*			outfile;
extern char			outstr[];
extern BOOLEAN			parametric;
extern BOOLEAN			polar;
extern BOOLEAN			hidden3d;
extern int			angles_format;
extern int			mapping3d;
extern int			samples;
extern int			iso_samples;
extern float			xsize; /* scale factor for size */
extern float			ysize; /* scale factor for size */
extern float			zsize; /* scale factor for size */
extern float			surface_rot_z;
extern float			surface_rot_x;
extern float			surface_scale;
extern float			surface_zscale;
extern int			term; /* unknown term is 0 */
extern char			term_options[];
extern char			title[];
extern char			xlabel[];
extern char			ylabel[];
extern char			zlabel[];
extern int			time_xoffset;
extern int			time_yoffset;
extern int			title_xoffset;
extern int			title_yoffset;
extern int			xlabel_xoffset;
extern int			xlabel_yoffset;
extern int			ylabel_xoffset;
extern int			ylabel_yoffset;
extern int			zlabel_xoffset;
extern int			zlabel_yoffset;
extern double			rmin, rmax;
extern double			tmin, tmax, umin, umax, vmin, vmax;
extern double			xmin, xmax, ymin, ymax, zmin, zmax;
extern double			loff, roff, toff, boff;
extern int			draw_contour;
extern int			contour_pts;
extern int			contour_kind;
extern int			contour_order;
extern int			contour_levels;
extern double			zero; /* zero threshold, not 0! */

extern BOOLEAN xzeroaxis;
extern BOOLEAN yzeroaxis;

extern BOOLEAN xtics;
extern BOOLEAN ytics;
extern BOOLEAN ztics;

extern float ticslevel;

extern struct ticdef xticdef;
extern struct ticdef yticdef;
extern struct ticdef zticdef;

extern BOOLEAN			tic_in;

extern struct text_label *first_label;
extern struct arrow_def *first_arrow;

/* The set and show commands, in setshow.c */
extern void set_command();
extern void show_command();
/* and some accessible support functions */
extern enum PLOT_STYLE get_style();
extern BOOLEAN load_range();
extern void show_version();
