/*
 * $Id: setshow.h%v 3.50 1993/07/09 05:35:24 woo Exp $
 *
 */

/* GNUPLOT - setshow.h */
/*
 * Copyright (C) 1986 - 1993   Thomas Williams, Colin Kelley
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
 * There is a mailing list for gnuplot users. Note, however, that the
 * newsgroup 
 *	comp.graphics.gnuplot 
 * is identical to the mailing list (they
 * both carry the same set of messages). We prefer that you read the
 * messages through that newsgroup, to subscribing to the mailing list.
 * (If you can read that newsgroup, and are already on the mailing list,
 * please send a message info-gnuplot-request@dartmouth.edu, asking to be
 * removed from the mailing list.)
 *
 * The address for mailing to list members is
 *	   info-gnuplot@dartmouth.edu
 * and for mailing administrative requests is 
 *	   info-gnuplot-request@dartmouth.edu
 * The mailing list for bug reports is 
 *	   bug-gnuplot@dartmouth.edu
 * The list of those interested in beta-test versions is
 *	   info-gnuplot-beta@dartmouth.edu
 */

/*
 * global variables to hold status of 'set' options
 *
 */
extern TBOOLEAN			autoscale_r;
extern TBOOLEAN			autoscale_t;
extern TBOOLEAN			autoscale_u;
extern TBOOLEAN			autoscale_v;
extern TBOOLEAN			autoscale_x;
extern TBOOLEAN			autoscale_y;
extern TBOOLEAN			autoscale_z;
extern TBOOLEAN			autoscale_lt;
extern TBOOLEAN			autoscale_lu;
extern TBOOLEAN			autoscale_lv;
extern TBOOLEAN			autoscale_lx;
extern TBOOLEAN			autoscale_ly;
extern TBOOLEAN			autoscale_lz;
extern double			boxwidth;
extern TBOOLEAN			clip_points;
extern TBOOLEAN			clip_lines1;
extern TBOOLEAN			clip_lines2;
extern TBOOLEAN			draw_border;
extern TBOOLEAN			draw_surface;
extern TBOOLEAN			timedate;
extern char			dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];
extern char			xformat[];
extern char			yformat[];
extern char			zformat[];
extern enum PLOT_STYLE data_style, func_style;
extern TBOOLEAN			grid;
extern int			key;
extern double			key_x, key_y, key_z; /* user specified position for key */
extern TBOOLEAN			is_log_x, is_log_y, is_log_z;
extern double			base_log_x, base_log_y, base_log_z;
				/* base, for computing pow(base,x) */
extern double			log_base_log_x, log_base_log_y, log_base_log_z;
				/* log of base, for computing logbase(base,x) */
extern FILE*			outfile;
extern char			outstr[];
extern TBOOLEAN			parametric;
extern TBOOLEAN			polar;
extern TBOOLEAN			hidden3d;
extern int			angles_format;
extern int			mapping3d;
extern int			samples;
extern int			samples_1;
extern int			samples_2;
extern int			iso_samples_1;
extern int			iso_samples_2;
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
extern TBOOLEAN      label_contours;
extern int			contour_pts;
extern int			contour_kind;
extern int			contour_order;
extern int			contour_levels;
extern double			zero; /* zero threshold, not 0! */
extern int			levels_kind;
extern double		levels_list[MAX_DISCRETE_LEVELS];

extern int			dgrid3d_row_fineness;
extern int			dgrid3d_col_fineness;
extern int			dgrid3d_norm_value;
extern TBOOLEAN			dgrid3d;

extern TBOOLEAN xzeroaxis;
extern TBOOLEAN yzeroaxis;

extern TBOOLEAN xtics;
extern TBOOLEAN ytics;
extern TBOOLEAN ztics;

extern float ticslevel;

extern struct ticdef xticdef;
extern struct ticdef yticdef;
extern struct ticdef zticdef;

extern TBOOLEAN			tic_in;

extern struct text_label *first_label;
extern struct arrow_def *first_arrow;

/* The set and show commands, in setshow.c */
extern void set_command();
extern void show_command();
/* and some accessible support functions */
extern enum PLOT_STYLE get_style();
extern TBOOLEAN load_range();
extern void show_version();
