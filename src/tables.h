/*
 * $Id: tables.h,v 1.13.2.2 2000/05/03 21:26:12 joze Exp $
 */

/* GNUPLOT - tables.h */

/*[
 * Copyright 1999  Lars Hecking
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
]*/

#ifndef GNUPLOT_TABLES_H
#define GNUPLOT_TABLES_H

#include "plot.h"


typedef void (*parsefuncp_t) __PROTO((void));

struct gen_ftable {
    const char *key;
    parsefuncp_t value;
};

/* The basic structure */
struct gen_table {
    const char *key;
    int value;
};

/* options for plot/splot */
enum plot_id {
    P_INVALID,
    P_AXES, P_BINARY, P_EVERY, P_INDEX, P_MATRIX, P_SMOOTH, P_THRU,
    P_TITLE, P_NOTITLE, P_USING, P_WITH
};

/* options for plot ax[ei]s */
enum plot_axes_id {
    AXES_X1Y1, AXES_X2Y2, AXES_X1Y2, AXES_X2Y1, AXES_NONE
};

/* plot smooth parameters in plot.h */

/* options for 'save' command */
enum save_id { SAVE_INVALID, SAVE_FUNCS, SAVE_TERMINAL, SAVE_SET, SAVE_VARS };

/* options for 'show' and 'set' commands
 * this is rather big, we might be better off with a hash table */
enum set_id {
    S_INVALID,
    S_ACTIONTABLE, S_ALL, S_ANGLES, S_ARROW, S_AUTOSCALE, S_BARS, S_BORDER,
    S_BOXWIDTH, S_CLABEL, S_CLIP, S_CNTRPARAM, S_CONTOUR, S_DATA, S_FUNCTIONS,
    S_DGRID3D, S_DUMMY, S_ENCODING, S_FORMAT, S_GRID, S_HIDDEN3D, S_HISTORYSIZE,
    S_ISOSAMPLES, S_KEY, S_KEYTITLE, S_LABEL, S_LINESTYLE, S_LOADPATH, S_LOCALE,
    S_LOGSCALE, S_MAPPING, S_MARGIN, S_LMARGIN, S_RMARGIN,
    S_TMARGIN, S_BMARGIN, S_MISSING,
#ifdef USE_MOUSE
    S_MOUSE,
#endif
    S_MULTIPLOT, S_MX2TICS, S_NOMX2TICS, S_MXTICS, S_NOMXTICS,
    S_MY2TICS, S_NOMY2TICS, S_MYTICS, S_NOMYTICS,
    S_MZTICS, S_NOMZTICS,
    S_OFFSETS, S_ORIGIN, S_OUTPUT, S_PARAMETRIC,
#ifdef PM3D
    S_PALETTE, S_PM3D,
#endif
    S_PLOT, S_POINTSIZE, S_POLAR,
    S_RRANGE, S_SAMPLES, S_SIZE, S_SURFACE, S_STYLE, S_TERMINAL, S_TERMOPTIONS,
    S_TICS, S_TICSCALE, S_TICSLEVEL, S_TIMEFMT, S_TIMESTAMP, S_TITLE,
    S_TRANGE, S_URANGE, S_VARIABLES, S_VERSION, S_VIEW, S_VRANGE,

    S_X2DATA, S_X2DTICS, S_NOX2DTICS, S_X2LABEL, S_X2MTICS, S_NOX2MTICS,
    S_X2RANGE, S_X2TICS, S_NOX2TICS,
    S_XDATA, S_XDTICS, S_NOXDTICS, S_XLABEL, S_XMTICS, S_NOXMTICS, S_XRANGE,
    S_XTICS, S_NOXTICS,

    S_Y2DATA, S_Y2DTICS, S_NOY2DTICS, S_Y2LABEL, S_Y2MTICS, S_NOY2MTICS,
    S_Y2RANGE, S_Y2TICS, S_NOY2TICS,
    S_YDATA, S_YDTICS, S_NOYDTICS, S_YLABEL, S_YMTICS, S_NOYMTICS, S_YRANGE,
    S_YTICS, S_NOYTICS,

    S_ZDATA, S_ZDTICS, S_NOZDTICS, S_ZLABEL, S_ZMTICS, S_NOZMTICS, S_ZRANGE,
    S_ZTICS, S_NOZTICS,

    S_ZERO, S_ZEROAXIS, S_XZEROAXIS, S_X2ZEROAXIS, S_YZEROAXIS, S_Y2ZEROAXIS
};

enum set_encoding_id {
   S_ENC_DEFAULT, S_ENC_ISO8859_1, S_ENC_CP437, S_ENC_CP850,
   S_ENC_INVALID
};

enum set_hidden3d_id {
    S_HI_INVALID,
    S_HI_DEFAULTS, S_HI_OFFSET, S_HI_NOOFFSET, S_HI_TRIANGLEPATTERN,
    S_HI_UNDEFINED, S_HI_NOUNDEFINED, S_HI_ALTDIAGONAL, S_HI_NOALTDIAGONAL,
    S_HI_BENTOVER, S_HI_NOBENTOVER
};

enum set_key_id {
    S_KEY_INVALID,
    S_KEY_TOP, S_KEY_BOTTOM, S_KEY_LEFT, S_KEY_RIGHT, S_KEY_UNDER,
    S_KEY_OUTSIDE, S_KEY_LLEFT, S_KEY_RRIGHT, S_KEY_REVERSE, S_KEY_NOREVERSE,
    S_KEY_BOX, S_KEY_NOBOX, S_KEY_SAMPLEN, S_KEY_SPACING, S_KEY_WIDTH,
    S_KEY_TITLE
};

enum show_style_id {
    SHOW_STYLE_INVALID,
    SHOW_STYLE_DATA, SHOW_STYLE_FUNCTION, SHOW_STYLE_LINE
};

extern struct gen_table command_tbl[];
extern struct gen_table plot_tbl[];
extern struct gen_table plot_axes_tbl[];
extern struct gen_table plot_smooth_tbl[];
extern struct gen_table save_tbl[];
extern struct gen_table set_tbl[];
extern struct gen_table set_key_tbl[];
extern struct gen_table set_encoding_tbl[];
extern struct gen_table set_hidden3d_tbl[];
extern struct gen_table show_style_tbl[];
extern struct gen_table plotstyle_tbl[];

extern struct gen_ftable command_ftbl[];

/* Function prototypes */
int lookup_table __PROTO((struct gen_table *, int));
parsefuncp_t lookup_ftable __PROTO((struct gen_ftable *, int));

#endif /* GNUPLT_TABLES_H */
