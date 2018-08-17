/*
 * $Id: jitter.h,v 1.0 2015/09/13 19:30:52 sfeam Exp $
 */
#ifndef GNUPLOT_JITTER_H
# define GNUPLOT_JITTER_H

#include "syscfg.h"
#include "axis.h"
#include "command.h"
#include "gp_types.h"
#include "gadgets.h"
#include "graphics.h"
#include "save.h"
#include <math.h>

enum jitterstyle {
    JITTER_DEFAULT = 0,
    JITTER_SWARM,
    JITTER_SQUARE,
    JITTER_ON_Y
    };

typedef struct {
    struct position overlap;
    double spread;
    double limit;
    enum jitterstyle style;
} t_jitter;
extern t_jitter jitter;

extern void jitter_points __PROTO((struct curve_points *plot));
extern void set_jitter __PROTO(( void ));
extern void show_jitter __PROTO(( void ));
extern void unset_jitter __PROTO(( void ));
extern void save_jitter __PROTO(( FILE * ));

#endif
