#ifndef GOT_DRIVER_H

#define GOT_DRIVER_H
#define GOT_TERM_DRIVER /* I started using this name for some reason */

#include "plot.h"
#include "bitmap.h"
#include "setshow.h"

/* functions provided by in term.c */

void do_point __PROTO((unsigned int x, unsigned int y, int number));
void line_and_point __PROTO((unsigned int x, unsigned int y, int number));
void do_arrow __PROTO((unsigned int sx, unsigned int sy, unsigned int ex, unsigned int ey, int head));
int null_text_angle __PROTO((int ang));
int null_justify_text __PROTO((enum JUSTIFY just));
int null_scale __PROTO((double x, double y));
int do_scale __PROTO((double x, double y));
void options_null __PROTO((void));
void UNKNOWN_null __PROTO((void));
int set_font_null __PROTO((char *s));
void null_set_pointsize __PROTO((double size));

extern FILE *outfile;
extern struct termentry *term;
extern float xsize, ysize;

/* for use by all drivers */
#ifndef NEXT
#define sign(x) ((x) >= 0 ? 1 : -1)
#else
/* it seems that sign as macro causes some conflict with precompiled headers */
static int sign(int x)
{
  return x >=0 ? 1 : -1;
}
#endif

/* abs as macro is now uppercase, there are conflicts with a few C compilers
   that have abs as macro, even though ANSI defines abs as function
   (int abs(int)). Most calls to ABS in term/* could be changed to abs if
   they use only int arguments and others to fabs, but for the time being,
   all calls are done via the macro */
#ifndef ABS
#define ABS(x) ((x) >= 0 ? (x) : -(x))
#endif

/*  GPMIN/GPMAX are already defined in "plot.h"  */

#define NICE_LINE		0
#define POINT_TYPES		6

#endif
