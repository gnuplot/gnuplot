
#ifndef TERM_POST_H
# define TERM_POST_H

/* Needed by terminals which output postscript
 * (post.trm and pslatex.trm)
 */

extern TBOOLEAN ps_color;
extern TBOOLEAN ps_solid;

#define PS_POINT_TYPES 8

/* page offset in pts */
#define PS_XOFF 50
#define PS_YOFF 50

/* assumes landscape */
#define PS_XMAX 7200
#define PS_YMAX 5040

#define PS_XLAST (PS_XMAX - 1)
#define PS_YLAST (PS_YMAX - 1)

#define PS_VTIC (PS_YMAX/80)
#define PS_HTIC (PS_YMAX/80)

/* scale is 1pt = 10 units */
#define PS_SC (10)

/* linewidth = 0.5 pts */
#define PS_LW (0.5*PS_SC)

/* default is 14 point characters */
#define PS_VCHAR (14*PS_SC)
#define PS_HCHAR (14*PS_SC*6/10)

#endif TERM_POST_H
