/* GNUPLOT - setshow.h */
/*
 * Copyright (C) 1986, 1987, 1990   Thomas Williams, Colin Kelley
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
 * This software  is provided "as is" without express or implied warranty.
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
 * send your comments or suggestions to (pixar!info-gnuplot@sun.com).
 * 
 */

/*
 * global variables to hold status of 'set' options
 *
 */
extern BOOLEAN			autoscale_x;
extern BOOLEAN			autoscale_y;
extern BOOLEAN			autoscale_lx;
extern BOOLEAN			autoscale_ly;
extern BOOLEAN			clip_points;
extern BOOLEAN			clip_lines1;
extern BOOLEAN			clip_lines2;
extern char			dummy_var[];
extern char			xformat[];
extern char			yformat[];
extern enum PLOT_STYLE data_style, func_style;
extern BOOLEAN			grid;
extern int			key;
extern double			key_x, key_y; /* user specified position for key */
extern BOOLEAN			log_x, log_y;
extern FILE*			outfile;
extern char			outstr[];
extern BOOLEAN			polar;
extern int			samples;
extern float			xsize; /* scale factor for size */
extern float			ysize; /* scale factor for size */
extern int			term; /* unknown term is 0 */
extern char			title[];
extern char			xlabel[];
extern char			ylabel[];
extern double			xmin, xmax, ymin, ymax;
extern double			loff, roff, toff, boff;
extern double			zero; /* zero threshold, not 0! */

extern BOOLEAN xtics;
extern BOOLEAN ytics;

extern struct ticdef xticdef;
extern struct ticdef yticdef;

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
