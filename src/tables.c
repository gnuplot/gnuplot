/* $Id: tables.c,v 1.2 1999/08/08 17:07:08 lhecking Exp $ */

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
    { "up$date", CMD_UPDATE },
    { ";", CMD_NULL },
    /* last key must be NULL */
    { NULL, CMD_INVALID }
};

/* 'save' command */
struct gen_table save_tbl[] =
{
    { "f$unctions", SAVE_FUNCS },
    { "s$et", SAVE_SET },
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
    { "noar$row", S_NOARROW },
    { "au$toscale", S_AUTOSCALE },
    { "noau$toscale", S_NOAUTOSCALE },
    { "b$ars", S_BARS },
    { "nob$ars", S_NOBARS },
    { "bor$der", S_BORDER },
    { "nobor$der", S_NOBORDER },
    { "box$width", S_BOXWIDTH },
    { "cl$abel", S_CLABEL },
    { "nocl$abel", S_NOCLABEL },
    { "c$lip", S_CLIP },
    { "noc$lip", S_NOCLIP },
    { "cn$trparam", S_CNTRPARAM },
    { "co$ntour", S_CONTOUR },
    { "noco$ntour", S_NOCONTOUR },
    { "da$ta", S_DATA },
    { "dg$rid3d", S_DGRID3D },
    { "nodg$rid3d", S_NODGRID3D },
    { "du$mmy", S_DUMMY },
    { "enc$oding", S_ENCODING },
    { "fo$rmat", S_FORMAT },
    { "fu$nction", S_FUNCTIONS },
    { "fu$nctions", S_FUNCTIONS },
    { "g$rid", S_GRID },
    { "nog$rid", S_NOGRID },
    { "hi$dden3d", S_HIDDEN3D },
    { "nohi$dden3d", S_NOHIDDEN3D },
    { "is$osamples", S_ISOSAMPLES },
    { "k$ey", S_KEY },
    { "nok$ey", S_NOKEY },
    { "keyt$itle", S_KEYTITLE },
    { "nokeyt$itle", S_NOKEYTITLE },
    { "la$bel", S_LABEL },
    { "nola$bel", S_NOLABEL },
    { "li$nestyle", S_LINESTYLE },
    { "noli$nestyle", S_NOLINESTYLE },
    { "ls", S_LINESTYLE },
    { "nols", S_NOLINESTYLE },
    { "loa$dpath", S_LOADPATH },
    { "noloa$dpath", S_NOLOADPATH },
    { "loc$ale", S_LOCALE },
    { "log$scale", S_LOGSCALE },
    { "nolog$scale", S_NOLOGSCALE },
    { "map$ping", S_MAPPING },
    { "map$ping3d", S_MAPPING },

    { "mar$gin", S_MARGIN },
    { "lmar$gin", S_LMARGIN },
    { "rmar$gin", S_RMARGIN },
    { "tmar$gin", S_TMARGIN },
    { "bmar$gin", S_BMARGIN },

    { "mis$sing", S_MISSING },
    { "nomis$sing", S_NOMISSING },

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
    { "noof$fsets", S_NOOFFSETS },
    { "or$igin", S_ORIGIN },
    { "o$utput", S_OUTPUT },
    { "pa$rametric", S_PARAMETRIC },
    { "nopa$rametric", S_NOPARAMETRIC },
    { "p$lot", S_PLOT },
    { "poi$ntsize", S_POINTSIZE },
    { "pol$ar", S_POLAR },
    { "nopol$ar", S_NOPOLAR },
    { "sa$mples", S_SAMPLES },
    { "si$ze", S_SIZE },
    { "su$rface", S_SURFACE },
    { "nosu$rface", S_NOSURFACE },
    { "t$erminal", S_TERMINAL },
    { "ti$cs", S_TICS },
    { "ticsc$ale", S_TICSCALE },
    { "ticsl$evel", S_TICSLEVEL },
    { "timef$mt", S_TIMEFMT },
    { "tim$estamp", S_TIMESTAMP },
    { "notim$estamp", S_NOTIMESTAMP },
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
    { "noxzeroa$xis", S_NOXZEROAXIS },
    { "x2zeroa$xis", S_XZEROAXIS },
    { "nox2zeroa$xis", S_NOXZEROAXIS },
    { "yzeroa$xis", S_YZEROAXIS },
    { "noyzeroa$xis", S_NOYZEROAXIS },
    { "y2zeroa$xis", S_YZEROAXIS },
    { "noy2zeroa$xis", S_NOYZEROAXIS },
    { "zeroa$xis", S_ZEROAXIS },
    { "nozeroa$xis", S_NOZEROAXIS },

    { "z$ero", S_ZERO },
    { NULL, S_INVALID }
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
