#ifndef GNUPLOT_BOUNDARY_H
# define GNUPLOT_BOUNDARY_H
#include "syscfg.h"

void boundary(struct curve_points *plots, int count);
void do_key_bounds(legend_key *key);
void do_key_layout(legend_key *key);
int find_maxl_keys(struct curve_points *plots, int count, int *kcnt);
void do_key_sample(struct curve_points *this_plot,
		   legend_key *key, char *title, coordval var_color);
void do_key_sample_point(struct curve_points *this_plot, legend_key *key);
void draw_titles(void);
void draw_key(legend_key *key, TBOOLEAN key_pass);
void advance_key(TBOOLEAN only_invert);
TBOOLEAN at_left_of_key(void);

/* Probably some of these could be made static */
extern int key_entry_height;
extern int key_point_offset;
extern int ylabel_x, y2label_x, xlabel_y, x2label_y;
extern int ylabel_y, y2label_y, xtic_y, x2tic_y, ytic_x, y2tic_x;
extern int key_cols, key_rows;
extern int key_count;
extern int title_x, title_y;

#endif /* GNUPLOT_BOUNDARY_H */
