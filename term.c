#ifndef lint
static char *RCSid = "$Id: term.c%v 3.50.1.17 1993/08/27 05:21:33 woo Exp woo $";
#endif


/* GNUPLOT - term.c */
/*
 * Copyright (C) 1986 - 1993   Thomas Williams, Colin Kelley
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
 * This software is provided "as is" without express or implied warranty.
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
 *   Gnuplot 3.0 additions:
 *       Gershon Elber and many others.
 * 
 * There is a mailing list for gnuplot users. Note, however, that the
 * newsgroup 
 *	comp.graphics.gnuplot 
 * is identical to the mailing list (they
 * both carry the same set of messages). We prefer that you read the
 * messages through that newsgroup, to subscribing to the mailing list.
 * (If you can read that newsgroup, and are already on the mailing list,
 * please send a message info-gnuplot-request@dartmouth.edu, asking to be
 * removed from the mailing list.)
 *
 * The address for mailing to list members is
 *	   info-gnuplot@dartmouth.edu
 * and for mailing administrative requests is 
 *	   info-gnuplot-request@dartmouth.edu
 * The mailing list for bug reports is 
 *	   bug-gnuplot@dartmouth.edu
 * The list of those interested in beta-test versions is
 *	   info-gnuplot-beta@dartmouth.edu
 */

#include <stdio.h>
#include <ctype.h>
#include "plot.h"
#include "setshow.h"
#include "term.h"
#include "bitmap.h"
#ifdef NEXT
#include <stdlib.h>
#include "epsviewe.h"
#endif /* NEXT */

#ifdef _Windows
#ifdef __MSC__
#include <malloc.h>
#else
#include <alloc.h>
#endif
#endif

#if defined(__TURBOC__) && defined(MSDOS)
#ifndef _Windows
#include <dos.h>
#endif
#endif

/* for use by all drivers */
#define sign(x) ((x) >= 0 ? 1 : -1)
#define abs(x) ((x) >= 0 ? (x) : -(x))

#ifndef max	/* GCC uses inline functions */
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

TBOOLEAN term_init;			/* true if terminal has been initialized */

extern FILE *outfile;
extern char outstr[];
extern TBOOLEAN term_init;
extern int term;
extern float xsize, ysize;

extern char input_line[];
extern struct lexical_unit token[];
extern int num_tokens, c_token;
extern struct value *const_express();

extern TBOOLEAN interactive;

/*
 * instead of <strings.h>
 */

#if defined(_Windows) || ( defined(__TURBOC__) && defined(MSDOS) )
# include <string.h>
#else
#ifdef ATARI
#include <string.h>
#include <math.h>
#else
#ifndef AMIGA_SC_6_1
extern char *strcpy();
#ifdef ANSI
extern size_t   strlen();
#else
extern int      strlen();
#endif
extern int  strcmp(), strncmp();
#endif /* !AMIGA_SC_6_1 */
#endif
#endif

#ifndef AMIGA_AC_5
extern double sqrt();
#endif

char *getenv();

#if defined(__TURBOC__) && defined(MSDOS) && !defined(_Windows)
char *turboc_init();
#endif

#ifdef __ZTC__
char *ztc_init();
/* #undef TGIF */
#endif


#if defined(MSDOS)||defined(ATARI)||defined(OS2)||defined(_Windows)||defined(DOS386)
void reopen_binary();
#define REOPEN_BINARY
#endif
#ifdef vms
char *vms_init();
void vms_reset();
void term_mode_tek();
void term_mode_native();
void term_pasthru();
void term_nopasthru();
void reopen_binary();
void fflush_binary();
#define REOPEN_BINARY
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
	    return(0);
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

do_arrow(sx, sy, ex, ey, head)
	int sx,sy;			/* start point */
	int ex, ey;			/* end point (point of arrowhead) */
	TBOOLEAN head;
{
    register struct termentry *t = &term_tbl[term];
    int len = (t->h_tic + t->v_tic)/2; /* arrowhead size = avg of tic sizes */

    /* draw the line for the arrow. That's easy. */
    (*t->move)(sx, sy);
    (*t->vector)(ex, ey);

    if (head) {
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
}

#ifdef DUMB                    /* paper or glass dumb terminal */
#include "term/dumb.trm"
#endif


#ifndef _Windows
# ifdef PC			/* all PC types except MS WINDOWS*/
#  include "term/pc.trm"
# endif
#else
#  include "term/win.trm"
#endif

#ifdef __ZTC__
#include "term/fg.trm"
#endif

#ifdef DJSVGA
#include "term/djsvga.trm"	/* DJGPP SVGA */
#endif

#ifdef EMXVGA
#include "term/emxvga.trm"	/* EMX VGA */
#endif

#ifdef OS2PM                    /* os/2 presentation manager */
#include "term/pm.trm"
#endif

#ifdef ATARI			/* ATARI-ST */
#include "term/atari.trm"
#endif

/*
   all TEK types (TEK,BITGRAPH,KERMIT,VTTEK,SELANAR) are ifdef'd in tek.trm,
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

#ifdef LN03P
# ifndef TEK
#  define TEK
# endif
#endif

#ifdef VTTEK
# ifndef TEK
#  define TEK
# endif
#endif

#ifdef T410X		/* Tektronix 4106, 4107, 4109 and 420x terminals */
#include "term/t410x.trm"
#endif

#ifdef TEK			/* all TEK types, TEK, BBN, SELANAR, KERMIT, VTTEK */
#include "term/tek.trm"
#endif

#ifdef OKIDATA
#define EPSONP
#endif

#ifdef EPSONP	/* bit map types, EPSON, NEC, PROPRINTER, STAR Color */
#include "term/epson.trm"
#endif

#ifdef HP500C  /* HP 500 deskjet Colour */
#include "term/hp500c.trm"
#endif

#ifdef HPLJII		/* HP LaserJet II */
#include "term/hpljii.trm"
#endif

#ifdef PCL /* HP LaserJet III in HPGL mode */
#  ifndef HPGL
#    define HPGL
#  endif
#endif

#ifdef HPPJ		/* HP PaintJet */
#include "term/hppj.trm"
#endif

#ifdef FIG			/* Fig 2.1 Interactive graphics program */
#include "term/fig.trm"
#include "term/bigfig.trm"
#endif
  
#ifdef GPR              /* Apollo Graphics Primitive Resource (fixed-size window) */
#include "term/gpr.trm"
#endif /* GPR */

#ifdef GRASS              /* GRASS (geographic info system) monitor */
#include "term/grass.trm"
#endif /* GRASS */

#ifdef APOLLO           /* Apollo Graphics Primitive Resource (resizable window) */
#include "term/apollo.trm"
#endif /* APOLLO */

#ifdef IMAGEN		/* IMAGEN printer */
#include "term/imagen.trm"
#endif

#ifdef MIF			/* Framemaker MIF  driver */
#include "term/mif.trm"
#endif

#ifdef MF			/* METAFONT driver */
#include "term/metafont.trm"
#endif

#ifdef TEXDRAW
#include "term/texdraw.trm"
#endif

#ifdef EEPIC		/* EEPIC (LATEX) type */
#include "term/eepic.trm"
# ifndef LATEX
#  define LATEX
# endif
#endif

#ifdef EMTEX		/* EMTEX (LATEX for PC) type */
# ifndef LATEX
#  define LATEX
# endif
#endif

#ifdef LATEX		/* LATEX type */
#include "term/latex.trm"
#endif

#ifdef GPIC		/* GPIC (groff) type */ 
#include "term/gpic.trm"
#endif

#ifdef PBM		/* PBMPLUS portable bitmap */
#include "term/pbm.trm"
#endif

#ifdef POSTSCRIPT	/* POSTSCRIPT type */
#include "term/post.trm"
#endif

#ifdef PRESCRIBE	/* PRESCRIBE type */
#include "term/kyo.trm"
#endif

/* note: this must come after term/post.trm */
#ifdef PSLATEX		/* LaTeX with embedded PostScript */
#include "term/pslatex.trm"
#endif

#ifdef PSTRICKS
#include "term/pstricks.trm"
#endif

#ifdef TPIC		/* TPIC (LATEX) type */
#include "term/tpic.trm"
# ifndef LATEX
#  define LATEX
# endif
#endif

#ifdef UNIXPC     /* unix-PC  ATT 7300 or 3b1 machine */
#include "term/unixpc.trm"
#endif /* UNIXPC */

#ifdef AED
#include "term/aed.trm"
#endif /* AED */

#ifdef AIFM
#include "term/ai.trm"
#endif /* AIFM */

#ifdef COREL
#include "term/corel.trm"
#endif /* COREL */

#ifdef CGI
#include "term/cgi.trm"
#endif /* CGI */

#ifdef DEBUG
#include "term/debug.trm"
#endif /* DEBUG */

#ifdef EXCL
#include "term/excl.trm"
#endif /* EXCL */

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

/* HPGL - includes HP75 and HPLJIII in HPGL mode */
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

#ifdef NEXT
#include "term/next.trm"
#endif /* NEXT */

#ifdef QMS
#include "term/qms.trm"
#endif /* QMS */

#ifdef REGIS
#include "term/regis.trm"
#endif /* REGIS */

#ifdef RGIP
#include "term/rgip.trm"
#endif /* RGIP UNIPLEX */

#ifdef MGR
#include "term/mgr.trm"
#endif /* MGR */

#ifdef SUN
#include "term/sun.trm"
#endif /* SUN */

#ifdef VWS
#include "term/vws.trm"
#endif /* VWS */

#ifdef V384
#include "term/v384.trm"
#endif /* V384 */

#ifdef TGIF
#include "term/tgif.trm"
#endif /* TGIF */

#ifdef UNIXPLOT
#ifdef GNUGRAPH
#include "term/gnugraph.trm"
#else
#include "term/unixplot.trm"
#endif /* GNUGRAPH */
#endif /* UNIXPLOT */

#ifdef X11
#include "term/x11.trm"
#include "term/xlib.trm"
#endif /* X11 */

#ifdef DXF
#include "term/dxf.trm"
#endif /* DXF */
  
#ifdef AMIGASCREEN
#include "term/amiga.trm"
#endif /* AMIGASCREEN */

/* Dummy functions for unavailable features */

/* change angle of text.  0 is horizontal left to right.
* 1 is vertical bottom to top (90 deg rotate)  
*/
static int null_text_angle()
{
return FALSE ;	/* can't be done */
}

/* change justification of text.  
 * modes are LEFT (flush left), CENTRE (centred), RIGHT (flush right)
 */
static int null_justify_text()
{
return FALSE ;	/* can't be done */
}


/* Change scale of plot.
 * Parameters are x,y scaling factors for this plot.
 * Some terminals (eg latex) need to do scaling themselves.
 */
static int null_scale()
{
return FALSE ;	/* can't be done */
}

static int do_scale()
{
return TRUE ;	/* can be done */
}

options_null()
{
	term_options[0] = '\0';	/* we have no options */
}

static UNKNOWN_null()
{
}

/*
 * term_tbl[] contains an entry for each terminal.  "unknown" must be the
 *   first, since term is initialized to 0.
 */
struct termentry term_tbl[] = {
    {"unknown", "Unknown terminal type - not a plotting device",
	  100, 100, 1, 1,
	  1, 1, options_null, UNKNOWN_null, UNKNOWN_null, 
	  UNKNOWN_null, null_scale, UNKNOWN_null, UNKNOWN_null, UNKNOWN_null, 
	  UNKNOWN_null, UNKNOWN_null, null_text_angle, 
	  null_justify_text, UNKNOWN_null, UNKNOWN_null}

    ,{"table", "Dump ASCII table of X Y [Z] values to output",
	  100, 100, 1, 1,
	  1, 1, options_null, UNKNOWN_null, UNKNOWN_null, 
	  UNKNOWN_null, null_scale, UNKNOWN_null, UNKNOWN_null, UNKNOWN_null, 
	  UNKNOWN_null, UNKNOWN_null, null_text_angle, 
	  null_justify_text, UNKNOWN_null, UNKNOWN_null}

#ifdef AMIGASCREEN
    ,{"amiga", "Amiga Custom Screen",
	   AMIGA_XMAX, AMIGA_YMAX, AMIGA_VCHAR, AMIGA_HCHAR, 
	   AMIGA_VTIC, AMIGA_HTIC, options_null, AMIGA_init, AMIGA_reset, 
	   AMIGA_text, null_scale, AMIGA_graphics, AMIGA_move, AMIGA_vector,
	   AMIGA_linetype, AMIGA_put_text, null_text_angle, 
	   AMIGA_justify_text, do_point, do_arrow}
#endif

#ifdef ATARI
    ,{"atari", "Atari ST/TT",
	   ATARI_XMAX, ATARI_YMAX, ATARI_VCHAR, ATARI_HCHAR, 
	   ATARI_VTIC, ATARI_HTIC, ATARI_options, ATARI_init, ATARI_reset, 
	   ATARI_text, null_scale, ATARI_graphics, ATARI_move, ATARI_vector, 
	   ATARI_linetype, ATARI_put_text, ATARI_text_angle, 
	   null_justify_text, ATARI_point, do_arrow}
#endif

#ifdef DUMB
    ,{"dumb", "printer or glass dumb terminal",
         DUMB_XMAX, DUMB_YMAX, 1, 1,
         1, 1, DUMB_options, DUMB_init, DUMB_reset,
         DUMB_text, null_scale, DUMB_graphics, DUMB_move, DUMB_vector,
         DUMB_linetype, DUMB_put_text, null_text_angle,
         null_justify_text, DUMB_point, DUMB_arrow}
#endif

#ifdef PC
#ifndef _Windows
# ifdef __TURBOC__
#ifdef ATT6300
    ,{"att", "IBM PC/Clone with AT&T 6300 graphics board",
	   ATT_XMAX, ATT_YMAX, ATT_VCHAR, ATT_HCHAR,
	   ATT_VTIC, ATT_HTIC, options_null, ATT_init, ATT_reset,
	   ATT_text, null_scale, ATT_graphics, ATT_move, ATT_vector,
	   ATT_linetype, ATT_put_text, ATT_text_angle, 
	   ATT_justify_text, line_and_point, do_arrow}
#endif

    ,{"cga", "IBM PC/Clone with CGA graphics board",
	   CGA_XMAX, CGA_YMAX, CGA_VCHAR, CGA_HCHAR,
	   CGA_VTIC, CGA_HTIC, options_null, CGA_init, CGA_reset,
	   CGA_text, null_scale, CGA_graphics, CGA_move, CGA_vector,
	   CGA_linetype, CGA_put_text, MCGA_text_angle, 
	   CGA_justify_text, line_and_point, do_arrow}

    ,{"egalib", "IBM PC/Clone with EGA graphics board",
	   EGALIB_XMAX, EGALIB_YMAX, EGALIB_VCHAR, EGALIB_HCHAR,
	   EGALIB_VTIC, EGALIB_HTIC, options_null, EGALIB_init, EGALIB_reset,
	   EGALIB_text, null_scale, EGALIB_graphics, EGALIB_move, EGALIB_vector,
	   EGALIB_linetype, EGALIB_put_text, EGALIB_text_angle, 
	   EGALIB_justify_text, do_point, do_arrow}

    ,{"hercules", "IBM PC/Clone with Hercules graphics board",
	   HERC_XMAX, HERC_YMAX, HERC_VCHAR, HERC_HCHAR,
	   HERC_VTIC, HERC_HTIC, options_null, HERC_init, HERC_reset,
	   HERC_text, null_scale, HERC_graphics, HERC_move, HERC_vector,
	   HERC_linetype, HERC_put_text, MCGA_text_angle, 
	   HERC_justify_text, line_and_point, do_arrow}

    ,{"mcga", "IBM PC/Clone with MCGA graphics board",
	   MCGA_XMAX, MCGA_YMAX, MCGA_VCHAR, MCGA_HCHAR,
	   MCGA_VTIC, MCGA_HTIC, options_null, MCGA_init, MCGA_reset,
	   MCGA_text, null_scale, MCGA_graphics, MCGA_move, MCGA_vector,
	   MCGA_linetype, MCGA_put_text, MCGA_text_angle, 
	   MCGA_justify_text, line_and_point, do_arrow}

    ,{"svga", "IBM PC/Clone with Super VGA graphics board",
	   SVGA_XMAX, SVGA_YMAX, SVGA_VCHAR, SVGA_HCHAR,
	   SVGA_VTIC, SVGA_HTIC, options_null, SVGA_init, SVGA_reset,
	   SVGA_text, null_scale, SVGA_graphics, SVGA_move, SVGA_vector,
	   SVGA_linetype, SVGA_put_text, SVGA_text_angle, 
	   SVGA_justify_text, do_point, do_arrow}

    ,{"vgalib", "IBM PC/Clone with VGA graphics board",
	   VGA_XMAX, VGA_YMAX, VGA_VCHAR, VGA_HCHAR,
	   VGA_VTIC, VGA_HTIC, options_null, VGA_init, VGA_reset,
	   VGA_text, null_scale, VGA_graphics, VGA_move, VGA_vector,
	   VGA_linetype, VGA_put_text, VGA_text_angle, 
	   VGA_justify_text, do_point, do_arrow}

    ,{"vgamono", "IBM PC/Clone with VGA Monochrome graphics board",
	   VGA_XMAX, VGA_YMAX, VGA_VCHAR, VGA_HCHAR,
	   VGA_VTIC, VGA_HTIC, options_null, VGA_init, VGA_reset,
	   VGA_text, null_scale, VGA_graphics, VGA_move, VGA_vector,
	   VGAMONO_linetype, VGA_put_text, VGA_text_angle, 
	   VGA_justify_text, line_and_point, do_arrow}
#else					/* TURBO */

#ifdef ATT6300
    ,{"att", "AT&T PC/6300 graphics",
	   ATT_XMAX, ATT_YMAX, ATT_VCHAR, ATT_HCHAR,
	   ATT_VTIC, ATT_HTIC, options_null, ATT_init, ATT_reset,
	   ATT_text, null_scale, ATT_graphics, ATT_move, ATT_vector,
	   ATT_linetype, ATT_put_text, ATT_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

    ,{"cga", "IBM PC/Clone with CGA graphics board",
	   CGA_XMAX, CGA_YMAX, CGA_VCHAR, CGA_HCHAR,
	   CGA_VTIC, CGA_HTIC, options_null, CGA_init, CGA_reset,
	   CGA_text, null_scale, CGA_graphics, CGA_move, CGA_vector,
	   CGA_linetype, CGA_put_text, CGA_text_angle, 
	   null_justify_text, line_and_point, do_arrow}

#ifdef CORONA
    ,{"corona325", "Corona graphics ???",
	   COR_XMAX, COR_YMAX, COR_VCHAR, COR_HCHAR,
	   COR_VTIC, COR_HTIC, options_null, COR_init, COR_reset,
	   COR_text, null_scale, COR_graphics, COR_move, COR_vector,
	   COR_linetype, COR_put_text, COR_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif					/* CORONA */

    ,{"egabios", "IBM PC/Clone with EGA graphics board (BIOS)",
	   EGA_XMAX, EGA_YMAX, EGA_VCHAR, EGA_HCHAR,
	   EGA_VTIC, EGA_HTIC, options_null, EGA_init, EGA_reset,
	   EGA_text, null_scale, EGA_graphics, EGA_move, EGA_vector,
	   EGA_linetype, EGA_put_text, EGA_text_angle, 
	   null_justify_text, do_point, do_arrow}

#ifdef EGALIB
    ,{"egalib", "IBM PC/Clone with EGA graphics board (LIB)",
	   EGALIB_XMAX, EGALIB_YMAX, EGALIB_VCHAR, EGALIB_HCHAR,
	   EGALIB_VTIC, EGALIB_HTIC, options_null, EGALIB_init, EGALIB_reset,
	   EGALIB_text, null_scale, EGALIB_graphics, EGALIB_move, EGALIB_vector,
	   EGALIB_linetype, EGALIB_put_text, null_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef HERCULES
    ,{"hercules", "IBM PC/Clone with Hercules graphics board",
	   HERC_XMAX, HERC_YMAX, HERC_VCHAR, HERC_HCHAR,
	   HERC_VTIC, HERC_HTIC, options_null, HERC_init, HERC_reset,
	   HERC_text, null_scale, HERC_graphics, HERC_move, HERC_vector,
	   HERC_linetype, HERC_put_text, HERC_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif					/* HERCULES */

    ,{"vgabios", "IBM PC/Clone with VGA graphics board (BIOS)",
	   VGA_XMAX, VGA_YMAX, VGA_VCHAR, VGA_HCHAR,
	   VGA_VTIC, VGA_HTIC, options_null, VGA_init, VGA_reset,
	   VGA_text, null_scale, VGA_graphics, VGA_move, VGA_vector,
	   VGA_linetype, VGA_put_text, VGA_text_angle, 
	   null_justify_text, do_point, do_arrow}

#endif					/* TURBO */
#endif					/* _Windows */
#endif					/* PC */

#ifdef __ZTC__		 /* zortech C flashgraphics for 386 */
	,{"hercules", "IBM PC/Clone with Hercules graphics board",
	   HERC_XMAX, HERC_YMAX, HERC_VCHAR, HERC_HCHAR,
	   HERC_VTIC, HERC_HTIC, options_null, VGA_init, VGA_reset,
	   VGA_text, null_scale, HERC_graphics, VGA_move, VGA_vector,
	   VGA_linetype, VGA_put_text, VGA_text_angle, 
       VGA_justify_text, do_point, do_arrow}

	,{"egamono", "IBM PC/Clone with monochrome EGA graphics board",
	   EGA_XMAX, EGA_YMAX, EGA_VCHAR, EGA_HCHAR,
	   EGA_VTIC, EGA_HTIC, options_null, VGA_init, VGA_reset,
	   VGA_text, null_scale, EGAMONO_graphics, VGA_move, VGA_vector,
	   VGA_linetype, VGA_put_text, VGA_text_angle, 
       VGA_justify_text, do_point, do_arrow}

	,{"egalib", "IBM PC/Clone with color EGA graphics board",
	   EGA_XMAX, EGA_YMAX, EGA_VCHAR, EGA_HCHAR,
	   EGA_VTIC, EGA_HTIC, options_null, VGA_init, VGA_reset,
	   VGA_text, null_scale, EGA_graphics, VGA_move, VGA_vector,
	   VGA_linetype, VGA_put_text, VGA_text_angle, 
       VGA_justify_text, do_point, do_arrow}

	,{"vgalib", "IBM PC/Clone with VGA graphics board",
	   VGA_XMAX, VGA_YMAX, VGA_VCHAR, VGA_HCHAR,
	   VGA_VTIC, VGA_HTIC, options_null, VGA_init, VGA_reset,
	   VGA_text, null_scale, VGA_graphics, VGA_move, VGA_vector,
	   VGA_linetype, VGA_put_text, VGA_text_angle, 
       VGA_justify_text, do_point, do_arrow}

	,{"vgamono", "IBM PC/Clone with monochrome VGA graphics board",
	   VGA_XMAX, VGA_YMAX, VGA_VCHAR, VGA_HCHAR,
	   VGA_VTIC, VGA_HTIC, options_null, VGA_init, VGA_reset,
	   VGA_text, null_scale, VGAMONO_graphics, VGA_move, VGA_vector,
	   VGA_linetype, VGA_put_text, VGA_text_angle, 
       VGA_justify_text, do_point, do_arrow}

	,{"svgalib", "IBM PC/Clone with VESA Super VGA graphics board",
	   SVGA_XMAX, SVGA_YMAX, SVGA_VCHAR, SVGA_HCHAR,
	   SVGA_VTIC, SVGA_HTIC, options_null, VGA_init, VGA_reset,
	   VGA_text, null_scale, SVGA_graphics, VGA_move, VGA_vector,
	   VGA_linetype, VGA_put_text, VGA_text_angle, 
	   VGA_justify_text, do_point, do_arrow}

	,{"ssvgalib", "IBM PC/Clone with VESA 256 color 1024 by 768 super VGA",
	   SSVGA_XMAX, SSVGA_YMAX, SSVGA_VCHAR, SSVGA_HCHAR,
	   SSVGA_VTIC, SSVGA_HTIC, options_null, VGA_init, VGA_reset,
	   VGA_text, null_scale, SSVGA_graphics, VGA_move, VGA_vector,
	   VGA_linetype, VGA_put_text, VGA_text_angle, 
       VGA_justify_text, do_point, do_arrow}
#endif	/* __ZTC__ */

#ifdef AED
    ,{"aed512", "AED 512 Terminal",
	   AED5_XMAX, AED_YMAX, AED_VCHAR, AED_HCHAR,
	   AED_VTIC, AED_HTIC, options_null, AED_init, AED_reset, 
	   AED_text, null_scale, AED_graphics, AED_move, AED_vector, 
	   AED_linetype, AED_put_text, null_text_angle, 
	   null_justify_text, do_point, do_arrow}
    ,{"aed767", "AED 767 Terminal",
	   AED_XMAX, AED_YMAX, AED_VCHAR, AED_HCHAR,
	   AED_VTIC, AED_HTIC, options_null, AED_init, AED_reset, 
	   AED_text, null_scale, AED_graphics, AED_move, AED_vector, 
	   AED_linetype, AED_put_text, null_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef AIFM
    ,{"aifm", "Adobe Illustrator 3.0 Format",
	   AI_XMAX, AI_YMAX, AI_VCHAR, AI_HCHAR, 
	   AI_VTIC, AI_HTIC, AI_options, AI_init, AI_reset, 
	   AI_text, null_scale, AI_graphics, AI_move, AI_vector, 
	   AI_linetype, AI_put_text, AI_text_angle, 
	   AI_justify_text, do_point, do_arrow}
#endif

#ifdef APOLLO
   	,{"apollo", "Apollo Graphics Primitive Resource, rescaling of subsequent plots after window resizing",
	   0, 0, 0, 0, /* APOLLO_XMAX, APOLLO_YMAX, APOLLO_VCHAR, APOLLO_HCHAR, are filled in at run-time */
	   APOLLO_VTIC, APOLLO_HTIC, options_null, APOLLO_init, APOLLO_reset,
	   APOLLO_text, null_scale, APOLLO_graphics, APOLLO_move, APOLLO_vector,
	   APOLLO_linetype, APOLLO_put_text, APOLLO_text_angle,
	   APOLLO_justify_text, line_and_point, do_arrow}
#endif

#ifdef GPR
   	,{"gpr", "Apollo Graphics Primitive Resource, fixed-size window",
	   GPR_XMAX, GPR_YMAX, GPR_VCHAR, GPR_HCHAR,
	   GPR_VTIC, GPR_HTIC, options_null, GPR_init, GPR_reset,
	   GPR_text, null_scale, GPR_graphics, GPR_move, GPR_vector,
	   GPR_linetype, GPR_put_text, GPR_text_angle,
	   GPR_justify_text, line_and_point, do_arrow}
#endif

#ifdef BITGRAPH
    ,{"bitgraph", "BBN Bitgraph Terminal",
	   BG_XMAX,BG_YMAX,BG_VCHAR, BG_HCHAR, 
	   BG_VTIC, BG_HTIC, options_null, BG_init, BG_reset, 
	   BG_text, null_scale, BG_graphics, BG_move, BG_vector,
	   BG_linetype, BG_put_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef COREL
    ,{"corel","EPS format for CorelDRAW",
     CORELD_XMAX, CORELD_YMAX, CORELD_VCHAR, CORELD_HCHAR,
     CORELD_VTIC, CORELD_HTIC, COREL_options, COREL_init, COREL_reset,
     COREL_text, null_scale, COREL_graphics, COREL_move, COREL_vector,
     COREL_linetype, COREL_put_text, COREL_text_angle,
     COREL_justify_text, do_point, do_arrow}
#endif

#ifdef CGI
    ,{"cgi", "SCO CGI drivers (requires CGIDISP or CGIPRNT env variable)",
	   CGI_XMAX, CGI_YMAX, 0, 0, 
	   CGI_VTIC, 0, options_null, CGI_init, CGI_reset, 
	   CGI_text, null_scale, CGI_graphics, CGI_move, CGI_vector, 
	   CGI_linetype, CGI_put_text, CGI_text_angle, 
	   CGI_justify_text, CGI_point, do_arrow}

    ,{"hcgi", "SCO CGI drivers (hardcopy, requires CGIPRNT env variable)",
	   CGI_XMAX, CGI_YMAX, 0, 0, 
	   CGI_VTIC, 0, options_null, HCGI_init, CGI_reset, 
	   CGI_text, null_scale, CGI_graphics, CGI_move, CGI_vector, 
	   CGI_linetype, CGI_put_text, CGI_text_angle, 
	   CGI_justify_text, CGI_point, do_arrow}
#endif

#ifdef DEBUG
    ,{"debug", "debugging driver",
	   DEBUG_XMAX, DEBUG_YMAX, DEBUG_VCHAR, DEBUG_HCHAR,
	   DEBUG_VTIC, DEBUG_HTIC, options_null, DEBUG_init, DEBUG_reset,
	   DEBUG_text, null_scale, DEBUG_graphics, DEBUG_move, DEBUG_vector,
	   DEBUG_linetype, DEBUG_put_text, null_text_angle, 
	   DEBUG_justify_text, line_and_point, do_arrow}
#endif

#ifdef DJSVGA
    ,{"svga", "IBM PC/Clone with Super VGA graphics board",
	   DJSVGA_XMAX, DJSVGA_YMAX, DJSVGA_VCHAR, DJSVGA_HCHAR,
	   DJSVGA_VTIC, DJSVGA_HTIC, options_null, DJSVGA_init, DJSVGA_reset,
	   DJSVGA_text, null_scale, DJSVGA_graphics, DJSVGA_move, DJSVGA_vector,
	   DJSVGA_linetype, DJSVGA_put_text, null_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef DXF
    ,{"dxf", "dxf-file for AutoCad (default size 120x80)",
       DXF_XMAX,DXF_YMAX,DXF_VCHAR, DXF_HCHAR,
       DXF_VTIC, DXF_HTIC, options_null,DXF_init, DXF_reset,
       DXF_text, null_scale, DXF_graphics, DXF_move, DXF_vector,
       DXF_linetype, DXF_put_text, DXF_text_angle,
       DXF_justify_text, do_point, do_arrow}
#endif

#ifdef DXY800A
    ,{"dxy800a", "Roland DXY800A plotter",
	   DXY_XMAX, DXY_YMAX, DXY_VCHAR, DXY_HCHAR,
	   DXY_VTIC, DXY_HTIC, options_null, DXY_init, DXY_reset,
	   DXY_text, null_scale, DXY_graphics, DXY_move, DXY_vector,
	   DXY_linetype, DXY_put_text, DXY_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef EEPIC
    ,{"eepic", "EEPIC -- extended LaTeX picture environment",
	   EEPIC_XMAX, EEPIC_YMAX, EEPIC_VCHAR, EEPIC_HCHAR, 
	   EEPIC_VTIC, EEPIC_HTIC, options_null, EEPIC_init, EEPIC_reset, 
	   EEPIC_text, EEPIC_scale, EEPIC_graphics, EEPIC_move, EEPIC_vector, 
	   EEPIC_linetype, EEPIC_put_text, EEPIC_text_angle, 
	   EEPIC_justify_text, EEPIC_point, EEPIC_arrow}
#endif

#ifdef EMTEX
    ,{"emtex", "LaTeX picture environment with emTeX specials",
	   LATEX_XMAX, LATEX_YMAX, LATEX_VCHAR, LATEX_HCHAR, 
	   LATEX_VTIC, LATEX_HTIC, LATEX_options, EMTEX_init, EMTEX_reset, 
	   EMTEX_text, LATEX_scale, LATEX_graphics, LATEX_move, LATEX_vector, 
	   LATEX_linetype, LATEX_put_text, LATEX_text_angle, 
	   LATEX_justify_text, LATEX_point, LATEX_arrow}
#endif

#ifdef EPS180
    ,{"epson_180dpi", "Epson LQ-style 180-dot per inch (24 pin) printers",
	   EPS180XMAX, EPS180YMAX, EPSON180VCHAR, EPSON180HCHAR,
	   EPSON180VTIC, EPSON180HTIC, options_null, EPSONinit, EPSONreset,
	   EPS180text, null_scale, EPS180graphics, EPSONmove, EPSONvector,
	   EPSONlinetype, EPSONput_text, EPSON_text_angle,
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef EPS60
    ,{"epson_60dpi", "Epson-style 60-dot per inch printers",
	   EPS60XMAX, EPS60YMAX, EPSONVCHAR, EPSONHCHAR,
	   EPSONVTIC, EPSONHTIC, options_null, EPSONinit, EPSONreset,
	   EPS60text, null_scale, EPS60graphics, EPSONmove, EPSONvector,
	   EPSONlinetype, EPSONput_text, EPSON_text_angle,
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef EPSONP
    ,{"epson_lx800", "Epson LX-800, Star NL-10, NX-1000, PROPRINTER ...",
	   EPSONXMAX, EPSONYMAX, EPSONVCHAR, EPSONHCHAR, 
	   EPSONVTIC, EPSONHTIC, options_null, EPSONinit, EPSONreset, 
	   EPSONtext, null_scale, EPSONgraphics, EPSONmove, EPSONvector, 
	   EPSONlinetype, EPSONput_text, EPSON_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef EXCL
    ,{"excl", "Talaris EXCL Laser printer (also Talaris 1590 and others)",
	   EXCL_XMAX,EXCL_YMAX, EXCL_VCHAR, EXCL_HCHAR, 
	   EXCL_VTIC, EXCL_HTIC, options_null, EXCL_init,EXCL_reset, 
	   EXCL_text, null_scale, EXCL_graphics, EXCL_move, EXCL_vector,
	   EXCL_linetype,EXCL_put_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif


#ifdef FIG
    ,{"fig", "FIG 2.1 graphics language: SunView or X graphics editor",
	   FIG_XMAX, FIG_YMAX, FIG_VCHAR, FIG_HCHAR, 
	   FIG_VTIC, FIG_HTIC, FIG_options, FIG_init, FIG_reset, 
	   FIG_text, null_scale, FIG_graphics, FIG_move, FIG_vector, 
	   FIG_linetype, FIG_put_text, FIG_text_angle, 
	   FIG_justify_text, line_and_point, FIG_arrow}
    ,{"bfig", "FIG 2.1 graphics lang: SunView or X graphics editor. Large Graph",
	   BFIG_XMAX, BFIG_YMAX, BFIG_VCHAR, BFIG_HCHAR, 
	   BFIG_VTIC, BFIG_HTIC, FIG_options, FIG_init, FIG_reset, 
	   FIG_text, null_scale, FIG_graphics, FIG_move, BFIG_vector, 
	   FIG_linetype, BFIG_put_text, FIG_text_angle, 
	   FIG_justify_text, line_and_point, BFIG_arrow}
#endif

#ifdef GPIC
    ,{"gpic", "GPIC -- Produce graphs in groff using the gpic preprocessor",
	   GPIC_XMAX, GPIC_YMAX, GPIC_VCHAR, GPIC_HCHAR, 
	   GPIC_VTIC, GPIC_HTIC, GPIC_options, GPIC_init, GPIC_reset, 
	   GPIC_text, GPIC_scale, GPIC_graphics, GPIC_move, GPIC_vector, 
	   GPIC_linetype, GPIC_put_text, GPIC_text_angle, 
	   GPIC_justify_text, line_and_point, GPIC_arrow}
#endif

#ifdef GRASS
    ,{"grass", "GRASS Graphics Monitor",
           GRASS_XMAX, GRASS_YMAX, GRASS_VCHAR, GRASS_HCHAR,
           GRASS_VTIC, GRASS_HTIC, GRASS_options, GRASS_init, GRASS_reset,
           GRASS_text, null_scale, GRASS_graphics, GRASS_move, GRASS_vector,
           GRASS_linetype, GRASS_put_text, GRASS_text_angle,
           GRASS_justify_text, GRASS_point, GRASS_arrow}
#endif

#ifdef HP26
    ,{"hp2623A", "HP2623A and maybe others",
	   HP26_XMAX, HP26_YMAX, HP26_VCHAR, HP26_HCHAR,
	   HP26_VTIC, HP26_HTIC, options_null, HP26_init, HP26_reset,
	   HP26_text, null_scale, HP26_graphics, HP26_move, HP26_vector,
	   HP26_linetype, HP26_put_text, HP26_text_angle, 
	   null_justify_text, HP26_line_and_point, do_arrow}
#endif

#ifdef HP2648
    ,{"hp2648", "HP2648 and HP2647",
	   HP2648XMAX, HP2648YMAX, HP2648VCHAR, HP2648HCHAR, 
	   HP2648VTIC, HP2648HTIC, options_null, HP2648init, HP2648reset, 
	   HP2648text, null_scale, HP2648graphics, HP2648move, HP2648vector, 
	   HP2648linetype, HP2648put_text, HP2648_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef HP75
    ,{"hp7580B", "HP7580, and probably other HPs (4 pens)",
	   HPGL_XMAX, HPGL_YMAX, HPGL_VCHAR, HPGL_HCHAR,
	   HPGL_VTIC, HPGL_HTIC, options_null, HPGL_init, HPGL_reset,
	   HPGL_text, null_scale, HPGL_graphics, HPGL_move, HPGL_vector,
	   HP75_linetype, HPGL_put_text, HPGL_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef HP500C
      ,{"hp500c", "HP DeskJet 500c, [75 100 150 300] [rle tiff]",
       HP500C_75PPI_XMAX, HP500C_75PPI_YMAX, HP500C_75PPI_VCHAR,
       HP500C_75PPI_HCHAR, HP500C_75PPI_VTIC, HP500C_75PPI_HTIC, HP500Coptions,
       HP500Cinit, HP500Creset, HP500Ctext, null_scale,
       HP500Cgraphics, HP500Cmove, HP500Cvector, HP500Clinetype,
       HP500Cput_text, HP500Ctext_angle, null_justify_text, do_point,
       do_arrow}
#endif

#ifdef HPGL
    ,{"hpgl", "HP7475 and (hopefully) lots of others (6 pens)",
	   HPGL_XMAX, HPGL_YMAX, HPGL_VCHAR, HPGL_HCHAR,
	   HPGL_VTIC, HPGL_HTIC, options_null, HPGL_init, HPGL_reset,
	   HPGL_text, null_scale, HPGL_graphics, HPGL_move, HPGL_vector,
	   HPGL_linetype, HPGL_put_text, HPGL_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef HPLJII
    ,{"hpljii", "HP Laserjet series II, [75 100 150 300]",
       HPLJII_75PPI_XMAX, HPLJII_75PPI_YMAX, HPLJII_75PPI_VCHAR,
       HPLJII_75PPI_HCHAR, HPLJII_75PPI_VTIC, HPLJII_75PPI_HTIC, HPLJIIoptions,
       HPLJIIinit, HPLJIIreset, HPLJIItext, null_scale,
       HPLJIIgraphics, HPLJIImove, HPLJIIvector, HPLJIIlinetype,
       HPLJIIput_text, HPLJIItext_angle, null_justify_text, line_and_point,
       do_arrow}
    ,{"hpdj", "HP DeskJet 500, [75 100 150 300]",
       HPLJII_75PPI_XMAX, HPLJII_75PPI_YMAX, HPLJII_75PPI_VCHAR,
       HPLJII_75PPI_HCHAR, HPLJII_75PPI_VTIC, HPLJII_75PPI_HTIC, HPLJIIoptions,
       HPLJIIinit, HPLJIIreset, HPDJtext, null_scale,
       HPDJgraphics, HPLJIImove, HPLJIIvector, HPLJIIlinetype,
       HPDJput_text, HPDJtext_angle, null_justify_text, line_and_point,
       do_arrow}
#endif

#ifdef HPPJ
    ,{"hppj", "HP PaintJet and HP3630 [FNT5X9 FNT9X17 FNT13X25]",
       HPPJ_XMAX, HPPJ_YMAX,
       HPPJ_9x17_VCHAR, HPPJ_9x17_HCHAR, HPPJ_9x17_VTIC, HPPJ_9x17_HTIC,
       HPPJoptions, HPPJinit, HPPJreset, HPPJtext, null_scale, HPPJgraphics,
       HPPJmove, HPPJvector, HPPJlinetype, HPPJput_text, HPPJtext_angle,
       null_justify_text, do_point, do_arrow}
#endif

#ifdef IMAGEN
    ,{"imagen", "Imagen laser printer",
	   IMAGEN_XMAX, IMAGEN_YMAX, IMAGEN_VCHAR, IMAGEN_HCHAR, 
	   IMAGEN_VTIC, IMAGEN_HTIC, options_null, IMAGEN_init, IMAGEN_reset, 
	   IMAGEN_text, null_scale, IMAGEN_graphics, IMAGEN_move, 
	   IMAGEN_vector, IMAGEN_linetype, IMAGEN_put_text, IMAGEN_text_angle,
	   IMAGEN_justify_text, line_and_point, do_arrow}
#endif

#ifdef IRIS4D
    ,{"iris4d", "Silicon Graphics IRIS 4D Series Computer",
	   IRIS4D_XMAX, IRIS4D_YMAX, IRIS4D_VCHAR, IRIS4D_HCHAR, 
	   IRIS4D_VTIC, IRIS4D_HTIC, IRIS4D_options, IRIS4D_init, IRIS4D_reset, 
	   IRIS4D_text, null_scale, IRIS4D_graphics, IRIS4D_move, IRIS4D_vector,
	   IRIS4D_linetype, IRIS4D_put_text, null_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef KERMIT
    ,{"kc_tek40xx", "MS-DOS Kermit Tek4010 terminal emulator - color",
	   TEK40XMAX,TEK40YMAX,TEK40VCHAR, KTEK40HCHAR, 
	   TEK40VTIC, TEK40HTIC, options_null, TEK40init, KTEK40reset, 
	   KTEK40Ctext, null_scale, KTEK40graphics, TEK40move, TEK40vector, 
	   KTEK40Clinetype, TEK40put_text, null_text_angle, 
	   null_justify_text, do_point, do_arrow}
    ,{"km_tek40xx", "MS-DOS Kermit Tek4010 terminal emulator - monochrome",
	   TEK40XMAX,TEK40YMAX,TEK40VCHAR, KTEK40HCHAR, 
	   TEK40VTIC, TEK40HTIC, options_null, TEK40init, KTEK40reset, 
	   TEK40text, null_scale, KTEK40graphics, TEK40move, TEK40vector, 
	   KTEK40Mlinetype, TEK40put_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef LATEX
    ,{"latex", "LaTeX picture environment",
	   LATEX_XMAX, LATEX_YMAX, LATEX_VCHAR, LATEX_HCHAR, 
	   LATEX_VTIC, LATEX_HTIC, LATEX_options, LATEX_init, LATEX_reset, 
	   LATEX_text, LATEX_scale, LATEX_graphics, LATEX_move, LATEX_vector, 
	   LATEX_linetype, LATEX_put_text, LATEX_text_angle, 
	   LATEX_justify_text, LATEX_point, LATEX_arrow}
#endif

#ifdef LN03P
     ,{"ln03", "LN03-plus laser printer in tektronix (EGM) mode",
	LN03PXMAX, LN03PYMAX, LN03PVCHAR, LN03PHCHAR,
	LN03PVTIC, LN03PHTIC, options_null, LN03Pinit, LN03Preset,
	LN03Ptext, null_scale, TEK40graphics, LN03Pmove, LN03Pvector,
	LN03Plinetype, LN03Pput_text, null_text_angle,
	null_justify_text, line_and_point, do_arrow}
#endif

#ifdef MF
    ,{"mf", "Metafont plotting standard",
	   MF_XMAX, MF_YMAX, MF_VCHAR, MF_HCHAR, 
       MF_VTIC, MF_HTIC, options_null, MF_init, MF_reset, 
	   MF_text, MF_scale, MF_graphics, MF_move, MF_vector, 
	   MF_linetype, MF_put_text, MF_text_angle, 
	   MF_justify_text, line_and_point, MF_arrow}
#endif

#ifdef MGR
    ,{"mgr", "Mgr window system",
	/* dimensions nominal, replaced during MGR_graphics call */
	   MGR_XMAX, MGR_YMAX, MGR_VCHAR, MGR_HCHAR, 
	   MGR_VTIC, MGR_HTIC, options_null, MGR_init, MGR_reset, 
	   MGR_text, null_scale, MGR_graphics, MGR_move, MGR_vector,
	   MGR_linetype, MGR_put_text, null_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef MIF
    ,{"mif", "Frame maker MIF 3.00 format",
      	MIF_XMAX, MIF_YMAX, MIF_VCHAR, MIF_HCHAR, 
 	    MIF_VTIC, MIF_HTIC, MIF_options, MIF_init, MIF_reset, 
	    MIF_text, null_scale, MIF_graphics, MIF_move, MIF_vector, 
	    MIF_linetype, MIF_put_text, MIF_text_angle, 
	    MIF_justify_text, line_and_point, do_arrow}
#endif

#ifdef NEC
    ,{"nec_cp6", "NEC printer CP6, Epson LQ-800 [monocrome color draft]",
	   NECXMAX, NECYMAX, NECVCHAR, NECHCHAR, 
	   NECVTIC, NECHTIC, NECoptions, NECinit, NECreset, 
	   NECtext, null_scale, NECgraphics, NECmove, NECvector, 
	   NEClinetype, NECput_text, NEC_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef NEXT
    ,{"next", "NeXTstep window system",
	   NEXT_XMAX, NEXT_YMAX, NEXT_VCHAR, NEXT_HCHAR, 
	   NEXT_VTIC, NEXT_HTIC, NEXT_options, NEXT_init, NEXT_reset, 
	   NEXT_text, do_scale, NEXT_graphics, NEXT_move, NEXT_vector, 
	   NEXT_linetype, NEXT_put_text, NEXT_text_angle, 
	   NEXT_justify_text, NEXT_point, do_arrow}
#endif /* The PostScript driver with NXImage displaying the PostScript on screen */

#ifdef OKIDATA
    ,{"okidata", "OKIDATA 320/321 Standard",
	   EPS60XMAX, EPS60YMAX, EPSONVCHAR, EPSONHCHAR,
	   EPSONVTIC, EPSONHTIC, options_null, EPSONinit, EPSONreset,
	   OKIDATAtext, null_scale, EPS60graphics, EPSONmove, EPSONvector,
	   EPSONlinetype, EPSONput_text, EPSON_text_angle,
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef OS2PM
    ,{"pm", "OS/2 Presentation Manager",
	   PM_XMAX, PM_YMAX, PM_VCHAR, PM_HCHAR, 
	   PM_VTIC, PM_HTIC, PM_args, PM_init, PM_reset, 
	   PM_text, null_scale, PM_graphics, PM_move, PM_vector,
	   PM_linetype, PM_put_text, PM_text_angle, 
	   PM_justify_text, PM_point, do_arrow}
#endif

#ifdef PBM
    ,{"pbm", "Portable bitmap [small medium large] [monochrome gray color]",
       PBM_XMAX, PBM_YMAX, PBM_VCHAR,
       PBM_HCHAR, PBM_VTIC, PBM_HTIC, PBMoptions,
       PBMinit, PBMreset, PBMtext, null_scale,
       PBMgraphics, PBMmove, PBMvector, PBMlinetype,
       PBMput_text, PBMtext_angle, null_justify_text, PBMpoint,
       do_arrow}
#endif

#ifdef PCL
 ,{"pcl5", "HP LaserJet III [mode] [font] [point]",
   PCL_XMAX, PCL_YMAX, HPGL2_VCHAR, HPGL2_HCHAR,
   PCL_VTIC, PCL_HTIC, PCL_options, PCL_init, PCL_reset,
   PCL_text, null_scale, PCL_graphics, HPGL2_move, HPGL2_vector,
   HPGL2_linetype, HPGL2_put_text, HPGL2_text_angle,
   HPGL2_justify_text, do_point, do_arrow}
#endif

#ifdef POSTSCRIPT
    ,{"postscript", "PostScript graphics language [mode \042fontname\042 font_size]",
	   PS_XMAX, PS_YMAX, PS_VCHAR, PS_HCHAR, 
	   PS_VTIC, PS_HTIC, PS_options, PS_init, PS_reset, 
	   PS_text, null_scale, PS_graphics, PS_move, PS_vector, 
	   PS_linetype, PS_put_text, PS_text_angle, 
	   PS_justify_text, PS_point, do_arrow}
#endif

#ifdef PRESCRIBE
    ,{"prescribe", "Prescribe - for the Kyocera Laser Printer",
	PRE_XMAX, PRE_YMAX, PRE_VCHAR, PRE_HCHAR, 
	PRE_VTIC, PRE_HTIC, options_null, PRE_init, PRE_reset, 
	PRE_text, null_scale, PRE_graphics, PRE_move, PRE_vector, 
	PRE_linetype, PRE_put_text, null_text_angle, 
	PRE_justify_text, line_and_point, do_arrow}
    ,{"kyo", "Kyocera Laser Printer with Courier font",
	PRE_XMAX, PRE_YMAX, KYO_VCHAR, KYO_HCHAR, 
	PRE_VTIC, PRE_HTIC, options_null, KYO_init, PRE_reset, 
	PRE_text, null_scale, PRE_graphics, PRE_move, PRE_vector, 
	PRE_linetype, PRE_put_text, null_text_angle, 
	PRE_justify_text, line_and_point, do_arrow}
#endif /* PRESCRIBE */

#ifdef PSLATEX
    ,{"pslatex", "LaTeX picture environment with PostScript \\specials",
	PS_XMAX, PS_YMAX, PSLATEX_VCHAR, PSLATEX_HCHAR,
	PS_VTIC, PS_HTIC, PSLATEX_options, PSLATEX_init, PSLATEX_reset,
	PSLATEX_text, PSLATEX_scale, PSLATEX_graphics, PS_move, PS_vector,
	PS_linetype, PSLATEX_put_text, PSLATEX_text_angle,
	PSLATEX_justify_text, PS_point, do_arrow}
#endif

#ifdef	PSTRICKS
    ,{"pstricks", "LaTeX picture environment with PSTricks macros",
	   PSTRICKS_XMAX, PSTRICKS_YMAX, PSTRICKS_VCHAR, PSTRICKS_HCHAR, 
	   PSTRICKS_VTIC, PSTRICKS_HTIC, options_null, PSTRICKS_init, PSTRICKS_reset, 
	   PSTRICKS_text, PSTRICKS_scale, PSTRICKS_graphics, PSTRICKS_move, PSTRICKS_vector, 
	   PSTRICKS_linetype, PSTRICKS_put_text, PSTRICKS_text_angle, 
	   PSTRICKS_justify_text, PSTRICKS_point, PSTRICKS_arrow}
#endif
    
#ifdef QMS
    ,{"qms", "QMS/QUIC Laser printer (also Talaris 1200 and others)",
	   QMS_XMAX,QMS_YMAX, QMS_VCHAR, QMS_HCHAR, 
	   QMS_VTIC, QMS_HTIC, options_null, QMS_init,QMS_reset, 
	   QMS_text, null_scale, QMS_graphics, QMS_move, QMS_vector,
	   QMS_linetype,QMS_put_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef REGIS
    ,{"regis", "REGIS graphics language",
	   REGISXMAX, REGISYMAX, REGISVCHAR, REGISHCHAR, 
	   REGISVTIC, REGISHTIC, REGISoptions, REGISinit, REGISreset, 
	   REGIStext, null_scale, REGISgraphics, REGISmove, REGISvector,
	   REGISlinetype, REGISput_text, REGIStext_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef RGIP
    ,{"rgip", "RGIP metafile (Uniplex). Option: fontsize (1-8)",
	   RGIP_XMAX, RGIP_YMAX, RGIP_VCHAR, RGIP_HCHAR,
	   RGIP_VTIC, RGIP_HTIC, RGIP_options, RGIP_init, RGIP_reset,
	   RGIP_text, null_scale, RGIP_graphics, RGIP_move,
	   RGIP_vector, RGIP_linetype, RGIP_put_text, RGIP_text_angle,
	   RGIP_justify_text, RGIP_do_point, do_arrow}
    ,{"uniplex", "RGIP metafile (Uniplex). Option: fontsize (1-8)",
	   RGIP_XMAX, RGIP_YMAX, RGIP_VCHAR, RGIP_HCHAR,
	   RGIP_VTIC, RGIP_HTIC, RGIP_options, RGIP_init, RGIP_reset,
	   RGIP_text, null_scale, RGIP_graphics, RGIP_move,
	   RGIP_vector, RGIP_linetype, RGIP_put_text, RGIP_text_angle,
	   RGIP_justify_text, RGIP_do_point, do_arrow}
#endif

#ifdef SELANAR
    ,{"selanar", "Selanar",
	   TEK40XMAX, TEK40YMAX, TEK40VCHAR, TEK40HCHAR, 
	   TEK40VTIC, TEK40HTIC, options_null, SEL_init, SEL_reset, 
	   SEL_text, null_scale, SEL_graphics, TEK40move, TEK40vector, 
	   TEK40linetype, TEK40put_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef STARC
    ,{"starc", "Star Color Printer",
	   STARCXMAX, STARCYMAX, STARCVCHAR, STARCHCHAR, 
	   STARCVTIC, STARCHTIC, options_null, STARCinit, STARCreset, 
	   STARCtext, null_scale, STARCgraphics, STARCmove, STARCvector, 
	   STARClinetype, STARCput_text, STARC_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef SUN
    ,{"sun", "SunView window system",
	   SUN_XMAX, SUN_YMAX, SUN_VCHAR, SUN_HCHAR, 
	   SUN_VTIC, SUN_HTIC, options_null, SUN_init, SUN_reset, 
	   SUN_text, null_scale, SUN_graphics, SUN_move, SUN_vector,
	   SUN_linetype, SUN_put_text, null_text_angle, 
	   SUN_justify_text, line_and_point, do_arrow}
#endif

#ifdef VWS
    ,{"VWS", "VAX Windowing System (UIS)",
           VWS_XMAX, VWS_YMAX, VWS_VCHAR, VWS_HCHAR,
           VWS_VTIC, VWS_HTIC, options_null, VWS_init, VWS_reset,
           VWS_text, null_scale, VWS_graphics, VWS_move, VWS_vector,
           VWS_linetype, VWS_put_text, VWS_text_angle,
           VWS_justify_text, do_point, do_arrow}
#endif

#ifdef TANDY60
    ,{"tandy_60dpi", "Tandy DMP-130 series 60-dot per inch graphics",
	   EPS60XMAX, EPS60YMAX, EPSONVCHAR, EPSONHCHAR,
	   EPSONVTIC, EPSONHTIC, options_null, EPSONinit, EPSONreset,
	   TANDY60text, null_scale, EPS60graphics, EPSONmove, EPSONvector,
	   EPSONlinetype, EPSONput_text, EPSON_text_angle,
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef T410X
    ,{"tek410x", "Tektronix 4106, 4107, 4109 and 420X terminals",
	   T410XXMAX, T410XYMAX, T410XVCHAR, T410XHCHAR, 
	   T410XVTIC, T410XHTIC, options_null, T410X_init, T410X_reset, 
	   T410X_text, null_scale, T410X_graphics, T410X_move, T410X_vector, 
	   T410X_linetype, T410X_put_text, T410X_text_angle, 
	   null_justify_text, T410X_point, do_arrow}
#endif

#ifdef TEK
    ,{"tek40xx", "Tektronix 4010 and others; most TEK emulators",
	   TEK40XMAX, TEK40YMAX, TEK40VCHAR, TEK40HCHAR, 
	   TEK40VTIC, TEK40HTIC, options_null, TEK40init, TEK40reset, 
	   TEK40text, null_scale, TEK40graphics, TEK40move, TEK40vector, 
	   TEK40linetype, TEK40put_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef TEXDRAW
    ,{"texdraw", "LaTeX texdraw environment",
	   TEXDRAW_XMAX, TEXDRAW_YMAX, TEXDRAW_VCHAR, TEXDRAW_HCHAR,
	TEXDRAW_VTIC, TEXDRAW_HTIC, options_null, TEXDRAW_init, TEXDRAW_reset,
	TEXDRAW_text, TEXDRAW_scale, TEXDRAW_graphics, TEXDRAW_move, TEXDRAW_vector,
	TEXDRAW_linetype, TEXDRAW_put_text, TEXDRAW_text_angle,
	TEXDRAW_justify_text, TEXDRAW_point, TEXDRAW_arrow}
#endif
  
#ifdef TGIF
    ,{"tgif", "TGIF X-Window draw tool (file version 10)",
	   TGIF_XMAX, TGIF_YMAX, TGIF_VCHAR1, TGIF_HCHAR1, 
	   TGIF_VTIC, TGIF_HTIC, options_null, TGIF_init, TGIF_reset, 
	   TGIF_text, null_scale, TGIF_graphics, TGIF_move, TGIF_vector, 
	   TGIF_linetype, TGIF_put_text, TGIF_text_angle, 
	   TGIF_justify_text, line_and_point, TGIF_arrow}
#endif

#ifdef TPIC
    ,{"tpic", "TPIC -- LaTeX picture environment with tpic \\specials",
	   TPIC_XMAX, TPIC_YMAX, TPIC_VCHAR, TPIC_HCHAR, 
	   TPIC_VTIC, TPIC_HTIC, TPIC_options, TPIC_init, TPIC_reset, 
	   TPIC_text, TPIC_scale, TPIC_graphics, TPIC_move, TPIC_vector, 
	   TPIC_linetype, TPIC_put_text, TPIC_text_angle, 
	   TPIC_justify_text, TPIC_point, TPIC_arrow}
#endif

#ifdef UNIXPLOT
#ifdef GNUGRAPH
    ,{"unixplot", "GNU plot(1) format [\042fontname\042 font_size]",
	   UP_XMAX, UP_YMAX, UP_VCHAR, UP_HCHAR,
	   UP_VTIC, UP_HTIC, UP_options, UP_init, UP_reset,
	   UP_text, null_scale, UP_graphics, UP_move, UP_vector,
	   UP_linetype, UP_put_text, UP_text_angle,
	   UP_justify_text, line_and_point, do_arrow}
#else
    ,{"unixplot", "Unix plotting standard (see plot(1))",
	   UP_XMAX, UP_YMAX, UP_VCHAR, UP_HCHAR, 
	   UP_VTIC, UP_HTIC, options_null, UP_init, UP_reset, 
	   UP_text, null_scale, UP_graphics, UP_move, UP_vector, 
	   UP_linetype, UP_put_text, null_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif /* GNUGRAPH */
#endif
	
#ifdef UNIXPC
    ,{"unixpc", "AT&T 3b1 or AT&T 7300 Unix PC",
	   uPC_XMAX, uPC_YMAX, uPC_VCHAR, uPC_HCHAR, 
	   uPC_VTIC, uPC_HTIC, options_null, uPC_init, uPC_reset, 
	   uPC_text, null_scale, uPC_graphics, uPC_move, uPC_vector,
	   uPC_linetype, uPC_put_text, uPC_text_angle, 
	   null_justify_text, line_and_point, do_arrow}
#endif

#ifdef EMXVGA
#ifdef EMXVESA
    ,{"vesa", "IBM PC/Clone with VESA SVGA graphics board [vesa mode]",
	   EMXVGA_XMAX, EMXVGA_YMAX, EMXVGA_VCHAR, EMXVGA_HCHAR,
	   EMXVGA_VTIC, EMXVGA_HTIC, EMXVESA_options, EMXVESA_init, EMXVESA_reset,
	   EMXVESA_text, null_scale, EMXVESA_graphics, EMXVGA_move, EMXVGA_vector,
	   EMXVGA_linetype, EMXVGA_put_text, EMXVGA_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif
    ,{"vgal", "IBM PC/Clone with VGA graphics board",
	   EMXVGA_XMAX, EMXVGA_YMAX, EMXVGA_VCHAR, EMXVGA_HCHAR,
	   EMXVGA_VTIC, EMXVGA_HTIC, options_null, EMXVGA_init, EMXVGA_reset,
	   EMXVGA_text, null_scale, EMXVGA_graphics, EMXVGA_move, EMXVGA_vector,
	   EMXVGA_linetype, EMXVGA_put_text, EMXVGA_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef V384
    ,{"vx384", "Vectrix 384 and Tandy color printer",
	   V384_XMAX, V384_YMAX, V384_VCHAR, V384_HCHAR, 
	   V384_VTIC, V384_HTIC, options_null, V384_init, V384_reset, 
	   V384_text, null_scale, V384_graphics, V384_move, V384_vector, 
	   V384_linetype, V384_put_text, null_text_angle, 
	   null_justify_text, do_point, do_arrow}
#endif

#ifdef VTTEK
    ,{"vttek", "VT-like tek40xx terminal emulator",
       TEK40XMAX,TEK40YMAX,TEK40VCHAR, TEK40HCHAR,
       TEK40VTIC, TEK40HTIC, options_null, VTTEK40init, VTTEK40reset,
       TEK40text, null_scale, TEK40graphics, TEK40move, TEK40vector,
       VTTEK40linetype, VTTEK40put_text, null_text_angle,
       null_justify_text, line_and_point, do_arrow}
#endif

#ifdef _Windows
    ,{"windows", "Microsoft Windows",
	   WIN_XMAX, WIN_YMAX, WIN_VCHAR, WIN_HCHAR, 
	   WIN_VTIC, WIN_HTIC, WIN_options, WIN_init, WIN_reset, 
	   WIN_text, WIN_scale, WIN_graphics, WIN_move, WIN_vector,
	   WIN_linetype, WIN_put_text, WIN_text_angle, 
	   WIN_justify_text, WIN_point, do_arrow}
#endif

#ifdef X11
    ,{"x11", "X11 Window System",
	   X11_XMAX, X11_YMAX, X11_VCHAR, X11_HCHAR, 
	   X11_VTIC, X11_HTIC, options_null, X11_init, X11_reset, 
	   X11_text, null_scale, X11_graphics, X11_move, X11_vector, 
	   X11_linetype, X11_put_text, null_text_angle, 
	   X11_justify_text, X11_point, do_arrow}
    ,{"X11", "X11 Window System (identical to x11)",
	   X11_XMAX, X11_YMAX, X11_VCHAR, X11_HCHAR, 
	   X11_VTIC, X11_HTIC, options_null, X11_init, X11_reset, 
	   X11_text, null_scale, X11_graphics, X11_move, X11_vector, 
	   X11_linetype, X11_put_text, null_text_angle, 
	   X11_justify_text, X11_point, do_arrow}
   ,{"xlib", "X11 Window System (gnulib_x11 dump)",
	   Xlib_XMAX, Xlib_YMAX, Xlib_VCHAR, Xlib_HCHAR, 
	   Xlib_VTIC, Xlib_HTIC, options_null, Xlib_init, Xlib_reset, 
	   Xlib_text, null_scale, Xlib_graphics, Xlib_move, Xlib_vector, 
	   Xlib_linetype, Xlib_put_text, null_text_angle, 
	   Xlib_justify_text, line_and_point, do_arrow}
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

    term = t;
    term_init = FALSE;
    name = term_tbl[term].name;

    /* Special handling for unixplot term type */
    if (!strncmp("unixplot",name,8)) {
	   UP_redirect (2);  /* Redirect actual stdout for unixplots */
    } else if (unixplot) {
	   UP_redirect (3);  /* Put stdout back together again. */
    }

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
   A default can be set with -DDEFAULTTERM=myterm in the Makefile
   or #define DEFAULTTERM myterm in term.h
*/
/* thanks to osupyr!alden (Dave Alden) for the original GNUTERM code */
init_terminal()
{
    int t;
#ifdef DEFAULTTERM
	char *term_name = DEFAULTTERM;
#else
    char *term_name = NULL;
#endif /* DEFAULTTERM */
#if (defined(__TURBOC__) && defined(MSDOS) && !defined(_Windows)) || defined(NEXT) || defined(SUN) || defined(X11)
    char *term = NULL;		/* from TERM environment var */
#endif
#ifdef X11
    char *display = NULL;
#endif
    char *gnuterm = NULL;

    /* GNUTERM environment variable is primary */
    gnuterm = getenv("GNUTERM");
    if (gnuterm != (char *)NULL) {
	 term_name = gnuterm;
#ifndef _Windows
#if defined(__TURBOC__) && defined(MSDOS)
         get_path();   /* So *_init() can find the BGI driver */
# endif
#endif
    }
    else {
#if defined(__TURBOC__) && defined(MSDOS) && !defined(_Windows)
	   term_name = turboc_init();
	   term = (char *)NULL; /* shut up turbo C */
#endif
	   
#ifdef __ZTC__
	  term_name = ztc_init();
#endif

#ifdef vms
	   term_name = vms_init();
#endif

#ifdef NEXT
	term = getenv("TERM");
	if (term_name == (char *)NULL
		&& term != (char *)NULL && strcmp(term,"next") == 0)
	      term_name = "next";
#endif /* NeXT */
	   
#ifdef SUN
	   term = getenv("TERM");	/* try $TERM */
	   if (term_name == (char *)NULL
		  && term != (char *)NULL && strcmp(term, "sun") == 0)
		term_name = "sun";
#endif /* sun */

#ifdef _Windows
		term_name = "win";
#endif /* _Windows */

#ifdef GPR
   if (gpr_isa_pad()) term_name = "gpr";       /* find out whether stdout is a DM pad. See term/gpr.trm */
#else
#ifdef APOLLO
   if (apollo_isa_pad()) term_name = "apollo"; /* find out whether stdout is a DM pad. See term/apollo.trm */
#endif /* APOLLO */
#endif /* GPR    */

#ifdef X11
	   term = getenv("TERM");	/* try $TERM */
	   if (term_name == (char *)NULL
		  && term != (char *)NULL && strcmp(term, "xterm") == 0)
		term_name = "x11";
	   display = getenv("DISPLAY");
	   if (term_name == (char *)NULL && display != (char *)NULL)
		term_name = "x11";
	   if (X11_Display)
		term_name = "x11";
#endif /* x11 */

#ifdef AMIGASCREEN
	   term_name = "amiga";
#endif

#ifdef ATARI
	   term_name = "atari";
#endif

#ifdef UNIXPC
           if (iswind() == 0) {
              term_name = "unixpc";
           }
#endif /* unixpc */

#ifdef CGI
	   if (getenv("CGIDISP") || getenv("CGIPRNT"))
	     term_name = "cgi";
#endif /*CGI */

#if defined(MSDOS) && defined(__EMX__)
#ifdef EMXVESA
	   term_name = "vesa";
#else
	   term_name = "vgal";
#endif
#endif

#ifdef DJGPP
		term_name = "svga";
#endif

#ifdef GRASS
        term_name = "grass";
#endif

#ifdef OS2
           term_name = "pm" ;
            /* EMX compiler has getcwd that can return drive */
           if( PM_path[0]=='\0' ) _getcwd2( PM_path, 256 ) ;
#endif /*OS2 */           
    }

    /* We have a name, try to set term type */
    if (term_name != NULL && *term_name != '\0') {
	   t = change_term(term_name, (int)strlen(term_name));
	   if (t == -1)
		fprintf(stderr, "Unknown terminal name '%s'\n", term_name);
	   else if (t == -2)
		fprintf(stderr, "Ambiguous terminal name '%s'\n", term_name);
	   else				/* successful */
		;
    }
}


#ifndef _Windows
#if defined (__TURBOC__) && defined (MSDOS)
char *
turboc_init()
{
  int g_driver,g_mode;
  char far *c1,*c2;
  char *term_name = NULL;
  struct text_info tinfo;       /* So we can restore starting text mode. */

/* Some of this code including BGI drivers is copyright Borland Intl. */
	g_driver=DETECT;
	      get_path();
	gettextinfo(&tinfo);
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
			case 8 : term_name = "att";
					 break;
			case 9 : term_name = "vgalib";
					 break;
            }
        closegraph();
        textmode(tinfo.currmode);
	clrscr();
	fprintf(stderr,"\tTC Graphics, driver %s  mode %s\n",c1,c2);
  return(term_name);
}
# endif /* __TURBOC__ */
#endif

#if defined(__ZTC__)
char *
ztc_init()
{
  int g_mode;
  char *term_name = NULL;

	g_mode = fg_init();

		  switch (g_mode){
			case FG_NULL	  :  fprintf(stderr,"Graphics card not detected or not supported.\n");
								 exit(1);
			case FG_HERCFULL  :  term_name = "hercules";
								 break;
			case FG_EGAMONO   :  term_name = "egamono";
								 break;
			case FG_EGAECD	  :  term_name = "egalib";
								 break;
			case FG_VGA11	  :  term_name = "vgamono";
								 break;
			case FG_VGA12	  :  term_name = "vgalib";
								 break;
			case FG_VESA6A	  :  term_name = "svgalib";
								 break;
			case FG_VESA5	  :  term_name = "ssvgalib";
								 break;
			}
		fg_term();
  return(term_name);
}
#endif /* __ZTC__ */


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
		/* closepl(); */  /* This is called by the term. */
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
	unsigned int xmax, ymax;
	char label[MAX_ID_LEN];
	int scaling;

	if (!term_init) {
	   (*t->init)();
	   term_init = TRUE;
	}
	screen_ok = FALSE;
	scaling = (*t->scale)(xsize, ysize);
	xmax = (unsigned int)(t->xmax * (scaling ? 1 : xsize));
	ymax = (unsigned int)(t->ymax * (scaling ? 1 : ysize));
	(*t->graphics)();
	/* border linetype */
	(*t->linetype)(-2);
	(*t->move)(0,0);
	(*t->vector)(xmax-1,0);
	(*t->vector)(xmax-1,ymax-1);
	(*t->vector)(0,ymax-1);
	(*t->vector)(0,0);
	(void) (*t->justify_text)(LEFT);
	(*t->put_text)(t->h_char*5,ymax-t->v_char*3,"Terminal Test");
	/* axis linetype */
	(*t->linetype)(-1);
	(*t->move)(xmax/2,0);
	(*t->vector)(xmax/2,ymax-1);
	(*t->move)(0,ymax/2);
	(*t->vector)(xmax-1,ymax/2);
	/* test width and height of characters */
	(*t->linetype)(-2);
	(*t->move)(  xmax/2-t->h_char*10,ymax/2+t->v_char/2);
	(*t->vector)(xmax/2+t->h_char*10,ymax/2+t->v_char/2);
	(*t->vector)(xmax/2+t->h_char*10,ymax/2-t->v_char/2);
	(*t->vector)(xmax/2-t->h_char*10,ymax/2-t->v_char/2);
	(*t->vector)(xmax/2-t->h_char*10,ymax/2+t->v_char/2);
	(*t->put_text)(xmax/2-t->h_char*10,ymax/2,
		"12345678901234567890");
	/* test justification */
	(void) (*t->justify_text)(LEFT);
	(*t->put_text)(xmax/2,ymax/2+t->v_char*6,"left justified");
	str = "centre+d text";
	if ((*t->justify_text)(CENTRE))
		(*t->put_text)(xmax/2,
				ymax/2+t->v_char*5,str);
	else
		(*t->put_text)(xmax/2-strlen(str)*t->h_char/2,
				ymax/2+t->v_char*5,str);
	str = "right justified";
	if ((*t->justify_text)(RIGHT))
		(*t->put_text)(xmax/2,
				ymax/2+t->v_char*4,str);
	else
		(*t->put_text)(xmax/2-strlen(str)*t->h_char,
				ymax/2+t->v_char*4,str);
	/* test text angle */
	str = "rotated ce+ntred text";
	if ((*t->text_angle)(1)) {
		if ((*t->justify_text)(CENTRE))
			(*t->put_text)(t->v_char,
				ymax/2,str);
		else
			(*t->put_text)(t->v_char,
				ymax/2-strlen(str)*t->h_char/2,str);
	}
	else {
	    (void) (*t->justify_text)(LEFT);
	    (*t->put_text)(t->h_char*2,ymax/2-t->v_char*2,"Can't rotate text");
	}
	(void) (*t->justify_text)(LEFT);
	(void) (*t->text_angle)(0);
	/* test tic size */
	(*t->move)(xmax/2+t->h_tic*2,0);
	(*t->vector)(xmax/2+t->h_tic*2,t->v_tic);
	(*t->move)(xmax/2,t->v_tic*2);
	(*t->vector)(xmax/2+t->h_tic,t->v_tic*2);
	(*t->put_text)(xmax/2+t->h_tic*2,t->v_tic*2+t->v_char/2,"test tics");
	/* test line and point types */
	x = xmax - t->h_char*4 - t->h_tic*4;
	y = ymax - t->v_char;
	for ( i = -2; y > t->v_char; i++ ) {
		(*t->linetype)(i);
		/*	(void) sprintf(label,"%d",i);  Jorgen Lippert
		lippert@risoe.dk */
		(void) sprintf(label,"%d",i+1);
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
	x = xmax/4;
	y = ymax/4;
	xl = t->h_tic*5;
	yl = t->v_tic*5;
	(*t->arrow)(x,y,x+xl,y,TRUE);
	(*t->arrow)(x,y,x+xl/2,y+yl,TRUE);
	(*t->arrow)(x,y,x,y+yl,TRUE);
	(*t->arrow)(x,y,x-xl/2,y+yl,FALSE);
	(*t->arrow)(x,y,x-xl,y,TRUE);
	(*t->arrow)(x,y,x-xl,y-yl,TRUE);
	(*t->arrow)(x,y,x,y-yl,TRUE);
	(*t->arrow)(x,y,x+xl,y-yl,TRUE);
	/* and back into text mode */
	(*t->text)();
}


#if defined(MSDOS)||defined(ATARI)||defined(OS2)||defined(_Windows)||defined(DOS386)
/* output for some terminal types must be binary to stop non Unix computers
   changing \n to \r\n. 
   If the output is not STDOUT, the following code reopens outfile 
   with binary mode. */
void
reopen_binary()
{
char filename[MAX_ID_LEN+1];

	if (outfile!=stdout) {
		(void) fclose(outfile);
		(void) strcpy(filename,outstr+1);	/* remove quotes */
		filename[strlen(filename)-1] = '\0';
#ifdef _Windows
		if ( !stricmp(outstr,"'PRN'") )
			(void) strcpy(filename,win_prntmp);	/* use temp file for windows */
#endif
		if ( (outfile = fopen(filename,"wb")) == (FILE *)NULL ) {
			if ( (outfile = fopen(filename,"w")) == (FILE *)NULL ) {
				os_error("cannot reopen file with binary type; output unknown",
					NO_CARET);
			} 
			else {
	os_error("cannot reopen file with binary type; output reset to ascii", 
					NO_CARET);
			}
		}
#if defined(__TURBOC__) && defined(MSDOS)
#ifndef _Windows
		if ( !stricmp(outstr,"'PRN'") )
		{
		/* Put the printer into binary mode. */
		union REGS regs;
			regs.h.ah = 0x44;	/* ioctl */
			regs.h.al = 0;		/* get device info */
			regs.x.bx = fileno(outfile);
			intdos(&regs, &regs);
			regs.h.dl |= 0x20;	/* binary (no ^Z intervention) */
			regs.h.dh = 0;
			regs.h.ah = 0x44;	/* ioctl */
			regs.h.al = 1;		/* set device info */
			intdos(&regs, &regs);
		}
#endif
#endif
	}
}
#endif

#ifdef vms
/* these are needed to modify terminal characteristics */
#include <descrip.h>
#include <iodef.h>
#include <ttdef.h>
#include <tt2def.h>
#include <dcdef.h>
#include <ssdef.h>
#include <stat.h>
#include <fab.h>
static unsigned short   chan;
static int  old_char_buf[3], cur_char_buf[3];
$DESCRIPTOR(sysoutput_desc,"SYS$OUTPUT");

char *vms_init()
/*
 * Determine if we have a regis terminal
 * and save terminal characteristics
*/
{
   /* Save terminal characteristics in old_char_buf and
   initialise cur_char_buf to current settings. */
   int i;
   sys$assign(&sysoutput_desc,&chan,0,0);
   sys$qiow(0,chan,IO$_SENSEMODE,0,0,0,old_char_buf,12,0,0,0,0);
   for (i = 0 ; i < 3 ; ++i) cur_char_buf[i] = old_char_buf[i];
   sys$dassgn(chan);

   /* Test if terminal is regis */
   if ((cur_char_buf[2] & TT2$M_REGIS) == TT2$M_REGIS) return("regis");
   return(NULL);
}

void
vms_reset()
/* set terminal to original state */
{
   int i;
   sys$assign(&sysoutput_desc,&chan,0,0);
   sys$qiow(0,chan,IO$_SETMODE,0,0,0,old_char_buf,12,0,0,0,0);
   for (i = 0 ; i < 3 ; ++i) cur_char_buf[i] = old_char_buf[i];
   sys$dassgn(chan);
}

void
term_mode_tek()
/* set terminal mode to tektronix */
{
   long status;
   if (outfile != stdout) return; /* don't modify if not stdout */
   sys$assign(&sysoutput_desc,&chan,0,0);
   cur_char_buf[0] = 0x004A0000 | DC$_TERM | (TT$_TEK401X<<8);
   cur_char_buf[1] = (cur_char_buf[1] & 0x00FFFFFF) | 0x18000000;

   cur_char_buf[1] &= ~TT$M_CRFILL;
   cur_char_buf[1] &= ~TT$M_ESCAPE;
   cur_char_buf[1] &= ~TT$M_HALFDUP;
   cur_char_buf[1] &= ~TT$M_LFFILL;
   cur_char_buf[1] &= ~TT$M_MECHFORM;
   cur_char_buf[1] &= ~TT$M_NOBRDCST;
   cur_char_buf[1] &= ~TT$M_NOECHO;
   cur_char_buf[1] &= ~TT$M_READSYNC;
   cur_char_buf[1] &= ~TT$M_REMOTE;
   cur_char_buf[1] |= TT$M_LOWER;
   cur_char_buf[1] |= TT$M_TTSYNC;
   cur_char_buf[1] |= TT$M_WRAP;
   cur_char_buf[1] &= ~TT$M_EIGHTBIT;
   cur_char_buf[1] &= ~TT$M_MECHTAB;
   cur_char_buf[1] &= ~TT$M_SCOPE;
   cur_char_buf[1] |= TT$M_HOSTSYNC;

   cur_char_buf[2] &= ~TT2$M_APP_KEYPAD;
   cur_char_buf[2] &= ~TT2$M_BLOCK;
   cur_char_buf[2] &= ~TT2$M_DECCRT3;
   cur_char_buf[2] &= ~TT2$M_LOCALECHO;
   cur_char_buf[2] &= ~TT2$M_PASTHRU;
   cur_char_buf[2] &= ~TT2$M_REGIS;
   cur_char_buf[2] &= ~TT2$M_SIXEL;
   cur_char_buf[2] |= TT2$M_BRDCSTMBX;
   cur_char_buf[2] |= TT2$M_EDITING;
   cur_char_buf[2] |= TT2$M_INSERT;
   cur_char_buf[2] |= TT2$M_PRINTER;
   cur_char_buf[2] &= ~TT2$M_ANSICRT;
   cur_char_buf[2] &= ~TT2$M_AVO;
   cur_char_buf[2] &= ~TT2$M_DECCRT;
   cur_char_buf[2] &= ~TT2$M_DECCRT2;
   cur_char_buf[2] &= ~TT2$M_DRCS;
   cur_char_buf[2] &= ~TT2$M_EDIT;
   cur_char_buf[2] |= TT2$M_FALLBACK;

   status = sys$qiow(0,chan,IO$_SETMODE,0,0,0,cur_char_buf,12,0,0,0,0);
   if (status == SS$_BADPARAM) {
      /* terminal fallback utility not installed on system */
      cur_char_buf[2] &= ~TT2$M_FALLBACK;
      sys$qiow(0,chan,IO$_SETMODE,0,0,0,cur_char_buf,12,0,0,0,0);
   }
   else {
      if (status != SS$_NORMAL)
         lib$signal(status,0,0);
   }
   sys$dassgn(chan);
}

void
term_mode_native()
/* set terminal mode back to native */
{
   int i;
   if (outfile != stdout) return; /* don't modify if not stdout */
   sys$assign(&sysoutput_desc,&chan,0,0);
   sys$qiow(0,chan,IO$_SETMODE,0,0,0,old_char_buf,12,0,0,0,0);
   for (i = 0 ; i < 3 ; ++i) cur_char_buf[i] = old_char_buf[i];
   sys$dassgn(chan);
}

void
term_pasthru()
/* set terminal mode pasthru */
{
   if (outfile != stdout) return; /* don't modify if not stdout */
   sys$assign(&sysoutput_desc,&chan,0,0);
   cur_char_buf[2] |= TT2$M_PASTHRU;
   sys$qiow(0,chan,IO$_SETMODE,0,0,0,cur_char_buf,12,0,0,0,0);
   sys$dassgn(chan);
}

void
term_nopasthru()
/* set terminal mode nopasthru */
{
   if (outfile != stdout) return; /* don't modify if not stdout */
   sys$assign(&sysoutput_desc,&chan,0,0);
   cur_char_buf[2] &= ~TT2$M_PASTHRU;
   sys$qiow(0,chan,IO$_SETMODE,0,0,0,cur_char_buf,12,0,0,0,0);
   sys$dassgn(chan);
}

void
reopen_binary()
/* close the file outfile outfile and reopen it with binary type
   if not already done or outfile == stdout */
{
   stat_t stat_buf;
   char filename[MAX_ID_LEN+1];
   if (outfile != stdout) { /* don't modify if not stdout */
      if (!fstat(fileno(outfile),&stat_buf)) {
         if (stat_buf.st_fab_rfm != FAB$C_FIX) {
            /* modify only if not already done */
            (void) fclose(outfile);
            (void) strcpy(filename,outstr+1);   /* remove quotes */
            filename[strlen(filename)-1] = '\0';
            (void) delete(filename);
            if ((outfile = fopen(filename,"wb","rfm=fix","bls=512","mrs=512"))
                == (FILE *)NULL ) {
               if ( (outfile = fopen(filename,"w")) == (FILE *)NULL ) {
                 os_error("cannot reopen file with binary type; output unknown",
                           NO_CARET);
               }
               else {
          os_error("cannot reopen file with binary type; output reset to ascii",
                           NO_CARET);
               }
            }
         }
      }
      else{
         os_error("cannot reopen file with binary type; output remains ascii",
                  NO_CARET);
      }
   }
}

void
fflush_binary()
{
   typedef short int INT16;     /* signed 16-bit integers */
   register INT16 k;            /* loop index */
   if (outfile != stdout) {
       /* Stupid VMS fflush() raises error and loses last data block
          unless it is full for a fixed-length record binary file.
          Pad it here with NULL characters. */
       for (k = (INT16)((*outfile)->_cnt); k > 0; --k)
          putc('\0',outfile);
       fflush(outfile);
   }
}
#endif
