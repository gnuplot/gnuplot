#ifndef lint
static char *RCSid() { return RCSid("$Id: tables.c,v 1.13.2.2 2000/05/03 21:26:12 joze Exp $"); }
#endif

/* GNUPLOT - tables.c */

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

#include "tables.h"

#include "command.h"
#include "fit.h"
#include "setshow.h"
#include "term_api.h"
#include "util.h"

/* gnuplot commands */

/* the actual commands */
struct gen_ftable command_ftbl[] =
{
#ifdef USE_MOUSE
    { "bi$nd", bind_command },
#endif
    { "ca$ll", call_command },
    { "cd", changedir_command },
    { "cl$ear", clear_command },
    { "ex$it", exit_command },
    { "f$it", fit_command },
    { "h$elp", help_command },
    { "?", help_command },
    { "hi$story", history_command },
    { "if", if_command },
    { "else", else_command },
    { "l$oad", load_command },
    { "pa$use", pause_command },
    { "p$lot", plot_command },
    { "pr$int", print_command },
    { "pwd", pwd_command },
    { "q$uit", exit_command },
    { "rep$lot", replot_command },
    { "re$read", reread_command },
    { "res$et", reset_command },
    { "sa$ve", save_command },
    { "scr$eendump", screendump_command },
    { "se$t", set_command },
    { "she$ll", do_shell },
    { "sh$ow", show_command },
    { "sp$lot", splot_command },
    { "sy$stem", system_command },
    { "test", test_term },
    { "testt$ime", testtime_command },
    { "uns$et", unset_command },
    { "up$date", update_command },
    { ";", null_command },
    /* last key must be NULL */
    { NULL, invalid_command }
};

/* 'plot' and 'splot' */
/* HBB 990829: unused, yet? */
/* Lars 991108: yes, because the 'plot' parser is a real bitch :( */
struct gen_table plot_tbl[] =
{
    { "ax$es", P_AXES },
    { "ax$is", P_AXES },
    { "bin$ary", P_BINARY },
    { "ev$ery", P_EVERY },
    { "i$ndex", P_INDEX },
    { "mat$rix", P_MATRIX },
    { "s$mooth", P_SMOOTH },
    { "thru$", P_THRU },
    { "t$itle", P_TITLE },
    { "not$itle", P_NOTITLE },
    { "u$sing", P_USING },
    { "w$ith", P_WITH },
    { NULL, P_INVALID }
};

/* 'plot ax[ei]s' parameter */
struct gen_table plot_axes_tbl[] =
{
    { "x1y1", AXES_X1Y1 },
    { "x2y2", AXES_X2Y2 },
    { "x1y2", AXES_X1Y2 },
    { "x2y1", AXES_X2Y1 },
    { NULL, AXES_NONE }
};

/* 'plot smooth' parameter */
struct gen_table plot_smooth_tbl[] =
{
    { "a$csplines", SMOOTH_ACSPLINES },
    { "b$ezier", SMOOTH_BEZIER },
    { "c$splines", SMOOTH_CSPLINES },
    { "s$bezier", SMOOTH_SBEZIER },
    { "u$nique", SMOOTH_UNIQUE },
    { NULL, SMOOTH_NONE }
};

/* 'save' command */
struct gen_table save_tbl[] =
{
    { "f$unctions", SAVE_FUNCS },
    { "s$et", SAVE_SET },
    { "t$erminal", SAVE_TERMINAL },
    { "v$ariables", SAVE_VARS },
    { NULL, SAVE_INVALID }
};

/* 'set' and 'show' commands */
struct gen_table set_tbl[] =
{
    { "a$ll", S_ALL },
    { "ac$tion_table", S_ACTIONTABLE },
    { "at", S_ACTIONTABLE },
    { "an$gles", S_ANGLES },
    { "ar$row", S_ARROW }, 
    { "au$toscale", S_AUTOSCALE }, 
    { "b$ars", S_BARS },
    { "bor$der", S_BORDER },
    { "box$width", S_BOXWIDTH }, 
    { "cl$abel", S_CLABEL },
    { "c$lip", S_CLIP },
    { "cn$trparam", S_CNTRPARAM },
    { "co$ntour", S_CONTOUR },
    { "da$ta", S_DATA },
    { "dg$rid3d", S_DGRID3D },
    { "du$mmy", S_DUMMY },
    { "enc$oding", S_ENCODING },
    { "fo$rmat", S_FORMAT },
    { "fu$nction", S_FUNCTIONS },
    { "fu$nctions", S_FUNCTIONS },
    { "g$rid", S_GRID },
    { "hid$den3d", S_HIDDEN3D },
#if defined(HAVE_LIBREADLINE) && defined(GNUPLOT_HISTORY)
    { "his$torysize", S_HISTORYSIZE },
#endif
    { "is$osamples", S_ISOSAMPLES },
    { "k$ey", S_KEY },
    { "keyt$itle", S_KEYTITLE },
    { "la$bel", S_LABEL },
    { "li$nestyle", S_LINESTYLE },
    { "ls", S_LINESTYLE },
    { "loa$dpath", S_LOADPATH },
    { "loc$ale", S_LOCALE },
    { "log$scale", S_LOGSCALE },
    { "map$ping", S_MAPPING },
    { "map$ping3d", S_MAPPING },

    { "mar$gin", S_MARGIN },
    { "lmar$gin", S_LMARGIN },
    { "rmar$gin", S_RMARGIN },
    { "tmar$gin", S_TMARGIN },
    { "bmar$gin", S_BMARGIN },

    { "mis$sing", S_MISSING },
#ifdef USE_MOUSE
    { "mo$use", S_MOUSE },
#endif
    { "multi$plot", S_MULTIPLOT },

    { "mxt$ics", S_MXTICS },
    { "nomxt$ics", S_NOMXTICS },
    { "mx2t$ics", S_MX2TICS },
    { "nomx2t$ics", S_NOMX2TICS },
    { "myt$ics", S_MYTICS },
    { "nomyt$ics", S_NOMYTICS },
    { "my2t$ics", S_MY2TICS },
    { "nomy2t$ics", S_NOMY2TICS },
    { "mzt$ics", S_MZTICS },
    { "nomzt$ics", S_NOMZTICS },

    { "of$fsets", S_OFFSETS },
    { "or$igin", S_ORIGIN },
    { "o$utput", S_OUTPUT },
    { "pa$rametric", S_PARAMETRIC },
#ifdef PM3D
    { "pal$ette", S_PALETTE },
    { "pm$3d", S_PM3D },
#endif
    { "p$lot", S_PLOT },
    { "poi$ntsize", S_POINTSIZE },
    { "pol$ar", S_POLAR },
    { "sa$mples", S_SAMPLES },
    { "si$ze", S_SIZE },
    { "st$yle", S_STYLE },
    { "su$rface", S_SURFACE },
    { "t$erminal", S_TERMINAL },
    { "termo$ptions", S_TERMOPTIONS },
    { "ti$cs", S_TICS },
    { "ticsc$ale", S_TICSCALE },
    { "ticsl$evel", S_TICSLEVEL },
    { "timef$mt", S_TIMEFMT },
    { "tim$estamp", S_TIMESTAMP },
    { "tit$le", S_TITLE },
    { "v$ariables", S_VARIABLES },
    { "ve$rsion", S_VERSION },
    { "vi$ew", S_VIEW },

    { "xda$ta", S_XDATA },
    { "x2da$ta", S_X2DATA },
    { "yda$ta", S_YDATA },
    { "y2da$ta", S_Y2DATA },
    { "zda$ta", S_YDATA },

    { "xl$abel", S_XLABEL },
    { "x2l$abel", S_X2LABEL },
    { "yl$abel", S_YLABEL },
    { "y2l$abel", S_Y2LABEL },
    { "zl$abel", S_ZLABEL },

    { "xti$cs", S_XTICS },
    { "noxti$cs", S_NOXTICS },
    { "x2ti$cs", S_X2TICS },
    { "nox2ti$cs", S_NOX2TICS },
    { "yti$cs", S_YTICS },
    { "noyti$cs", S_NOYTICS },
    { "y2ti$cs", S_Y2TICS },
    { "noy2ti$cs", S_NOY2TICS },
    { "zti$cs", S_ZTICS },
    { "nozti$cs", S_NOZTICS },

    { "xdti$cs", S_XDTICS },
    { "noxdti$cs", S_NOXDTICS },
    { "x2dti$cs", S_X2DTICS },
    { "nox2dti$cs", S_NOX2DTICS },
    { "ydti$cs", S_YDTICS },
    { "noydti$cs", S_NOYDTICS },
    { "y2dti$cs", S_Y2DTICS },
    { "noy2dti$cs", S_NOY2DTICS },
    { "zdti$cs", S_ZDTICS },
    { "nozdti$cs", S_NOZDTICS },

    { "xmti$cs", S_XMTICS },
    { "noxmti$cs", S_NOXMTICS },
    { "x2mti$cs", S_X2MTICS },
    { "nox2mti$cs", S_NOX2MTICS },
    { "ymti$cs", S_YMTICS },
    { "noymti$cs", S_NOYMTICS },
    { "y2mti$cs", S_Y2MTICS },
    { "noy2mti$cs", S_NOY2MTICS },
    { "zmti$cs", S_ZMTICS },
    { "nozmti$cs", S_NOZMTICS },

    { "xr$ange", S_XRANGE },
    { "x2r$ange", S_X2RANGE },
    { "yr$ange", S_YRANGE },
    { "y2r$ange", S_Y2RANGE },
    { "zr$ange", S_ZRANGE },
    { "rr$ange", S_RRANGE },
    { "tr$ange", S_TRANGE },
    { "ur$ange", S_URANGE },
    { "vr$ange", S_VRANGE },

    { "xzeroa$xis", S_XZEROAXIS },
    { "x2zeroa$xis", S_XZEROAXIS },
    { "yzeroa$xis", S_YZEROAXIS },
    { "y2zeroa$xis", S_YZEROAXIS },
    { "zeroa$xis", S_ZEROAXIS },

    { "z$ero", S_ZERO },
    { NULL, S_INVALID }
};

/* 'set encoding' options */
struct gen_table set_encoding_tbl[] =
{
    { "def$ault", S_ENC_DEFAULT },
    { "iso$_8859_1", S_ENC_ISO8859_1 },
    { "cp4$37", S_ENC_CP437 },
    { "cp8$50", S_ENC_CP850 },
    { NULL, S_ENC_INVALID }
};

/* 'set hidden3d' options */
struct gen_table set_hidden3d_tbl[] =
{
    { "def$aults", S_HI_DEFAULTS },
    { "off$set", S_HI_OFFSET },
    { "nooff$set", S_HI_NOOFFSET },
    { "tri$anglepattern", S_HI_TRIANGLEPATTERN },
    { "undef$ined", S_HI_UNDEFINED },
    { "nound$efined", S_HI_NOUNDEFINED },
    { "alt$diagonal", S_HI_ALTDIAGONAL },
    { "noalt$diagonal", S_HI_NOALTDIAGONAL },
    { "bent$over", S_HI_BENTOVER },
    { "nobent$over", S_HI_NOBENTOVER },
    { NULL, S_HI_INVALID }
};

/* 'set key' options */
struct gen_table set_key_tbl[] =
{
    { "t$op", S_KEY_TOP },
    { "b$ottom", S_KEY_BOTTOM },
    { "l$eft", S_KEY_LEFT },
    { "r$ight", S_KEY_RIGHT },
    { "u$nder", S_KEY_UNDER },
    { "be$low", S_KEY_UNDER },
    { "o$utside", S_KEY_OUTSIDE },
    { "L$eft", S_KEY_LLEFT },
    { "R$ight", S_KEY_RRIGHT },
    { "rev$erse", S_KEY_REVERSE },
    { "norev$erse", S_KEY_NOREVERSE },
    { "b$ox", S_KEY_BOX },
    { "nob$ox", S_KEY_NOBOX },
    { "sa$mplen", S_KEY_SAMPLEN },
    { "sp$acing", S_KEY_SPACING },
    { "w$idth", S_KEY_WIDTH },
    { "ti$tle", S_KEY_TITLE },
    { NULL, S_KEY_INVALID }
};

struct gen_table show_style_tbl[] =
{
    { "d$ata", SHOW_STYLE_DATA },
    { "f$unction", SHOW_STYLE_FUNCTION },
    { "l$ine", SHOW_STYLE_LINE },
    { NULL, SHOW_STYLE_INVALID }
};

struct gen_table plotstyle_tbl[] =
{
    { "l$ines", LINES },
    { "i$mpulses", IMPULSES },
    { "p$oints", POINTSTYLE },
    { "linesp$oints", LINESPOINTS },
    { "lp", LINESPOINTS },
    { "d$ots", DOTS },
    { "yerrorl$ines", YERRORLINES },
    { "errorl$ines", YERRORLINES },
    { "xerrorl$ines", XERRORLINES },
    { "xyerrorl$ines", XYERRORLINES },
    { "ye$rrorbars", YERRORBARS },
    { "e$rrorbars", YERRORBARS },
    { "xe$rrorbars", XERRORBARS },
    { "xye$rrorbars", XYERRORBARS },
    { "boxes", BOXES },
    { "boxer$rorbars", BOXERROR },
    { "boxx$yerrorbars", BOXXYERROR },
    { "st$eps", STEPS },
    { "fs$teps", FSTEPS },
    { "his$teps", HISTEPS },
    { "vec$tor", VECTOR },
    { "fin$ancebars", FINANCEBARS },
    { "can$dlesticks", CANDLESTICKS },
    { NULL, -1 }
};

int
lookup_table(tbl, find_token)
struct gen_table *tbl;
int find_token;
{
    while (tbl->key) {
	if (almost_equals(find_token, tbl->key))
	    return tbl->value;
	tbl++;
    }
    return tbl->value; /* *_INVALID */
}

parsefuncp_t
lookup_ftable(ftbl, find_token)
struct gen_ftable *ftbl;
int find_token;
{
    while (ftbl->key) {
	if (almost_equals(find_token, ftbl->key))
	    return ftbl->value;
	ftbl++;
    }
    return ftbl->value;
}
