/* $Id: tables.c,v 1.5 1999/08/17 15:52:43 lhecking Exp $ */

/* GNUPLOT - tables.c */

#include "plot.h"
#include "tables.h"

/* gnuplot commands */

/* the actual commands */
struct gen_table command_tbl[] =
{
    { "ca$ll", CMD_CALL },
    { "cd", CMD_CD },
    { "cl$ear", CMD_CLEAR },
    { "ex$it", CMD_EXIT },
    { "f$it", CMD_FIT },
    { "h$elp", CMD_HELP },
    { "?", CMD_HELP },
    { "hi$story", CMD_HISTORY },
    { "if", CMD_IF },
    { "l$oad", CMD_LOAD },
    { "pa$use", CMD_PAUSE },
    { "p$lot", CMD_PLOT },
    { "pr$int", CMD_PRINT },
    { "pwd", CMD_PWD },
    { "q$uit", CMD_EXIT },
    { "rep$lot", CMD_REPLOT },
    { "re$read", CMD_REREAD },
    { "res$et", CMD_RESET },
    { "sa$ve", CMD_SAVE },
    { "scr$eendump", CMD_SCREENDUMP },
    { "se$t", CMD_SET },
    { "she$ll", CMD_SHELL },
    { "sh$ow", CMD_SHOW },
    { "sp$lot", CMD_SPLOT },
    { "sy$stem", CMD_SYSTEM },
    { "test", CMD_TEST },
    { "testt$ime", CMD_TESTTIME },
    { "uns$et", CMD_UNSET },
    { "up$date", CMD_UPDATE },
    { ";", CMD_NULL },
    /* last key must be NULL */
    { NULL, CMD_INVALID }
};

/* 'plot' and 'splot' */
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
    { "hi$dden3d", S_HIDDEN3D },
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

/* 'set hidden3d' options */
struct gen_table set_hidden3d_tbl[] =
{
    { "def$aults", H_DEFAULTS },
    { "off$set", H_OFFSET },
    { "nooff$set", H_NOOFFSET },
    { "tri$anglepattern", H_TRIANGLEPATTERN },
    { "undef$ined", H_UNDEFINED },
    { "nound$efined", H_NOUNDEFINED },
    { "alt$diagonal", H_ALTDIAGONAL },
    { "noalt$diagonal", H_NOALTDIAGONAL },
    { "bent$over", H_BENTOVER },
    { "nobent$over", H_NOBENTOVER },
    { NULL, H_INVALID }
};

/* 'set key' options */
struct gen_table set_key_tbl[] =
{
    { "t$op", KEY_TOP },
    { "b$ottom", KEY_BOTTOM },
    { "l$eft", KEY_LEFT },
    { "r$ight", KEY_RIGHT },
    { "u$nder", KEY_UNDER },
    { "be$low", KEY_UNDER },
    { "o$utside", KEY_OUTSIDE },
    { "L$eft", KEY_LLEFT },
    { "R$ight", KEY_RRIGHT },
    { "rev$erse", KEY_REVERSE },
    { "norev$erse", KEY_NOREVERSE },
    { "b$ox", KEY_BOX },
    { "nob$ox", KEY_NOBOX },
    { "sa$mplen", KEY_SAMPLEN },
    { "sp$acing", KEY_SPACING },
    { "w$idth", KEY_WIDTH },
    { "ti$tle", KEY_TITLE },
    { NULL, KEY_INVALID }
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
lookup_table(tbl, token)
struct gen_table *tbl;
int token;
{
    while (tbl->key) {
	if (almost_equals(token, tbl->key))
	    return tbl->value;
	tbl++;
    }
    return tbl->value; /* *_INVALID */
}
