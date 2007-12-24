#ifndef lint
static char *RCSid() { return RCSid("$Id: tables.c,v 1.76 2007/12/08 10:55:17 mikulik Exp $"); }
#endif

/* GNUPLOT - tables.c */

/*[
 * Copyright 1999, 2004   Lars Hecking
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
# include "getcolor.h"

/* gnuplot commands */

/* the actual commands */
const struct gen_ftable command_ftbl[] =
{
    { "ra$ise", raise_command },
    { "low$er", lower_command },
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
#ifdef VOLATILE_REFRESH
    { "ref$resh", refresh_command },
#endif
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
    { "test", test_command },
    { "uns$et", unset_command },
    { "up$date", update_command },
    { ";", null_command },
    /* last key must be NULL */
    { NULL, invalid_command }
};

/* 'plot' and 'splot' */
/* HBB 990829: unused, yet? */
/* Lars 991108: yes, because the 'plot' parser is a real bitch :( */
/* pm 011129: ...and therefore I'm putting it into #if 0 ... #endif.
 * Anyway, this table can't be used as below because some options
 * belong to the group of data file options and others to the group
 * of plot options
 */
#if 0
const struct gen_table plot_tbl[] =
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
#endif

/* 'plot ax[ei]s' parameter */
const struct gen_table plot_axes_tbl[] =
{
    { "x1y1", AXES_X1Y1 },
    { "x2y2", AXES_X2Y2 },
    { "x1y2", AXES_X1Y2 },
    { "x2y1", AXES_X2Y1 },
    { NULL, AXES_NONE }
};

/* 'plot smooth' parameter */
const struct gen_table plot_smooth_tbl[] =
{
    { "a$csplines", SMOOTH_ACSPLINES },
    { "b$ezier", SMOOTH_BEZIER },
    { "c$splines", SMOOTH_CSPLINES },
    { "s$bezier", SMOOTH_SBEZIER },
    { "u$nique", SMOOTH_UNIQUE },
    { "f$requency", SMOOTH_FREQUENCY },
    { NULL, SMOOTH_NONE }
};

/* 'save' command */
const struct gen_table save_tbl[] =
{
    { "f$unctions", SAVE_FUNCS },
    { "s$et", SAVE_SET },
    { "t$erminal", SAVE_TERMINAL },
    { "v$ariables", SAVE_VARS },
    { NULL, SAVE_INVALID }
};

/* 'set' and 'show' commands */
const struct gen_table set_tbl[] =
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

    { "data$file", S_DATAFILE },

    { "dg$rid3d", S_DGRID3D },
    { "du$mmy", S_DUMMY },
    { "enc$oding", S_ENCODING },
    { "dec$imalsign", S_DECIMALSIGN },
    { "fit", S_FIT },
    { "font$path", S_FONTPATH },
    { "fo$rmat", S_FORMAT },
    { "fu$nction", S_FUNCTIONS },
    { "fu$nctions", S_FUNCTIONS },
    { "g$rid", S_GRID },
    { "hid$den3d", S_HIDDEN3D },
    { "his$torysize", S_HISTORYSIZE },
    { "is$osamples", S_ISOSAMPLES },
    { "k$ey", S_KEY },
    { "keyt$itle", S_KEYTITLE },
    { "la$bel", S_LABEL },
    { "li$nestyle", S_LINESTYLE },
    { "ls", S_LINESTYLE },
    { "loa$dpath", S_LOADPATH },
    { "loc$ale", S_LOCALE },
    { "log$scale", S_LOGSCALE },
#ifdef GP_MACROS
    { "mac$ros", S_MACROS },
#endif
    { "map$ping", S_MAPPING },
    { "map$ping3d", S_MAPPING },

    { "mar$gin", S_MARGIN },
    { "lmar$gin", S_LMARGIN },
    { "rmar$gin", S_RMARGIN },
    { "tmar$gin", S_TMARGIN },
    { "bmar$gin", S_BMARGIN },

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
    { "mcbt$ics", S_MCBTICS },
    { "nomcbt$ics", S_NOMCBTICS },
    { "of$fsets", S_OFFSETS },
    { "or$igin", S_ORIGIN },
    { "o$utput", SET_OUTPUT },
    { "pa$rametric", S_PARAMETRIC },
    { "pm$3d", S_PM3D },
    { "pal$ette", S_PALETTE },
    { "colorb$ox", S_COLORBOX },
    { "p$lot", S_PLOT },
    { "poi$ntsize", S_POINTSIZE },
    { "pol$ar", S_POLAR },
    { "pr$int", S_PRINT },
    { "obj$ect", S_OBJECT },
    { "sa$mples", S_SAMPLES },
    { "si$ze", S_SIZE },
    { "st$yle", S_STYLE },
    { "su$rface", S_SURFACE },
    { "table", S_TABLE },
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
    { "xyp$lane", S_XYPLANE },

    { "xda$ta", S_XDATA },
    { "x2da$ta", S_X2DATA },
    { "yda$ta", S_YDATA },
    { "y2da$ta", S_Y2DATA },
    { "zda$ta", S_ZDATA },
    { "cbda$ta", S_CBDATA },

    { "xl$abel", S_XLABEL },
    { "x2l$abel", S_X2LABEL },
    { "yl$abel", S_YLABEL },
    { "y2l$abel", S_Y2LABEL },
    { "zl$abel", S_ZLABEL },
    { "cbl$abel", S_CBLABEL },

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
    { "cbti$cs", S_CBTICS },
    { "nocbti$cs", S_NOCBTICS },

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
    { "cbdti$cs", S_CBDTICS },
    { "nocbdti$cs", S_NOCBDTICS },

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
    { "cbmti$cs", S_CBMTICS },
    { "nocbmti$cs", S_NOCBMTICS },

    { "xr$ange", S_XRANGE },
    { "x2r$ange", S_X2RANGE },
    { "yr$ange", S_YRANGE },
    { "y2r$ange", S_Y2RANGE },
    { "zr$ange", S_ZRANGE },
    { "cbr$ange", S_CBRANGE },
    { "rr$ange", S_RRANGE },
    { "tr$ange", S_TRANGE },
    { "ur$ange", S_URANGE },
    { "vr$ange", S_VRANGE },

    { "xzeroa$xis", S_XZEROAXIS },
    { "x2zeroa$xis", S_X2ZEROAXIS },
    { "yzeroa$xis", S_YZEROAXIS },
    { "y2zeroa$xis", S_Y2ZEROAXIS },
    { "zzeroa$xis", S_ZZEROAXIS },
    { "zeroa$xis", S_ZEROAXIS },

    { "z$ero", S_ZERO },
    { NULL, S_INVALID }
};

/* 'set hidden3d' options */
const struct gen_table set_hidden3d_tbl[] =
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
    { "front", S_HI_FRONT },
    { "back", S_HI_BACK },
    { NULL, S_HI_INVALID }
};

/* 'set key' options */
const struct gen_table set_key_tbl[] =
{
    { "def$ault", S_KEY_DEFAULT },
    { "on", S_KEY_ON },
    { "off", S_KEY_OFF },
    { "t$op", S_KEY_TOP },
    { "b$ottom", S_KEY_BOTTOM },
    { "l$eft", S_KEY_LEFT },
    { "r$ight", S_KEY_RIGHT },
    { "c$enter", S_KEY_CENTER },
    { "ver$tical", S_KEY_VERTICAL },
    { "hor$izontal", S_KEY_HORIZONTAL },
    { "ov$er", S_KEY_OVER },
    { "ab$ove", S_KEY_ABOVE },
    { "u$nder", S_KEY_UNDER },
    { "be$low", S_KEY_BELOW },
    { "at", S_KEY_MANUAL },
    { "ins$ide", S_KEY_INSIDE },
    { "o$utside", S_KEY_OUTSIDE },
    { "tm$argin", S_KEY_TMARGIN },
    { "bm$argin", S_KEY_BMARGIN },
    { "lm$argin", S_KEY_LMARGIN },
    { "rm$argin", S_KEY_RMARGIN },
    { "L$eft", S_KEY_LLEFT },
    { "R$ight", S_KEY_RRIGHT },
    { "rev$erse", S_KEY_REVERSE },
    { "norev$erse", S_KEY_NOREVERSE },
    { "inv$ert", S_KEY_INVERT },
    { "noinv$ert", S_KEY_NOINVERT },
    { "enh$anced", S_KEY_ENHANCED },
    { "noenh$anced", S_KEY_NOENHANCED },
    { "b$ox", S_KEY_BOX },
    { "nob$ox", S_KEY_NOBOX },
    { "sa$mplen", S_KEY_SAMPLEN },
    { "sp$acing", S_KEY_SPACING },
    { "w$idth", S_KEY_WIDTH },
    { "h$eight", S_KEY_HEIGHT },
    { "a$utotitles", S_KEY_AUTOTITLES },
    { "noa$utotitles", S_KEY_NOAUTOTITLES },
    { "ti$tle", S_KEY_TITLE },
    { "font", S_KEY_FONT },
    { "tc", S_KEY_TEXTCOLOR },
    { "text$color", S_KEY_TEXTCOLOR },
    { NULL, S_KEY_INVALID }
};

/* 'test' command */
const struct gen_table test_tbl[] =
{
    { "term$inal", TEST_TERMINAL },
    { "pal$ette", TEST_PALETTE },
    { "time", TEST_TIME },
    { NULL, TEST_INVALID }
};

/* 'set colorbox' options */
const struct gen_table set_colorbox_tbl[] =
{
    { "v$ertical",	S_COLORBOX_VERTICAL },
    { "h$orizontal",	S_COLORBOX_HORIZONTAL },
    { "def$ault",	S_COLORBOX_DEFAULT },
    { "u$ser",		S_COLORBOX_USER },
    { "bo$rder",	S_COLORBOX_BORDER },
    { "bd$efault",	S_COLORBOX_BDEFAULT },
    { "nobo$rder",	S_COLORBOX_NOBORDER },
    { "o$rigin",	S_COLORBOX_ORIGIN },
    { "s$ize",		S_COLORBOX_SIZE },
    { "fr$ont",		S_COLORBOX_FRONT },
    { "ba$ck",		S_COLORBOX_BACK },
    { NULL, S_COLORBOX_INVALID }
};

/* 'set palette' options */
const struct gen_table set_palette_tbl[] =
{
    { "pos$itive",	S_PALETTE_POSITIVE },
    { "neg$ative",	S_PALETTE_NEGATIVE },
    { "gray",		S_PALETTE_GRAY },
    { "grey",		S_PALETTE_GRAY },
    { "col$or",		S_PALETTE_COLOR },
    { "rgb$formulae",	S_PALETTE_RGBFORMULAE },
    { "def$ined",       S_PALETTE_DEFINED },
    { "file",           S_PALETTE_FILE },
    { "func$tions",     S_PALETTE_FUNCTIONS },
    { "mo$del",         S_PALETTE_MODEL },
    { "nops_allcF",	S_PALETTE_NOPS_ALLCF },
    { "ps_allcF",	S_PALETTE_PS_ALLCF },
    { "maxc$olors",	S_PALETTE_MAXCOLORS },
    { "gam$ma",         S_PALETTE_GAMMA },
    { NULL, S_PALETTE_INVALID }
};


const struct gen_table color_model_tbl[] =
{
    { "RGB", C_MODEL_RGB },
    { "HSV", C_MODEL_HSV },
    { "CMY", C_MODEL_CMY },
    { "YIQ", C_MODEL_YIQ },
    { "XYZ", C_MODEL_XYZ },
    { NULL,  -1 }
};

/* 'set pm3d' options */
const struct gen_table set_pm3d_tbl[] =
{
    { "at",		S_PM3D_AT },
    { "interp$olate",	S_PM3D_INTERPOLATE },
    { "scansfor$ward",	S_PM3D_SCANSFORWARD },
    { "scansback$ward", S_PM3D_SCANSBACKWARD },
    { "scansauto$matic",S_PM3D_SCANS_AUTOMATIC },
    { "dep$thorder",    S_PM3D_DEPTH },
    { "fl$ush",		S_PM3D_FLUSH },
    { "ftr$iangles",	S_PM3D_FTRIANGLES },
    { "noftr$iangles",	S_PM3D_NOFTRIANGLES },
    { "clip1$in",	S_PM3D_CLIP_1IN },
    { "clip4$in",	S_PM3D_CLIP_4IN },
    { "map", 		S_PM3D_MAP },
    { "hi$dden3d",	S_PM3D_HIDDEN },
    { "nohi$dden3d",	S_PM3D_NOHIDDEN },
    { "so$lid",		S_PM3D_SOLID },
    { "notr$ansparent",	S_PM3D_NOTRANSPARENT },
    { "noso$lid",	S_PM3D_NOSOLID },
    { "tr$ansparent",	S_PM3D_TRANSPARENT },
    { "i$mplicit",	S_PM3D_IMPLICIT },
    { "noe$xplicit",	S_PM3D_NOEXPLICIT },
    { "noi$mplicit",	S_PM3D_NOIMPLICIT },
    { "e$xplicit",	S_PM3D_EXPLICIT },
    { "corners2c$olor",S_PM3D_WHICH_CORNER },
    { NULL, S_PM3D_INVALID }
};

/* fixed RGB color names for 'set palette defined' */
const struct gen_table pm3d_color_names_tbl[] =
{
    /* black, white and gray/grey */
    { "white"           , 255*(1<<16) + 255*(1<<8) + 255 },
    { "black"           ,   0*(1<<16) +   0*(1<<8) +   0 },
    { "gray0"           ,   0*(1<<16) +   0*(1<<8) +   0 },
    { "grey0"           ,   0*(1<<16) +   0*(1<<8) +   0 },
    { "gray10"          ,  26*(1<<16) +  26*(1<<8) +  26 },
    { "grey10"          ,  26*(1<<16) +  26*(1<<8) +  26 },
    { "gray20"          ,  51*(1<<16) +  51*(1<<8) +  51 },
    { "grey20"          ,  51*(1<<16) +  51*(1<<8) +  51 },
    { "gray30"          ,  77*(1<<16) +  77*(1<<8) +  77 },
    { "grey30"          ,  77*(1<<16) +  77*(1<<8) +  77 },
    { "gray40"          , 102*(1<<16) + 102*(1<<8) + 102 },
    { "grey40"          , 102*(1<<16) + 102*(1<<8) + 102 },
    { "gray50"          , 127*(1<<16) + 127*(1<<8) + 127 },
    { "grey50"          , 127*(1<<16) + 127*(1<<8) + 127 },
    { "gray60"          , 153*(1<<16) + 153*(1<<8) + 153 },
    { "grey60"          , 153*(1<<16) + 153*(1<<8) + 153 },
    { "gray70"          , 179*(1<<16) + 179*(1<<8) + 179 },
    { "grey70"          , 179*(1<<16) + 179*(1<<8) + 179 },
    { "gray80"          , 204*(1<<16) + 204*(1<<8) + 204 },
    { "grey80"          , 204*(1<<16) + 204*(1<<8) + 204 },
    { "gray90"          , 229*(1<<16) + 229*(1<<8) + 229 },
    { "grey90"          , 229*(1<<16) + 229*(1<<8) + 229 },
    { "gray100"         , 255*(1<<16) + 255*(1<<8) + 255 },
    { "grey100"         , 255*(1<<16) + 255*(1<<8) + 255 },
    { "gray"            , 190*(1<<16) + 190*(1<<8) + 190 },
    { "grey"            , 190*(1<<16) + 190*(1<<8) + 190 },
    { "light-gray"      , 211*(1<<16) + 211*(1<<8) + 211 },
    { "light-grey"      , 211*(1<<16) + 211*(1<<8) + 211 },
    { "dark-gray"       , 169*(1<<16) + 169*(1<<8) + 169 },
    { "dark-grey"       , 169*(1<<16) + 169*(1<<8) + 169 },
    /* red */
    { "red"             , 255*(1<<16) +   0*(1<<8) +   0 },
    { "light-red"       , 240*(1<<16) +  50*(1<<8) +  50 },
    { "dark-red"        , 139*(1<<16) +   0*(1<<8) +   0 },
    /* yellow */
    { "yellow"          , 255*(1<<16) + 255*(1<<8) +   0 },
    { "light-yellow"    , 255*(1<<16) + 255*(1<<8) + 224 },
    { "dark-yellow"     , 200*(1<<16) + 200*(1<<8) +   0 },
    /* green */
    { "green"           ,   0*(1<<16) + 255*(1<<8) +   0 },
    { "light-green"     , 144*(1<<16) + 238*(1<<8) + 144 },
    { "dark-green"      ,   0*(1<<16) + 100*(1<<8) +   0 },
    { "spring-green"    ,   0*(1<<16) + 255*(1<<8) + 127 },
    { "forest-green"    ,  34*(1<<16) + 139*(1<<8) +  34 },
    { "sea-green"       ,  46*(1<<16) + 139*(1<<8) +  87 },
    /* blue */
    { "blue"            ,   0*(1<<16) +   0*(1<<8) + 255 },
    { "light-blue"      , 173*(1<<16) + 216*(1<<8) + 230 },
    { "dark-blue"       ,   0*(1<<16) +   0*(1<<8) + 139 },
    { "midnight-blue"   ,  25*(1<<16) +  25*(1<<8) + 112 },
    { "navy"            ,   0*(1<<16) +   0*(1<<8) + 128 },
    { "medium-blue"     ,   0*(1<<16) +   0*(1<<8) + 205 },
    { "royalblue"       ,  65*(1<<16) + 105*(1<<8) + 225 },
    { "skyblue"         , 135*(1<<16) + 206*(1<<8) + 235 },
    /* cyan */
    { "cyan"            ,   0*(1<<16) + 255*(1<<8) + 255 },
    { "light-cyan"      , 224*(1<<16) + 255*(1<<8) + 255 },
    { "dark-cyan"       ,   0*(1<<16) + 139*(1<<8) + 139 },
    /* magenta */
    { "magenta"         , 255*(1<<16) +   0*(1<<8) + 255 },
    { "light-magenta"   , 240*(1<<16) +  85*(1<<8) + 240 },
    { "dark-magenta"    , 139*(1<<16) +   0*(1<<8) + 139 },
    /* turquoise */
    { "turquoise"       ,  64*(1<<16) + 224*(1<<8) + 208 },
    { "light-turquoise" , 175*(1<<16) + 238*(1<<8) + 238 },
    { "dark-turquoise"  ,   0*(1<<16) + 206*(1<<8) + 209 },
    /* pink */
    { "pink"            , 255*(1<<16) + 192*(1<<8) + 203 },
    { "light-pink"      , 255*(1<<16) + 182*(1<<8) + 193 },
    { "dark-pink"       , 255*(1<<16) +  20*(1<<8) + 147 },
    /* coral */
    { "coral"           , 255*(1<<16) + 127*(1<<8) +  80 },
    { "light-coral"     , 240*(1<<16) + 128*(1<<8) + 128 },
    { "orange-red"      , 255*(1<<16) +  69*(1<<8) +   0 },
    /* salmon */
    { "salmon"          , 250*(1<<16) + 128*(1<<8) + 114 },
    { "light-salmon"    , 255*(1<<16) + 160*(1<<8) + 122 },
    { "dark-salmon"     , 233*(1<<16) + 150*(1<<8) + 122 },
    /* some more */
    { "aquamarine"      , 127*(1<<16) + 255*(1<<8) + 212 },
    { "khaki"           , 240*(1<<16) + 230*(1<<8) + 140 },
    { "dark-khaki"      , 189*(1<<16) + 183*(1<<8) + 107 },
    { "goldenrod"       , 218*(1<<16) + 165*(1<<8) +  32 },
    { "light-goldenrod" , 238*(1<<16) + 221*(1<<8) + 130 },
    { "dark-goldenrod"  , 184*(1<<16) + 134*(1<<8) +  11 },
    { "gold"            , 255*(1<<16) + 215*(1<<8) +   0 },
    { "beige"           , 245*(1<<16) + 245*(1<<8) + 220 },
    { "brown"           , 165*(1<<16) +  42*(1<<8) +  42 },
    { "orange"          , 255*(1<<16) + 165*(1<<8) +   0 },
    { "dark-orange"     , 255*(1<<16) + 140*(1<<8) +   0 },
    { "violet"          , 238*(1<<16) + 130*(1<<8) + 238 },
    { "dark-violet"     , 148*(1<<16) +   0*(1<<8) + 211 },
    { "plum"            , 221*(1<<16) + 160*(1<<8) + 221 },
    { "purple"          , 160*(1<<16) +  32*(1<<8) + 240 },
    { NULL, -1 }
};

const struct gen_table show_style_tbl[] =
{
    { "d$ata", SHOW_STYLE_DATA },
    { "f$unction", SHOW_STYLE_FUNCTION },
    { "l$ine", SHOW_STYLE_LINE },
    { "fill", SHOW_STYLE_FILLING },
    { "fs", SHOW_STYLE_FILLING },
    { "ar$row", SHOW_STYLE_ARROW },
    { "incr$ement", SHOW_STYLE_INCREMENT },
#ifdef EAM_HISTOGRAMS
    { "hist$ogram", SHOW_STYLE_HISTOGRAM },
#endif
    { "rect$angle", SHOW_STYLE_RECTANGLE },
    { NULL, SHOW_STYLE_INVALID }
};

const struct gen_table plotstyle_tbl[] =
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
#ifdef EAM_HISTOGRAMS
    { "hist$ograms", HISTOGRAMS },
#endif
    { "filledc$urves", FILLEDCURVES },
    { "boxer$rorbars", BOXERROR },
    { "boxx$yerrorbars", BOXXYERROR },
    { "st$eps", STEPS },
    { "fs$teps", FSTEPS },
    { "his$teps", HISTEPS },
    { "vec$tors", VECTOR },
    { "fin$ancebars", FINANCEBARS },
    { "can$dlesticks", CANDLESTICKS },
    { "pm$3d", PM3DSURFACE },
#ifdef EAM_DATASTRINGS
    { "labels", LABELPOINTS },
#endif
#ifdef WITH_IMAGE
    { "ima$ge", IMAGE },
    { "rgbima$ge", RGBIMAGE },
#endif
    { NULL, -1 }
};

const struct gen_table filledcurves_opts_tbl[] =
{
    { "c$losed", FILLEDCURVES_CLOSED },
    { "x1", FILLEDCURVES_X1 },
    { "y1", FILLEDCURVES_Y1 },
    { "x2", FILLEDCURVES_X2 },
    { "y2", FILLEDCURVES_Y2 },
    { "xy", FILLEDCURVES_ATXY },
    { "above", FILLEDCURVES_ABOVE },
    { "below", FILLEDCURVES_BELOW },
    { NULL, -1 }
};

int
lookup_table(const struct gen_table *tbl, int find_token)
{
    while (tbl->key) {
	if (almost_equals(find_token, tbl->key))
	    return tbl->value;
	tbl++;
    }
    return tbl->value; /* *_INVALID */
}

parsefuncp_t
lookup_ftable(const struct gen_ftable *ftbl, int find_token)
{
    while (ftbl->key) {
	if (almost_equals(find_token, ftbl->key))
	    return ftbl->value;
	ftbl++;
    }
    return ftbl->value;
}

/* Returns index of the table tbl whose key matches the beginning of the
 * search string search_str.
 * It returns index into the table or -1 if there is no match.
 */
int
lookup_table_nth(const struct gen_table *tbl, const char *search_str)
{
    int k = -1;
    while (tbl[++k].key)
	if (tbl[k].key && !strncmp(search_str, tbl[k].key, strlen(tbl[k].key)))
	    return k;
    return -1; /* not found */
}

/* Returns index of the table tbl whose key matches the beginning of the
 * search string search_str. The table_len is necessary because the table
 * is searched in the reversed order. The routine is used in parsing commands
 * '(un)set log x2zcb', for instance.
 * It returns index into the table or -1 if there is no match.
 */
int
lookup_table_nth_reverse(
    const struct gen_table *tbl,
    int table_len,
    const char *search_str)
{
    while (--table_len >= 0) {
	if (tbl[table_len].key && !strncmp(search_str, tbl[table_len].key, strlen(tbl[table_len].key)))
	    return table_len;
    }
    return -1; /* not found */
}

/* Returns the key associated with this indexed value
 * or NULL if the key/value pair is not found.
 */
const char *
reverse_table_lookup(const struct gen_table *tbl, int entry)
{
    int k = -1;
    while (tbl[++k].key)
	if (tbl[k].value == entry)
	    return(tbl[k].key);
    return NULL;
}
