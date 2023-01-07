#ifndef GNUPLOT_WATCH_H
#define GNUPLOT_WATCH_H

typedef struct watch_t {
    struct watch_t *next;
    int watchno;	/* sequential position in plot command */
    AXIS_INDEX type;	/* FIRST_Y_AXIS, MOUSE_PROXY_AXIS, etc */
    double target;	/* target value in user coordinates */
    struct udft_entry *func; /* If the target is F(x,y)=<value>, this is F() */
    int hits;		/* number of triggers during most recent plot command */
} watch_t;


/* Exported variables */

extern TBOOLEAN watch_mouse_active;

/* Prototype for bisection (used also by "sharpen" filter) */
typedef enum t_bisection_target {
    BISECT_MINIMIZE = -1,
    BISECT_MATCH = 0,
    BISECT_MAXIMIZE = 1
} t_bisection_target;
void bisect_hit(struct curve_points *plot, t_bisection_target target,
		double *xhit, double *yhit, double xlow, double xhigh);

#ifdef USE_WATCHPOINTS

/* Prototypes of functions exported by watch.c */

void parse_watch(struct curve_points *plot);
void watch_line(struct curve_points *plot, double x1, double y1, double z1,
		double x2, double y2, double z2);
void init_watch(struct curve_points *plot);
void free_watchlist(struct watch_t *watchlist);
void show_watchpoints(void);
void reset_watches(void);
void unset_watchpoint_style(void);
void set_style_watchpoint(void);
void show_style_watchpoint(void);

#else	/* USE_WATCHPOINTS */

/* Dummy functions so that the core code is spared having to 
 * wrap the corresponding calls in #ifdef USE_WATCHPOINTS
 */

#define watch_line(p,x1,y1,z1,x2,y2,z2)
#define init_watch(p)
#define parse_watch(p)
#define free_watchlist(w)
#define show_watchpoints()
#define reset_watches()
#define unset_watchpoint_style()
#define show_style_watchpoint()

#endif	/* USE_WATCHPOINTS */

#endif /* GNUPLOT_WATCH_H */
