/*
 * $Id$
 */

/* GNUPLOT - wxt_term.h */

/*[
 * Copyright 2005,2006   Timothee Lecomte
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
 *
 *
 * Alternatively, the contents of this file may be used under the terms of the
 * GNU General Public License Version 2 or later (the "GPL"), in which case the
 * provisions of GPL are applicable instead of those above. If you wish to allow
 * use of your version of this file only under the terms of the GPL and not
 * to allow others to use your version of this file under the above gnuplot
 * license, indicate your decision by deleting the provisions above and replace
 * them with the notice and other provisions required by the GPL. If you do not
 * delete the provisions above, a recipient may use your version of this file
 * under either the GPL or the gnuplot license.
]*/

/* ------------------------------------------------------
 * Here you will find the declarations of the C++ functions
 * used in wxt.trm and defined in wxt_gui.cpp,
 * where the wxWidgets terminal is mainly implemented.
 * ------------------------------------------------------*/

#ifndef GNUPLOT_WXT_TERM_H
# define GNUPLOT_WXT_TERM_H

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/

extern void wxt_init(void);
extern void wxt_graphics(void);
extern void wxt_text(void);
extern void wxt_linetype(int linetype);
extern void wxt_move(unsigned int x, unsigned int y);
extern void wxt_vector(unsigned int x, unsigned int y);
extern void wxt_put_text(unsigned int x, unsigned int y, const char *str);
extern void wxt_reset(void);
extern int wxt_justify_text(enum JUSTIFY mode);
extern void wxt_point(unsigned int x, unsigned int y, int pointstyle);
extern void wxt_linewidth(double linewidth);
extern int wxt_text_angle(int ang);
extern void wxt_fillbox(int style, unsigned int x1, unsigned int y1, unsigned int width, unsigned int height);
extern int wxt_set_font __PROTO ((const char *font));
extern void wxt_pointsize(double ptsize);
#ifdef WITH_IMAGE
extern void wxt_image(unsigned M, unsigned N, coordval * image, gpiPoint * corner, t_imagecolor color_mode);
#endif /*WITH_IMAGE*/
# ifdef USE_MOUSE
extern int wxt_waitforinput(void);
extern void wxt_put_tmptext(int, const char str[]);
extern void wxt_set_ruler(int x, int y);
extern void wxt_set_cursor(int, int, int);
extern void wxt_set_clipboard(const char s[]);
# endif /*USE_MOUSE*/
extern int wxt_make_palette(t_sm_palette *palette);
extern void wxt_set_color(t_colorspec *colorspec);
extern void wxt_filled_polygon(int n, gpiPoint * corners);

extern void wxt_raise_terminal_window __PROTO((int));
extern void wxt_raise_terminal_group __PROTO((void));
extern void wxt_lower_terminal_window __PROTO((int));
extern void wxt_lower_terminal_group __PROTO((void));
extern void wxt_close_terminal_window __PROTO((int number));
extern void wxt_update_title __PROTO((int number));

/* state variables shared between wxt.trm and wxt_gui.cpp */
extern int wxt_window_number;
extern TBOOLEAN wxt_enhanced_enabled;
extern int wxt_persist;
extern int wxt_raise;
extern char wxt_set_fontname[MAX_ID_LEN + 1];
extern int wxt_set_fontsize;
extern char wxt_title[MAX_ID_LEN + 1];

#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /* gnuplot_wxt_term_h */
