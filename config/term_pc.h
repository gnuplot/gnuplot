/*
 * $Id: term_pc.h,v 1.7 2008/03/30 03:27:51 sfeam Exp $
 *
 */

/* GNUPLOT - term.h */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
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

/*
 * term.h: terminal support definitions
 *   Edit this file depending on the set of terminals you wish to support.
 * Comment out the terminal types that you don't want or don't have, and
 * uncomment those that you want included. Be aware that some terminal
 * types (eg, SUN, UNIXPLOT) will require changes in the makefile
 * LIBS definition.
 */

/*
 * first draft after all terminals are converted to new layout
 * Stefan Bodewig Dec. 1995
 */

/* This version of term.h is pre-crippled to easily support 16bit
 * PC (DOS/Win) compilers that can't cope with many terminals */

/* Define SHORT_TERMLIST to select a few terminals. It is easier
 * to define the macro and list desired terminals in this section.
 * Sample configuration for a Unix workstation
 */
#ifdef SHORT_TERMLIST
# include "dumb.trm"		/* dumb terminal */
# include "post.trm"		/* postscript */
# include "regis.trm"		/* regis graphics */
# include "tek.trm"		/* a Tek 4010 and others including VT-style */
# ifdef X11
#  include "x11.trm"		/* x Window system */
# endif				/* X11 */
#else /* include all applicable terminals not commented out */

/****************************************************************************/
/* Platform dependent part                                                  */
/****************************************************************************/

/* Apple Macintosh */
#ifdef _Macintosh
# include "mac.trm"
#endif


/* BeOS */
#ifdef __BEOS__
# include "be.trm"
#endif


/****************************************************************************/
/* MS-DOS and Windows */
#if defined(MSDOS) || defined(_Windows)

/* MSDOS with emx-gcc compiler */
# if defined(MSDOS) && defined(__EMX__)
   /* Vesa-Cards */
#  define EMXVESA
#  include "emxvga.trm"
# endif				/* MSDOS && EMX */

/* MSDOS with djgpp compiler */
# ifdef DJGPP
#  include "djsvga.trm"
# endif

/* MSDOS with Zortech-C++ Compiler */
# ifdef __ZTC__
#  include "fg.trm"
# endif

/* All other Compilers */
# ifndef _Windows
#  ifdef PC
/* uncomment the next line to include SuperVGA support */
#   define BGI_NAME "svga256"	/* the name of the SVGA.BGI for Borland C */
/* this also triggers the inclusion of Super VGA support */
#   include "pc.trm"		/* all PC types except MS WINDOWS */
#  endif
# else				/* _Windows */
#  include "win.trm"		/* MS-Windows */
# endif				/* _Windows */
#endif /* MSDOS || _Windows */
/****************************************************************************/


/* NeXT */
#ifdef NEXT
# include "next.trm"
#endif


/* OS/2 */
#ifdef OS2
/* presentation manager */
# include "pm.trm"
# ifdef EMXVESA
/* works with DOS and OS/2 (windowed/full screen) */
#  include "emxvga.trm"
# endif
#endif /* OS2 */


/***************************************************************************/
/* Terminals for various Unix platforms                                    */
/***************************************************************************/

/* Apollo Graphics Primitive Resource */
#ifdef APOLLO
/* with resizeable windows */
# include "apollo.trm"
#  ifdef GPR
/* with fixed windows */
#   include "gpr.h"
#  endif
#endif /* Apollo */


/* Linux VGA */
#ifdef LINUXVGA
# include "linux.trm"
#endif


/* MGR Window system */
#ifdef MGR
# include "mgr.trm"
#endif

/* Redwood Graphics Interface Protocol UNIPLEX */
/* Metafile, requires POSIX */
#ifdef RGIP
# include "rgip.trm"
#endif


/* SCO CGI drivers */
#ifdef SCO
# include "cgi.trm"
#endif

/* SunView */
#ifdef SUN
# include "sun.trm"
#endif


/* VAX Windowing System requires UIS libraries */
#ifdef UIS
# include "vws.trm"
#endif

/* AT&T Unix-PC */
#ifdef UNIXPC
# include "unixpc.trm"
#endif

/****************************************************************************/
/* Terminals not relevant for MSDOS, OS2, MS-Windows       */
#if !(defined(MSDOS) || defined(OS2) || defined(_Windows))

/* AED 512 and AED 767 graphics terminals */
# include "aed.trm"

# if defined(UNIXPLOT) || defined(GNUGRAPH)
#  ifdef GNUGRAPH
#   include "gnugraph.trm"
#  else
#   include "unixplot.trm"
#  endif			/* !GNUGRAPH */
# endif				/* UNIXPLOT || GNUGRAPH */

/* gpic for groff */
# include "gpic.trm"

/* REGIS graphics language */
# include "regis.trm"

/* Tektronix 4106, 4107, 4109 and 420x terminals */
# include "t410x.trm"

/* a Tek 4010 and others including VT-style */
# include "tek.trm"

/* inboard terminal driver for X11 (dumps gnuplot_x11 commands) */
# include "xlib.trm"

#endif /* !(MSDOS || OS2 || _Windows) */
/****************************************************************************/


/****************************************************************************/
/* These terminals can be used on any system */

#ifdef X11
# include "x11.trm"		/* X Window System */
#endif

/* Adobe Illustrator Format */
/* #include "ai.trm" */

/* Computer Graphics Metafile (eg ms office) */
#include "cgm.trm"

/* CorelDraw! eps format */
/* #include "corel.trm" */

/* debugging terminal */
#ifdef DEBUG
# include "debug.trm"
#endif

/* dumb terminal */
/* #include "dumb.trm" */

/* DXF format for use with AutoCad (Release 10.x) */
/* #include "dxf.trm" */

/* Roland DXY800A plotter */
/* #include "dxy.trm" */

/* QMS/EXCL laserprinter (Talaris 1590 and others) */
/* #include "excl.trm" */

/* fig graphics */
/* #include "fig.trm" */

/* NOTE THAT GIF REQUIRES A SEPARATE LIBRARY : see term/gif.trm */
/* GIF format. */
#ifdef HAVE_LIBGD
# include "gif.trm"
#endif

/* geographical info system */
/* #include "grass.trm" */

/* HP2623A and probably others */
/* #include "hp26.trm" */

/* HP2647 and 2648 */
/* #include "hp2648.trm" */

/* HP DeskJet 500 C */
/* #include "hp500c.trm" */

/* HP7475, HP7220 plotters, and (hopefully) lots of others */
#include "hpgl.trm"

/* HP Laserjet II */
/* #include "hpljii.trm" */

/* HP PrintJet */
/* #include "hppj.trm" */

/* Imagen laser printers */
/* #include "imagen.trm" */

/* Kyocera Prescribe printer */
/* #include "kyo.trm" */

/* Frame Maker MIF 3.00 format driver */
/* #include "mif.trm" */


/* portable bit map */
#include "pbm.trm"

/* NOTE THAT PNG REQUIRES A SEPARATE LIBRARY : see term/png.trm */
#ifdef HAVE_LIBPNG
# include "png.trm"
#endif

/* postscript */
#include "post.trm"

/* QMS laser printers */
/* #include "qms.trm" */

/* x11 tgif tool */
/* #include "tgif.trm" */

/* tcl/tk */
/* #include "tkcanvas.trm" */

/* Vectrix 384 printer, also Tandy colour */
/* #include "v384.trm" */

/* wire printers */
#define EPSONP			/* Epson LX-800, Star NL-10, NX-1000 and lots of others */
#define EPS60			/* Epson-style 60-dot per inch printers */
#define EPS180			/* Epson-style 180-dot per inch (24 pin) printers */
#define NEC
#define OKIDATA
#define STARC
#define TANDY60			/* Tandy DMP-130 series 60-dot per inch graphics */
/* the common driver file for all of these */
/* #include "epson.trm" */

/* TeX related terminals */
/* latex and emtex */
/*#define EMTEX*/
/*#define EEPIC*/
/* #include "latex.trm" */

/* latex/tex with picture in postscript */
/* #include "pslatex.trm" */

/* EEPIC-extended LaTeX driver, for EEPIC users */
/* #include "eepic.trm" */

/* TPIC specials for TeX */
/* #include "tpic.trm" */

/* LaTeX picture environment with PSTricks macros */
/* #include "pstricks.trm" */

/* TeXDraw drawing package for LaTeX */
/* #include "texdraw.trm" */

/* METAFONT */
/* #include "metafont.trm" */

#endif /* !SHORT_TERMLIST */
