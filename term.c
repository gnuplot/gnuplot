/* GNUPLOT - term.c */
/*
 * Copyright (C) 1986, 1987, 1990   Thomas Williams, Colin Kelley
 *
 * Permission to use, copy, and distribute this software and its
 * documentation for any purpose with or without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and 
 * that both that copyright notice and this permission notice appear 
 * in supporting documentation.
 *
 * Permission to modify the software is granted, but not the right to
 * distribute the modified code.  Modifications are to be distributed 
 * as patches to released version.
 *  
 * This software  is provided "as is" without express or implied warranty.
 * 
 *
 * AUTHORS
 * 
 *   Original Software:
 *     Thomas Williams,  Colin Kelley.
 * 
 *   Gnuplot 2.0 additions:
 *       Russell Lang, Dave Kotz, John Campbell.
 * 
 * send your comments or suggestions to (pixar!info-gnuplot@sun.com).
 * 
 */

#include <stdio.h>
#include "plot.h"

/* for use by all drivers */
#define sign(x) ((x) >= 0 ? 1 : -1)
#define abs(x) ((x) >= 0 ? (x) : -(x))
#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

BOOLEAN term_init;			/* true if terminal has been initialized */

extern FILE *outfile;
extern char outstr[];
extern BOOLEAN term_init;
extern int term;
extern float xsize, ysize;

extern char input_line[];
extern struct lexical_unit token[];

extern BOOLEAN interactive;

/*
 * instead of <strings.h>
 */
extern char *strcpy();
extern int strlen(), strcmp();

char *getenv();

#ifdef __TURBOC__
char *turboc_init();
#endif

#ifdef vms
/* these are needed to sense a regis terminal and
 * to do a SET TERM/NOWRAP */
#include <descrip.h>
#include <dvidef.h>
#include <iodef.h>
#include <ttdef.h>
#include <tt2def.h>
static unsigned short   chan;
static int  old_char_buf[3], new_char_buf[3];
$DESCRIPTOR(sysoutput_desc,"SYS$OUTPUT");
char *vms_init();
#endif


/* This is needed because the unixplot library only writes to stdout. */
#ifdef UNIXPLOT
FILE save_stdout;
#endif
int unixplot=0;

#define NICE_LINE		0
#define POINT_TYPES		6


do_point(x,y,number)
int x,y;
int number;
{
register int htic,vtic;
register struct termentry *t = &term_tbl[term];

     if (number < 0) {		/* do dot */
	    (*t->move)(x,y);
	    (*t->vector)(x,y);
	    return;
	}

	number %= POINT_TYPES;
	htic = (t->h_tic/2);	/* should be in term_tbl[] in later version */
	vtic = (t->v_tic/2);	

	switch(number) {
		case 0: /* do diamond */ 
				(*t->move)(x-htic,y);
				(*t->vector)(x,y-vtic);
				(*t->vector)(x+htic,y);
				(*t->vector)(x,y+vtic);
				(*t->vector)(x-htic,y);
				(*t->move)(x,y);
				(*t->vector)(x,y);
				break;
		case 1: /* do plus */ 
				(*t->move)(x-htic,y);
				(*t->vector)(x-htic,y);
				(*t->vector)(x+htic,y);
				(*t->move)(x,y-vtic);
				(*t->vector)(x,y-vtic);
				(*t->vector)(x,y+vtic);
				break;
		case 2: /* do box */ 
				(*t->move)(x-htic,y-vtic);
				(*t->vector)(x-htic,y-vtic);
				(*t->vector)(x+htic,y-vtic);
				(*t->vector)(x+htic,y+vtic);
				(*t->vector)(x-htic,y+vtic);
				(*t->vector)(x-htic,y-vtic);
				(*t->move)(x,y);
				(*t->vector)(x,y);
				break;
		case 3: /* do X */ 
				(*t->move)(x-htic,y-vtic);
				(*t->vector)(x-htic,y-vtic);
				(*t->vector)(x+htic,y+vtic);
				(*t->move)(x-htic,y+vtic);
				(*t->vector)(x-htic,y+vtic);
				(*t->vector)(x+htic,y-vtic);
				break;
		case 4: /* do triangle */ 
				(*t->move)(x,y+(4*vtic/3));
				(*t->vector)(x-(4*htic/3),y-(2*vtic/3));
				(*t->vector)(x+(4*htic/3),y-(2*vtic/3));
				(*t->vector)(x,y+(4*vtic/3));
				(*t->move)(x,y);
				(*t->vector)(x,y);
				break;
		case 5: /* do star */ 
				(*t->move)(x-htic,y);
				(*t->vector)(x-htic,y);
				(*t->vector)(x+htic,y);
				(*t->move)(x,y-vtic);
				(*t->vector)(x,y-vtic);
				(*t->vector)(x,y+vtic);
				(*t->move)(x-htic,y-vtic);
				(*t->vector)(x-htic,y-vtic);
				(*t->vector)(x+htic,y+vtic);
				(*t->move)(x-htic,y+vtic);
				(*t->vector)(x-htic,y+vtic);
				(*t->vector)(x+htic,y-vtic);
				break;
	}
}


/*
 * general point routine
 */
line_and_point(x,y,number)
int x,y,number;
{
	/* temporary(?) kludge to allow terminals with bad linetypes 
		to make nice marks */

	(*term_tbl[term].linetype)(NICE_LINE);
	do_point(x,y,number);
}

/* 
 * general arrow routine
 */
#define ROOT2 (1.41421)		/* sqrt of 2 */

do_arrow(sx, sy, ex, ey)
	int sx,sy;			/* start point */
	int ex, ey;			/* end point (point of arrowhead) */
{
    register struct termentry *t = &term_tbl[term];
    int len = (t->h_tic + t->v_tic)/2; /* arrowhead size = avg of tic sizes */
    extern double sqrt();

    /* draw the line for the arrow. That's easy. */
    (*t->move)(sx, sy);
    (*t->vector)(ex, ey);

    /* now draw the arrow head. */
    /* we put the arrowhead marks at 45 degrees to line */
    if (sx == ex) {
	   /* vertical line, special case */
	   int delta = ((float)len / ROOT2 + 0.5);
	   if (sy < ey)
		delta = -delta;	/* up arrow goes the other way */
	   (*t->move)(ex - delta, ey + delta);
	   (*t->vector)(ex,ey);
	   (*t->vector)(ex + delta, ey + delta);
    } else {
	   int dx = sx - ex;
	   int dy = sy - ey;
	   double coeff = len / sqrt(2.0*((double)dx*(double)dx 
				+ (double)dy*(double)dy));
	   int x,y;			/* one endpoint */

	   x = (int)( ex + (dx + dy) * coeff );
	   y = (int)( ey + (dy - dx) * coeff );
	   (*t->move)(x,y);
	   (*t->vector)(ex,ey);

	   x = (int)( ex + (dx - dy) * coeff );
	   y = (int)( ey + (dy + dx) * coeff );
	   (*t->vector)(x,y);
    }

}

#ifdef PC
#ifndef __TURBOC__
#define FONT57
#endif
#endif

#ifdef NEC
#ifndef EPSON
#define EPSON
#endif
#endif

#ifdef PROPRINTER 
#ifndef EPSON
#define EPSON 
#endif
#endif

#ifdef EPSON
#define FONT57
#endif

#ifdef UNIXPC
#define FONT57
#endif

#ifdef FONT57
#include "term/font5x7.trm"
#endif  /* FONT57  */


#ifdef PC			/* all PC types */
#include "term/pc.trm"
#endif

/*
   all TEK types (TEK,BITGRAPH,KERMIT,SELANAR) are ifdef'd in tek.trm,
   but most require various TEK routines.  Hence TEK must be defined for
   the others to compile.
*/
#ifdef BITGRAPH
# ifndef TEK
#  define TEK
# endif
#endif

#ifdef SELENAR
# ifndef TEK
#  define TEK
# endif
#endif

#ifdef KERMIT
# ifndef TEK
#  define TEK
# endif
#endif

#ifdef TEK			/* all TEK types, TEK, BBN, SELANAR, KERMIT */
#include "term/tek.trm"
#endif

#ifdef EPSON		/* all bit map types, EPSON, NEC, PROPRINTER */
#include "term/epson.trm"
#endif

#ifdef FIG				/* Fig 1.4FS Interactive graphics program */
#include "term/fig.trm"
#endif

#ifdef IMAGEN		/* IMAGEN printer */
#include "term/imagen.trm"
#endif

#ifdef EEPIC		/* EEPIC (LATEX) type */
#include "term/eepic.trm"
# ifndef LATEX
#  define LATEX
# endif
#endif

#ifdef LATEX		/* LATEX type */
#include "term/latex.trm"
#endif

#ifdef POSTSCRIPT	/* POSTSCRIPT type */
#include "term/post.trm"
#endif

#ifdef HPLJET		/* hplaserjet */
#include "term/hpljet.trm"
#endif

#ifdef UNIXPC     /* unix-PC  ATT 7300 or 3b1 machine */
#include "term/unixpc.trm"
#endif /* UNIXPC */

#ifdef AED
#include "term/aed.trm"
#endif /* AED */

#ifdef HP2648
/* also works for HP2647 */
#include "term/hp2648.trm"
#endif /* HP2648 */

#ifdef HP26
#include "term/hp26.trm"
#endif /* HP26 */

#ifdef HP75
#ifndef HPGL
#define HPGL
#endif
#endif

/* HPGL - includes HP75 */
#ifdef HPGL
#include "term/hpgl.trm"
#endif /* HPGL */

/* Roland DXY800A plotter driver by Martin Yii, eln557h@monu3.OZ 
	and Russell Lang, rjl@monu1.cc.monash.oz */
#ifdef DXY800A
#include "term/dxy.trm"
#endif /* DXY800A */

#ifdef IRIS4D
#include "term/iris4d.trm"
#endif /* IRIS4D */

#ifdef QMS
#include "term/qms.trm"
#endif /* QMS */

#ifdef REGIS
#include "term/regis.trm"
#endif /* REGIS */

#ifdef SUN
#include "term/sun.trm"
#endif /* SUN */

#ifdef V384
#include "term/v384.trm"
#endif /* V384 */

#ifdef UNIXPLOT
#include "term/unixplot.trm"
#endif /* UNIXPLOT */

/* Dummy functions for unavailable features */

/* change angle of text.  0 is horizontal left to right.
* 1 is vertical bottom to top (90 deg rotate)  
*/
int null_text_angle()
{
return FALSE ;	/* can't be done */
}

/* change justification of text.  
 * modes are LEFT (flush left), CENTRE (centred), RIGHT (flush right)
 */
int null_justify_text()
{
return FALSE ;	/* can't be done */
}


/* Change scale of plot.
 * Parameters are x,y scaling factors for this plot.
 * Some terminals (eg latex) need to do scaling themselves.
 */
int null_scale()
{
return FALSE ;	/* can't be done */
}


UNKNOWN_null()
{
}


/*
 * term_tbl[] contains an entry for each terminal.  "unknown" must be the
 *   first, since term is initialized to 0.
 */
struct termentry term_tbl[] = {
    {"unknown", "Unknown terminal type - not a plotting device",
	  100, 100, 1, 1,
	  1, 1, UNKNOWN_null, UNKNOWN_null, 
	  UNKNOWN_null, null_scale, UNKNOWN_null, UNKNOWN_null, UNKNOWN_null, 
	  UNKNOWN_null, UNKNOWN_null, null_text_angle, 
	  null_justify_text, UNKNOWN_null, UNKNOWN_null}
#ifdef PC
#ifdef __TURBOC__

    ,{"egalib", "IBM PC/Clone with EGA graphics board",
	   EGALIB_XMAX, EGALIB_YMAX, EGALIB_VCHAR, EGALIB_HCHAR,
	   EGALIB_VTIC, EGALIB_HTIC, EGALIB_init, EGALIB_reset,
	   EGALIB_text, null_scale, EGALIB_graphics, EGALIB_move, EGALIB_vector,
	   EGALIB_linetype, EGALIB_put_text, EGALIB_text_angle, 
	   EGALIB_justify_text, do_point, do_arrow}

    ,{"vgalib", "IBM PC/Clone with VGA graphics board",
	   VGA_XMAX, VGA_YMAX, VGA_VCHAR, VGA_HCHAR,
	   VGA_VTIC, VGA_HTIC, VGA_init, VGA_reset,
	   VGA_text, null_scale, VGA_graphics, VGA_move, VGA_vector,
	   VGA_linetype, VGA_put_text, VGA_text_angle, 
	   VGA_justify_text, do_point, do_arrow}

    ,{"vgamono", "IBM PC/Clone with VGA Monochrome graphics board",
	   VGA_XMAX, VGA_YMAX, VGA_VCHAR, VGA_HCHAR,
	   VGA_VTIC, VGA_HTIC, VGA_init, VGA_reset,
	   VGA_text, null_scale, VGA_graphics, VGA_move, VGA_vector,
	   VGAMONO_linetype, VGA_put_text, VGA_text_angle, 
	   VGA_justify_text, line_and_point, do_arrow}

    ,{"mcga", "IBM PC/Clone with MCGA graphics board",
	   MCGA_XMAX, MCGA_YMAX, MCGA_VCHAR, MCGA_HCHAR,
	   MCGA_VTIC, MCGA_HTIC, MCGA_init, MCGA_reset,
	   MCGA_text, null_scale, MCGA_graphics, MCGA_move, MCGA_vector,
	   MCGA_linetype, MCGA_put_text, MCGA_text_angle, 
	   MCGA_justify_text, line_and_point, do_arrow}

    ,{"cga", "IBM PC/Clone with CGA graphics board",
	   CGA_XMAX, CGA_YMAX, CGA_VCHAR, CGA_HCHAR,
	   CGA_VTIC, CGA_HTIC, CGA_init, CGA_reset,
	   CGA_text, null_scale, CGA_graphics, CGA_move, CGA_vector,
	   CGA_linetype, CGA_put_text, MCGA_text_angle, 
	   CGA_justify_text, line_and_point, do_arrow}

    ,{"hercules", "IBM PC/Clone with Hercules graphics board",
	   HERC_XMAX, HERC_YMAX, HERC_VCHAR, HERC_HCHAR,
	   HERC_VTIC, HERC_HTIC, HERC_init, HERC_reset,
	   HERC_text, null_scale, HERC_graphics, HERC_move, HERC_vector,
	   HERC_linetype, HERC_put_text, MCGA_text_angle, 
	   HERC_justify_text, line_and_point, do_arrow}
#else					/* TURBO */

    ,{"cga", "IBM PC/Clone with CGA graphics board",
	   CGA_XMAX, CGA_YMAX, CGA_VCHAR, CGA_HCHAR,
	   CGA_VTIC, CGA_HTIC, CGA_init, CGA_reset,
	   CGA_text, null_scale, CGA_graphics, CGA_move, CGA_vector,
	   CGA_linetype, CGA_put_text, CGA_text_angle, 
	   null_justify_text, line_and_point, do_arrow}

    ,{"egabios", "IBM PC/Clone with EGA graphics board (BIOS)",
	   EGA_XMAX, EGA_YMAX, EGA_VCHAR, EGA_HCHAR,
	   EGA_VTIC, EGA_HTIC, EGA_init, EGA_reset,
	   EGA_text, null_scale, EGA_graphics, EGA_move, EGA_vector,
	   EGA_linetype, EGA_put_text, EGA_text_angle, 
	   null_justify_text, do_point, do_arrow}

    ,{"vgabios", "IBM PC/Clone with VGA graphics board (BIOS)",
	   VGA_XMAX, VGA_YMAX, VGA_VCHAR, VGA_HCHAR,
	   VGA_VTIC, VGA_HTIC, VGA_init, VGA_reset,
	   VGA_text, null_scale, VGA_graphics, VGA_move, VGA_vector,
	   VGA_linetype, VGA_put_text, VGA_text_angle, 
	   null_justify_text, do_point, do_arrow}

#ifdef EGALIB
    ,{"egalib", "IBM PC/Clone with EGA graphics board (LIB)",
	   EGALIB_XMAX, EGALIB_YMAX, EGALIB_VCHAR, EGALIB_HCHAR,
	   EGALIB_VTIC, EGALIB_HTIC, EGALIB_init, EGALIB_reset,
	   EGALIB_text, null_scale, EGALIB_graphics, EGALIB_move, EGALIB_vector,
	   EGALIB_linetype, EGALIB_put_text, null_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef HERCULES
    ,{"hercules", "IBM PC/Clone with Hercules graphics board",
	   HERC_XMAX, HERC_YMAX, HERC_VCHAR, HERC_HCHAR,
	   HERC_VTIC, HERC_HTIC, HERC_init, HERC_reset,
	   HERC_text, null_scale, HERC_graphics, HERC_move, HERC_vector,
	   HERC_linetype, HERC_put_text, HERC_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif					/* HERCULES */

#ifdef ATT6300
    ,{"att", "AT&T 6300 terminal ???",
	   ATT_XMAX, ATT_YMAX, ATT_VCHAR, ATT_HCHAR,
	   ATT_VTIC, ATT_HTIC, ATT_init, ATT_reset,
	   ATT_text, null_scale, ATT_graphics, ATT_move, ATT_vector,
	   ATT_linetype, ATT_put_text, ATT_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef CORONA
    ,{"corona325", "Corona graphics ???",
	   COR_XMAX, COR_YMAX, COR_VCHAR, COR_HCHAR,
	   COR_VTIC, COR_HTIC, COR_init, COR_reset,
	   COR_text, null_scale, COR_graphics, COR_move, COR_vector,
	   COR_linetype, COR_put_text, COR_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif					/* CORONA */
#endif					/* TURBO */
#endif					/* PC */

#ifdef AED
    ,{"aed512", "AED 512 Terminal",
	   AED5_XMAX, AED_YMAX, AED_VCHAR, AED_HCHAR,
	   AED_VTIC, AED_HTIC, AED_init, AED_reset, 
	   AED_text, null_scale, AED_graphics, AED_move, AED_vector, 
	   AED_linetype, AED_put_text, null_text_angle, 
	   null_justify_text, do_point, do_arrow}
    ,{"aed767", "AED 767 Terminal",
	   AED_XMAX, AED_YMAX, AED_VCHAR, AED_HCHAR,
	   AED_VTIC, AED_HTIC, AED_init, AED_reset, 
	   AED_text, null_scale, AED_graphics, AED_move, AED_vector, 
	   AED_linetype, AED_put_text, null_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef BITGRAPH
    ,{"bitgraph", "BBN Bitgraph Terminal",
	   BG_XMAX,BG_YMAX,BG_VCHAR, BG_HCHAR, 
	   BG_VTIC, BG_HTIC, BG_init, BG_reset, 
	   BG_text, null_scale, BG_graphics, BG_move, BG_vector,
	   BG_linetype, BG_put_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef DXY800A
    ,{"dxy800a", "Roland DXY800A plotter",
	   DXY_XMAX, DXY_YMAX, DXY_VCHAR, DXY_HCHAR,
	   DXY_VTIC, DXY_HTIC, DXY_init, DXY_reset,
	   DXY_text, null_scale, DXY_graphics, DXY_move, DXY_vector,
	   DXY_linetype, DXY_put_text, DXY_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef EPSON
    ,{"epson_lx800", "Epson LX-800, Star NL-10, NX-1000 and lots of others",
	   EPSONXMAX, EPSONYMAX, EPSONVCHAR, EPSONHCHAR, 
	   EPSONVTIC, EPSONHTIC, EPSONinit, EPSONreset, 
	   EPSONtext, null_scale, EPSONgraphics, EPSONmove, EPSONvector, 
	   EPSONlinetype, EPSONput_text, EPSON_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef FIG
    ,{"fig", "FIG graphics language: SunView or X graphics editor",
	   FIG_XMAX, FIG_YMAX, FIG_VCHAR, FIG_HCHAR, 
	   FIG_VTIC, FIG_HTIC, FIG_init, FIG_reset, 
	   FIG_text, null_scale, FIG_graphics, FIG_move, FIG_vector, 
	   FIG_linetype, FIG_put_text, FIG_text_angle, 
	   FIG_justify_text, do_point, FIG_arrow}
#endif

#ifdef HP26
    ,{"hp2623A", "HP2623A and maybe others",
	   HP26_XMAX, HP26_YMAX, HP26_VCHAR, HP26_HCHAR,
	   HP26_VTIC, HP26_HTIC, HP26_init, HP26_reset,
	   HP26_text, null_scale, HP26_graphics, HP26_move, HP26_vector,
	   HP26_linetype, HP26_put_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef HP2648
    ,{"hp2648", "HP2648 and HP2647",
	   HP2648XMAX, HP2648YMAX, HP2648VCHAR, HP2648HCHAR, 
	   HP2648VTIC, HP2648HTIC, HP2648init, HP2648reset, 
	   HP2648text, null_scale, HP2648graphics, HP2648move, HP2648vector, 
	   HP2648linetype, HP2648put_text, HP2648_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef HP75
    ,{"hp7580B", "HP7580, and probably other HPs (4 pens)",
	   HPGL_XMAX, HPGL_YMAX, HPGL_VCHAR, HPGL_HCHAR,
	   HPGL_VTIC, HPGL_HTIC, HPGL_init, HPGL_reset,
	   HPGL_text, null_scale, HPGL_graphics, HPGL_move, HPGL_vector,
	   HP75_linetype, HPGL_put_text, HPGL_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef HPGL
    ,{"hpgl", "HP7475 and (hopefully) lots of others (6 pens)",
	   HPGL_XMAX, HPGL_YMAX, HPGL_VCHAR, HPGL_HCHAR,
	   HPGL_VTIC, HPGL_HTIC, HPGL_init, HPGL_reset,
	   HPGL_text, null_scale, HPGL_graphics, HPGL_move, HPGL_vector,
	   HPGL_linetype, HPGL_put_text, HPGL_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef HPLJET
    ,{"laserjet1", "HP Laserjet, smallest size",
	   HPLJETXMAX, HPLJETYMAX, HPLJET1VCHAR, HPLJET1HCHAR, 
	   HPLJETVTIC, HPLJETHTIC, HPLJET1init, HPLJETreset, 
	   HPLJETtext, null_scale, HPLJETgraphics, HPLJETmove, HPLJETvector,
	   HPLJETlinetype, HPLJETput_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
    ,{"laserjet2", "HP Laserjet, medium size",
	   HPLJETXMAX, HPLJETYMAX, HPLJET2VCHAR, HPLJET2HCHAR, 
	   HPLJETVTIC, HPLJETHTIC, HPLJET2init, HPLJETreset, 
	   HPLJETtext, null_scale, HPLJETgraphics, HPLJETmove, HPLJETvector,
	   HPLJETlinetype, HPLJETput_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
    ,{"laserjet3", "HP Laserjet, largest size",
	   HPLJETXMAX, HPLJETYMAX, HPLJET3VCHAR, HPLJET3HCHAR, 
	   HPLJETVTIC, HPLJETHTIC, HPLJET3init, HPLJETreset, 
	   HPLJETtext, null_scale, HPLJETgraphics, HPLJETmove, HPLJETvector, 
	   HPLJETlinetype, HPLJETput_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef IMAGEN
    ,{"imagen", "Imagen laser printer",
	   IMAGEN_XMAX, IMAGEN_YMAX, IMAGEN_VCHAR, IMAGEN_HCHAR, 
	   IMAGEN_VTIC, IMAGEN_HTIC, IMAGEN_init, IMAGEN_reset, 
	   IMAGEN_text, null_scale, IMAGEN_graphics, IMAGEN_move, 
	   IMAGEN_vector, IMAGEN_linetype, IMAGEN_put_text, IMAGEN_text_angle,
	   IMAGEN_justify_text, line_and_point, do_arrow}
#endif

#ifdef IRIS4D
    ,{"iris4d", "Silicon Graphics IRIS 4D Series Computer",
	   IRIS4D_XMAX, IRIS4D_YMAX, IRIS4D_VCHAR, IRIS4D_HCHAR, 
	   IRIS4D_VTIC, IRIS4D_HTIC, IRIS4D_init, IRIS4D_reset, 
	   IRIS4D_text, null_scale, IRIS4D_graphics, IRIS4D_move, IRIS4D_vector,
	   IRIS4D_linetype, IRIS4D_put_text, null_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef KERMIT
    ,{"kc_tek40xx", "Kermit-MS tek40xx terminal emulator - color",
	   TEK40XMAX,TEK40YMAX,TEK40VCHAR, KTEK40HCHAR, 
	   TEK40VTIC, TEK40HTIC, TEK40init, KTEK40reset, 
	   KTEK40Ctext, null_scale, KTEK40graphics, TEK40move, TEK40vector, 
	   KTEK40Clinetype, TEK40put_text, null_text_angle, 
	   null_justify_text, do_point, do_arrow}
    ,{"km_tek40xx", "Kermit-MS tek40xx terminal emulator - monochrome",
	   TEK40XMAX,TEK40YMAX,TEK40VCHAR, KTEK40HCHAR, 
	   TEK40VTIC, TEK40HTIC, TEK40init, KTEK40reset, 
	   TEK40text, null_scale, KTEK40graphics, TEK40move, TEK40vector, 
	   KTEK40Mlinetype, TEK40put_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef LATEX
    ,{"latex", "LaTeX picture environment",
	   LATEX_XMAX, LATEX_YMAX, LATEX_VCHAR, LATEX_HCHAR, 
	   LATEX_VTIC, LATEX_HTIC, LATEX_init, LATEX_reset, 
	   LATEX_text, LATEX_scale, LATEX_graphics, LATEX_move, LATEX_vector, 
	   LATEX_linetype, LATEX_put_text, LATEX_text_angle, 
	   LATEX_justify_text, LATEX_point, LATEX_arrow}
#endif

#ifdef EEPIC
    ,{"eepic", "EEPIC -- extended LaTeX picture environment",
	   EEPIC_XMAX, EEPIC_YMAX, EEPIC_VCHAR, EEPIC_HCHAR, 
	   EEPIC_VTIC, EEPIC_HTIC, EEPIC_init, EEPIC_reset, 
	   EEPIC_text, EEPIC_scale, EEPIC_graphics, EEPIC_move, EEPIC_vector, 
	   EEPIC_linetype, EEPIC_put_text, EEPIC_text_angle, 
	   EEPIC_justify_text, EEPIC_point, EEPIC_arrow}
#endif

#ifdef NEC
    ,{"nec_cp6m", "NEC printer CP6 Monochrome",
	   NECXMAX, NECYMAX, NECVCHAR, NECHCHAR, 
	   NECVTIC, NECHTIC, NECMinit, NECreset, 
	   NECtext, null_scale, NECgraphics, NECmove, NECvector, 
	   NECMlinetype, NECput_text, NEC_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
    ,{"nec_cp6c", "NEC printer CP6 Color",
	   NECXMAX, NECYMAX, NECVCHAR, NECHCHAR, 
	   NECVTIC, NECHTIC, NECCinit, NECreset, 
	   NECtext, null_scale, NECgraphics, NECmove, NECvector, 
	   NECClinetype, NECput_text, NEC_text_angle, 
	   null_justify_text, do_point, do_arrow}
    ,{"nec_cp6d", "NEC printer CP6 Draft monochrome",
	   NECXMAX, NECYMAX, NECVCHAR, NECHCHAR, 
	   NECVTIC, NECHTIC, NECMinit, NECreset, 
	   NECdraft_text, null_scale, NECgraphics, NECmove, NECvector, 
	   NECMlinetype, NECput_text, NEC_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef POSTSCRIPT
    ,{"postscript", "Postscript graphics language, small characters",
	   PS_XMAX, PS_YMAX, PS_VCHAR1, PS_HCHAR1, 
	   PS_VTIC, PS_HTIC, PS1_init, PS_reset, 
	   PS_text, null_scale, PS_graphics, PS_move, PS_vector, 
	   PS_linetype, PS_put_text, PS_text_angle, 
	   PS_justify_text, PS_point, do_arrow}
    ,{"psbig", "Postscript graphics language, big characters",
	   PS_XMAX, PS_YMAX, PS_VCHAR2, PS_HCHAR2, 
	   PS_VTIC, PS_HTIC, PS2_init, PS_reset, 
	   PS_text, null_scale, PS_graphics, PS_move, PS_vector, 
	   PS_linetype, PS_put_text, PS_text_angle, 
	   PS_justify_text, PS_point, do_arrow}
    ,{"epsf1", "Encapsulated Postscript graphics language, small characters",
	   PS_XMAX, PS_YMAX, PS_VCHAR1, PS_HCHAR1, 
	   PS_VTIC, PS_HTIC, EPSF1_init, EPSF_reset, 
	   EPSF_text, null_scale, EPSF_graphics, PS_move, PS_vector, 
	   PS_linetype, PS_put_text, PS_text_angle, 
	   PS_justify_text, PS_point, do_arrow}
    ,{"epsf2", "Encapsulated Postscript graphics language, big characters",
	   PS_XMAX, PS_YMAX, PS_VCHAR2, PS_HCHAR2, 
	   PS_VTIC, PS_HTIC, EPSF2_init, EPSF_reset, 
	   EPSF_text, null_scale, EPSF_graphics, PS_move, PS_vector, 
	   PS_linetype, PS_put_text, PS_text_angle, 
	   PS_justify_text, PS_point, do_arrow}
#endif

#ifdef PROPRINTER
    ,{"proprinter", "IBM Proprinter",
	   EPSONXMAX, EPSONYMAX, EPSONVCHAR, EPSONHCHAR, 
	   EPSONVTIC, EPSONHTIC, EPSONinit, EPSONreset, 
	   PROPRINTERtext, null_scale, EPSONgraphics, EPSONmove, EPSONvector, 
	   EPSONlinetype, EPSONput_text, EPSON_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef QMS
    ,{"qms", "QMS/QUIC Laser printer (also Talaris 1200 and others)",
	   QMS_XMAX,QMS_YMAX, QMS_VCHAR, QMS_HCHAR, 
	   QMS_VTIC, QMS_HTIC, QMS_init,QMS_reset, 
	   QMS_text, null_scale, QMS_graphics, QMS_move, QMS_vector,
	   QMS_linetype,QMS_put_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef REGIS
    ,{"regis", "REGIS graphics language",
	   REGISXMAX, REGISYMAX, REGISVCHAR, REGISHCHAR, 
	   REGISVTIC, REGISHTIC, REGISinit, REGISreset, 
	   REGIStext, null_scale, REGISgraphics, REGISmove, REGISvector,
	   REGISlinetype, REGISput_text, REGIStext_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif


#ifdef SELANAR
    ,{"selanar", "Selanar",
	   TEK40XMAX, TEK40YMAX, TEK40VCHAR, TEK40HCHAR, 
	   TEK40VTIC, TEK40HTIC, SEL_init, SEL_reset, 
	   SEL_text, null_scale, SEL_graphics, TEK40move, TEK40vector, 
	   TEK40linetype, TEK40put_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef SUN
    ,{"sun", "SunView window system",
	   SUN_XMAX, SUN_YMAX, SUN_VCHAR, SUN_HCHAR, 
	   SUN_VTIC, SUN_HTIC, SUN_init, SUN_reset, 
	   SUN_text, null_scale, SUN_graphics, SUN_move, SUN_vector,
	   SUN_linetype, SUN_put_text, null_text_angle, 
	   SUN_justify_text, line_and_point, do_arrow}
#endif

#ifdef TEK
    ,{"tek40xx", "Tektronix 4010 and others; most TEK emulators",
	   TEK40XMAX, TEK40YMAX, TEK40VCHAR, TEK40HCHAR, 
	   TEK40VTIC, TEK40HTIC, TEK40init, TEK40reset, 
	   TEK40text, null_scale, TEK40graphics, TEK40move, TEK40vector, 
	   TEK40linetype, TEK40put_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef UNIXPLOT
    ,{"unixplot", "Unix plotting standard (see plot(1))",
	   UP_XMAX, UP_YMAX, UP_VCHAR, UP_HCHAR, 
	   UP_VTIC, UP_HTIC, UP_init, UP_reset, 
	   UP_text, null_scale, UP_graphics, UP_move, UP_vector, 
	   UP_linetype, UP_put_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif
	
#ifdef UNIXPC
    ,{"unixpc", "AT&T 3b1 or AT&T 7300 Unix PC",
	   uPC_XMAX, uPC_YMAX, uPC_VCHAR, uPC_HCHAR, 
	   uPC_VTIC, uPC_HTIC, uPC_init, uPC_reset, 
	   uPC_text, null_scale, uPC_graphics, uPC_move, uPC_vector,
	   uPC_linetype, uPC_put_text, uPC_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef V384
    ,{"vx384", "Vectrix 384 and Tandy color printer",
	   V384_XMAX, V384_YMAX, V384_VCHAR, V384_HCHAR, 
	   V384_VTIC, V384_HTIC, V384_init, V384_reset, 
	   V384_text, null_scale, V384_graphics, V384_move, V384_vector, 
	   V384_linetype, V384_put_text, null_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif
};

#define TERMCOUNT (sizeof(term_tbl)/sizeof(struct termentry))


list_terms()
{
register int i;

	fprintf(stderr,"\nAvailable terminal types:\n");
	for (i = 0; i < TERMCOUNT; i++)
		fprintf(stderr,"  %15s  %s\n",
			   term_tbl[i].name, term_tbl[i].description);
	(void) putc('\n',stderr);
}


/* set_term: get terminal number from name on command line */
/* will change 'term' variable if successful */
int						/* term number */
set_term(c_token)
int c_token;
{
    register int t;
    char *input_name;

    if (!token[c_token].is_token)
	 int_error("terminal name expected",c_token);
    t = -1;
    input_name = input_line + token[c_token].start_index;
    t = change_term(input_name, token[c_token].length);
    if (t == -1)
	 int_error("unknown terminal type; type just 'set terminal' for a list",
			 c_token);
    if (t == -2)
	 int_error("ambiguous terminal name; type just 'set terminal' for a list",
			 c_token);

    /* otherwise the type was changed */

    return(t);
}

/* change_term: get terminal number from name and set terminal type */
/* returns -1 if unknown, -2 if ambiguous, >=0 is terminal number */
int
change_term(name, length)
	char *name;
	int length;
{
    int i, t = -1;

    for (i = 0; i < TERMCOUNT; i++) {
	   if (!strncmp(name,term_tbl[i].name,length)) {
		  if (t != -1)
		    return(-2);	/* ambiguous */
		  t = i;
	   }
    }

    if (t == -1)			/* unknown */
	 return(t);

    /* Success: set terminal type now */

    /* Special handling for unixplot term type */
    if (!strncmp("unixplot",name,sizeof(unixplot))) {
	   UP_redirect (2);  /* Redirect actual stdout for unixplots */
    } else if (unixplot) {
	   UP_redirect (3);  /* Put stdout back together again. */
    }

    term = t;
    term_init = FALSE;
    name = term_tbl[term].name;

    if (interactive)
	 fprintf(stderr, "Terminal type set to '%s'\n", name);

    return(t);
}

/*
   Routine to detect what terminal is being used (or do anything else
   that would be nice).  One anticipated (or allowed for) side effect
   is that the global ``term'' may be set. 
   The environment variable GNUTERM is checked first; if that does
   not exist, then the terminal hardware is checked, if possible, 
   and finally, we can check $TERM for some kinds of terminals.
*/
/* thanks to osupyr!alden (Dave Alden) for the original GNUTERM code */
init_terminal()
{
    char *term_name = NULL;
    int t;
#ifdef SUN  /* turbo C doesn't like unused variables */
    char *term = NULL;		/* from TERM environment var */
#endif
    char *gnuterm = NULL;

    /* GNUTERM environment variable is primary */
    gnuterm = getenv("GNUTERM");
    if (gnuterm != (char *)NULL)
	 term_name = gnuterm;
    else {
#ifdef __TURBOC__
	   term_name = turboc_init();
#endif
	   
#ifdef vms
	   term_name = vms_init();
#endif
	   
#ifdef SUN
	   term = getenv("TERM");	/* try $TERM */
	   if (term_name == (char *)NULL
		  && term != (char *)NULL && strcmp(term, "sun") == 0)
		term_name = "sun";
#endif /* sun */

#ifdef UNIXPC
           if (iswind() == 0) {
              term_name = "unixpc";
           }
#endif /* unixpc */
    }

    /* We have a name, try to set term type */
    if (term_name != NULL && *term_name != '\0') {
	   t = change_term(term_name, strlen(term_name));
	   if (t == -1)
		fprintf(stderr, "Unknown terminal name '%s'\n", term_name);
	   else if (t == -2)
		fprintf(stderr, "Ambiguous terminal name '%s'\n", term_name);
	   else				/* successful */
		;
    }
}


#ifdef __TURBOC__
char *
turboc_init()
{
  int g_driver,g_mode;
  char far *c1,*c2;
  char *term_name = NULL;

/* Some of this code including BGI drivers is copyright Borland Intl. */
	g_driver=DETECT;
	      get_path();
        initgraph(&g_driver,&g_mode,path);
        c1=getdrivername();
        c2=getmodename(g_mode);
          switch (g_driver){
            case -2: fprintf(stderr,"Graphics card not detected.\n");
                     break;
            case -3: fprintf(stderr,"BGI driver file cannot be found.\n");
                     break;
            case -4: fprintf(stderr,"Invalid BGI driver file.\n");
                     break;
            case -5: fprintf(stderr,"Insufficient memory to load ",
                             "graphics driver.");
                     break;
			case 1 : term_name = "cga";
					 break;
			case 2 : term_name = "mcga";
					 break;
			case 3 : 
			case 4 : term_name = "egalib";
					 break;
			case 7 : term_name = "hercules";
					 break;
			case 9 : term_name = "vgalib";
					 break;
            }
        closegraph();
	fprintf(stderr,"\tTC Graphics, driver %s  mode %s\n",c1,c2);
  return(term_name);
}
#endif /* __TURBOC__ */

#ifdef vms
/*
 * Determine if we have a regis terminal.  
 * and do a SET TERM/NOWRAP
*/
char *
vms_init()
{
char *term_str="tt:";
typedef struct
          {
          short cond_value;
          short count;
          int info;
          }  status_block;
typedef struct
          {
          short buffer_len;
          short item_code;
          int buffer_addr;
          int ret_len_addr;
          }  item_desc;
struct {
    item_desc dev_type;
      int terminator;
   }  dvi_list;
   int status, dev_type, zero=0;
   char buffer[255];
   $DESCRIPTOR(term_desc, term_str);

/* This does the equivalent of a SET TERM/NOWRAP command */
    int i;
    sys$assign(&sysoutput_desc,&chan,0,0);
    sys$qiow(0,chan,IO$_SENSEMODE,0,0,0,old_char_buf,12,0,0,0,0);
    for (i = 0 ; i < 3 ; ++i) new_char_buf[i] = old_char_buf[i];
    new_char_buf[1] &= ~(TT$M_WRAP);
    sys$qiow(0,chan,IO$_SETMODE,0,0,0,new_char_buf,12,0,0,0,0);
    sys$dassgn(chan);

/* set up dvi item list */
	dvi_list.dev_type.buffer_len = 4;
	dvi_list.dev_type.item_code = DVI$_TT_REGIS;
	dvi_list.dev_type.buffer_addr = &dev_type;
	dvi_list.dev_type.ret_len_addr = 0;

	dvi_list.terminator = 0;

/* See what type of terminal we have. */
	status = SYS$GETDVIW (0, 0, &term_desc, &dvi_list, 0, 0, 0, 0);
	if ((status & 1) && dev_type) {
		return("regis");
	}
     return(NULL);
}



/*
 *  vms_reset
 */
vms_reset()
{
    /* set wrap back to original state */
    sys$assign(&sysoutput_desc,&chan,0,0);
    sys$qiow(0,chan,IO$_SETMODE,0,0,0,old_char_buf,12,0,0,0,0);
    sys$dassgn(chan);
}
#endif /* VMS */

/*
	This is always defined so we don't have to have command.c know if it
	is there or not.
*/
#ifndef UNIXPLOT
UP_redirect(caller) int caller; 
{
	caller = caller;	/* to stop Turbo C complaining 
						 * about caller not being used */
}
#else
UP_redirect (caller)
int caller;
/*
	Unixplot can't really write to outfile--it wants to write to stdout.
	This is normally ok, but the original design of gnuplot gives us
	little choice.  Originally users of unixplot had to anticipate
	their needs and redirect all I/O to a file...  Not very gnuplot-like.

	caller:  1 - called from SET OUTPUT "FOO.OUT"
			 2 - called from SET TERM UNIXPLOT
			 3 - called from SET TERM other
			 4 - called from SET OUTPUT
*/
{
	switch (caller) {
	case 1:
	/* Don't save, just replace stdout w/outfile (save was already done). */
		if (unixplot)
			*(stdout) = *(outfile);  /* Copy FILE structure */
	break;
	case 2:
		if (!unixplot) {
			fflush(stdout);
			save_stdout = *(stdout);
			*(stdout) = *(outfile);  /* Copy FILE structure */
			unixplot = 1;
		}
	break;
	case 3:
	/* New terminal in use--put stdout back to original. */
		closepl();
		fflush(stdout);
		*(stdout) = save_stdout;  /* Copy FILE structure */
		unixplot = 0;
	break;
	case 4:
	/*  User really wants to go to normal output... */
		if (unixplot) {
			fflush(stdout);
			*(stdout) = save_stdout;  /* Copy FILE structure */
		}
	break;
	}
}
#endif


/* test terminal by drawing border and text */
/* called from command test */
test_term()
{
	register struct termentry *t = &term_tbl[term];
	char *str;
	int x,y, xl,yl, i;
	char label[MAX_ID_LEN];

	if (!term_init) {
	   (*t->init)();
	   term_init = TRUE;
	}
	screen_ok = FALSE;
	(*t->graphics)();
	/* border linetype */
	(*t->linetype)(-2);
	(*t->move)(0,0);
	(*t->vector)(t->xmax-1,0);
	(*t->vector)(t->xmax-1,t->ymax-1);
	(*t->vector)(0,t->ymax-1);
	(*t->vector)(0,0);
	(void) (*t->justify_text)(LEFT);
	(*t->put_text)(t->h_char*5,t->ymax-t->v_char*3,"Terminal Test");
	/* axis linetype */
	(*t->linetype)(-1);
	(*t->move)(t->xmax/2,0);
	(*t->vector)(t->xmax/2,t->ymax-1);
	(*t->move)(0,t->ymax/2);
	(*t->vector)(t->xmax-1,t->ymax/2);
	/* test width and height of characters */
	(*t->linetype)(-2);
	(*t->move)(  t->xmax/2-t->h_char*10,t->ymax/2+t->v_char/2);
	(*t->vector)(t->xmax/2+t->h_char*10,t->ymax/2+t->v_char/2);
	(*t->vector)(t->xmax/2+t->h_char*10,t->ymax/2-t->v_char/2);
	(*t->vector)(t->xmax/2-t->h_char*10,t->ymax/2-t->v_char/2);
	(*t->vector)(t->xmax/2-t->h_char*10,t->ymax/2+t->v_char/2);
	(*t->put_text)(t->xmax/2-t->h_char*10,t->ymax/2,
		"12345678901234567890");
	/* test justification */
	(void) (*t->justify_text)(LEFT);
	(*t->put_text)(t->xmax/2,t->ymax/2+t->v_char*6,"left justified");
	str = "centre+d text";
	if ((*t->justify_text)(CENTRE))
		(*t->put_text)(t->xmax/2,
				t->ymax/2+t->v_char*5,str);
	else
		(*t->put_text)(t->xmax/2-strlen(str)*t->h_char/2,
				t->ymax/2+t->v_char*5,str);
	str = "right justified";
	if ((*t->justify_text)(RIGHT))
		(*t->put_text)(t->xmax/2,
				t->ymax/2+t->v_char*4,str);
	else
		(*t->put_text)(t->xmax/2-strlen(str)*t->h_char,
				t->ymax/2+t->v_char*4,str);
	/* test text angle */
	str = "rotated ce+ntred text";
	if ((*t->text_angle)(1)) {
		if ((*t->justify_text)(CENTRE))
			(*t->put_text)(t->v_char,
				t->ymax/2,str);
		else
			(*t->put_text)(t->v_char,
				t->ymax/2-strlen(str)*t->h_char/2,str);
	}
	else {
	    (void) (*t->justify_text)(LEFT);
	    (*t->put_text)(t->h_char*2,t->ymax/2-t->v_char*2,"Can't rotate text");
	}
	(void) (*t->justify_text)(LEFT);
	(void) (*t->text_angle)(0);
	/* test tic size */
	(*t->move)(t->xmax/2+t->h_tic*2,0);
	(*t->vector)(t->xmax/2+t->h_tic*2,t->v_tic);
	(*t->move)(t->xmax/2,t->v_tic*2);
	(*t->vector)(t->xmax/2+t->h_tic,t->v_tic*2);
	(*t->put_text)(t->xmax/2+t->h_tic*2,t->v_tic*2+t->v_char/2,"test tics");
	/* test line and point types */
	x = t->xmax - t->h_char*4 - t->h_tic*4;
	y = t->ymax - t->v_char;
	for ( i = -2; y > t->v_char; i++ ) {
		(*t->linetype)(i);
		(void) sprintf(label,"%d",i);
		if ((*t->justify_text)(RIGHT))
			(*t->put_text)(x,y,label);
		else
			(*t->put_text)(x-strlen(label)*t->h_char,y,label);
		(*t->move)(x+t->h_char,y);
		(*t->vector)(x+t->h_char*4,y);
		if ( i >= -1 )
			(*t->point)(x+t->h_char*4+t->h_tic*2,y,i);
		y -= t->v_char;
	}
	/* test some arrows */
	(*t->linetype)(0);
	x = t->xmax/4;
	y = t->ymax/4;
	xl = t->h_tic*5;
	yl = t->v_tic*5;
	(*t->arrow)(x,y,x+xl,y);
	(*t->arrow)(x,y,x+xl,y+yl);
	(*t->arrow)(x,y,x,y+yl);
	(*t->arrow)(x,y,x-xl,y+yl);
	(*t->arrow)(x,y,x-xl,y);
	(*t->arrow)(x,y,x-xl,y-yl);
	(*t->arrow)(x,y,x,y-yl);
	(*t->arrow)(x,y,x+xl,y-yl);
	/* and back into text mode */
	(*t->text)();
}
