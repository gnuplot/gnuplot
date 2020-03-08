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
 * types will require changes in the makefile LIBS definition.
 */

/*
 * first draft after all terminals are converted to new layout
 * Stefan Bodewig Dec. 1995
 */

/*
 * >>> CONFIGURATION OPTIONS FOLLOW <<<  PLEASE READ
 *
 * pslatex and epslatex support is now provided by the combination of
 * post.trm and pslatex.trm.  You cannot build pslatex without post.
 * Both drivers are selected by default, but you can disable them below.
 *
 * Enhanced text support is pretty much required for all terminals now.
 * If you build without GP_ENH_EST text layout will be degraded.
 */
#define GP_ENH_EST 1		/* estimate string length of enhanced text */
#define POSTSCRIPT_DRIVER 1	/* include post.trm */
#define PSLATEX_DRIVER 1	/* include pslatex.trm */

#if defined(PSLATEX_DRIVER) && !defined(POSTSCRIPT_DRIVER)
#define POSTSCRIPT_DRIVER
#endif

# ifdef GP_ENH_EST
#  include "estimate.trm"	/* used for enhanced text processing */
# endif

/* Unicode escape sequences (\U+hhhh) are handling by the enhanced text code.
 * Various terminals check every string to see whether it needs enhanced text
 * processing. This macro allows them to include a check for the presence of
 * unicode escapes.
 */
#define contains_unicode(S) strstr(S, "\\U+")

/* Define SHORT_TERMLIST to select a few terminals. It is easier
 * to define the macro and list desired terminals in this section.
 * Sample configuration for a Unix workstation
 */
#ifdef SHORT_TERMLIST
# include "dumb.trm"		/* dumb terminal */

# ifdef POSTSCRIPT_DRIVER
#  ifdef  PSLATEX_DRIVER
#   undef PSLATEX_DRIVER
#  endif
#  include "post.trm"		/* postscript */
# endif

# ifdef X11
#  include "x11.trm"		/* X Window system */
# endif				/* X11 */
# ifdef OS2
#  include "pm.trm"		/* OS/2 Presentation Manager */
# endif
# ifdef _WIN32
#  include "win.trm"		/* MS-Windows */
# endif
#else /* include all applicable terminals not commented out */

/****************************************************************************/
/* Platform dependent part                                                  */
/****************************************************************************/

/* BeOS */
#ifdef __BEOS__
# include "be.trm"
#endif

/* MSDOS with djgpp compiler or _WIN32/Mingw or X11 */
#if (defined(DJGPP) && (!defined(DJSVGA) || (DJSVGA != 0))) || defined(HAVE_GRX)
# include "djsvga.trm"
#endif

/****************************************************************************/
/* MS-DOS */
#if defined(MSDOS)

/* MSDOS with emx-gcc compiler */
# if defined(__EMX__)
   /* Vesa-Cards */
#  define EMXVESA
#  include "emxvga.trm"
# endif				/* MSDOS && EMX */

/* MSDOS with OpenWatcom compiler */
# if defined(__WATCOMC__)
#  include "pc.trm"
# endif

#endif /* MSDOS */
/****************************************************************************/

/* Windows */
#ifdef _WIN32
/* Windows GDI/GDI+/Direct2D */
# include "win.trm"
#endif

/* Apple Mac OS X */
#ifdef HAVE_FRAMEWORK_AQUATERM
/* support for AquaTerm.app */
# include "aquaterm.trm"
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

/* VAX Windowing System requires UIS libraries */
#ifdef UIS
# include "vws.trm"
#endif

/****************************************************************************/
/* Terminals not relevant for MSDOS, MS-Windows */
#if !(defined(MSDOS) || defined(_WIN32))

/* This is not really a terminal, it generates a help section for */
/* using the linux console. */
# include "linux-vgagl.trm"

/* gpic for groff */
#ifdef HAVE_GPIC
# include "gpic.trm"
#endif

/* REGIS graphics language */
#if defined(HAVE_REGIS)
# include "regis.trm"
#endif

#ifdef WITH_TEKTRONIX
    /* Tektronix 4106, 4107, 4109 and 420x terminals */
#   include "t410x.trm"

    /* a Tek 4010 and others including VT-style */
#   include "tek.trm"
#endif


#endif /* !MSDOS && !_WIN32 */
/****************************************************************************/


/****************************************************************************/
/* These terminals can be used on any system */

#ifdef X11
# include "x11.trm"		/* X Window System */
# include "xlib.trm"		/* dumps x11 commands to gpoutfile */
#endif

/* Adobe Illustrator Format */
/* obsolete: use 'set term postscript level1 */
/* #include "ai.trm" */

/* HTML Canvas terminal */
#if (defined(HAVE_GD_PNG) || defined(HAVE_CAIROPDF))
# include "write_png_image.c"
#endif
#include "canvas.trm"

/* Computer Graphics Metafile (eg ms office) */
#include "cgm.trm"

/* CorelDraw! eps format */
/* #include "corel.trm"  */

/* debugging terminal */
#ifdef DEBUG
# include "debug.trm"
#endif

/* dumb terminal */
#include "dumb.trm"

/* caca: color ascii art terminal using libcaca */
#ifdef HAVE_LIBCACA
# include "caca.trm"
#endif

/* Legacy terminal for export to AutoCad (Release 10.x)
 * DWGR10 format (1988)
 * Still included by popular demand although basically untouched for 20+ years.
 * Someone please update this terminal to adhere to a newer DXF standard!
 * http://images.autodesk.com/adsk/files/autocad_2012_pdf_dxf-reference_enu.pdf
 */
#include "dxf.trm"

/* Enhanced Metafile Format driver */
#include "emf.trm"

/* Roland DXY800A plotter */
/* #include "dxy.trm" */
/* QMS/EXCL laserprinter (Talaris 1590 and others) */
/* #include "excl.trm" */

/* fig graphics */
#include "fig.trm"

/* geographical info system */
/* #include "grass.trm" */

/* HP2623A "ET head" 1980 era graphics terminal */
/* #include "hp26.trm" */

/* HP2647 and 2648 */
/* #include "hp2648.trm" */

/* HP7475, HP7220 plotters, and (hopefully) lots of others */
#include "hpgl.trm"

#ifndef NO_BITMAP_SUPPORT
/* HP DeskJet 500 C */
#include "hp500c.trm"

/* HP Laserjet II */
#include "hpljii.trm"

/* HP PrintJet */
#include "hppj.trm"

#endif /* NO_BITMAP_SUPPORT */

/* Imagen laser printers */
/* #include "imagen.trm" */

/* Kyocera Prescribe printer */
/* #include "kyo.trm" */

/* Frame Maker MIF 3.00 format driver */
#ifdef HAVE_MIF
#include "mif.trm"
#endif

/* DEPRECATED since 5.0.6
 * PDF terminal based on non-free library PDFlib or PDFlib-lite from GmbH.
 */
/* # include "pdf.trm" */

#if defined(HAVE_GD_PNG) || defined(HAVE_GD_JPEG) || defined(HAVE_GD_GIF)
# include "gd.trm"
#endif

/* postscript */
#ifdef POSTSCRIPT_DRIVER
#include "post.trm"
#endif

/* QMS laser printers */
/* #include "qms.trm" */

/* W3C Scalable Vector Graphics file */
#include "svg.trm"

/* x11 tgif tool */
#ifdef HAVE_TGIF
#include "tgif.trm"
#endif

/* tcl/tk with perl extensions */
#include "tkcanvas.trm"

#ifndef NO_BITMAP_SUPPORT

/* portable bit map */
#include "pbm.trm"

/* wire printers */
/* Epson LX-800, Star NL-10, NX-1000 and lots of others */
#define EPSONP

/* Epson-style 60-dot per inch printers */
#define EPS60

/* Epson-style 180-dot per inch (24 pin) printers */
#define EPS180

#define NEC
#define OKIDATA
#define STARC

/* Seiko DPU-414 thermal printer */
#define DPU414

/* Tandy DMP-130 series 60-dot per inch graphics */
#define TANDY60

/* the common driver file for all of these */
#include "epson.trm"

#endif /* NO_BITMAP_SUPPORT */

/* TeX related terminals start here */


/* LaTeX2e picture environment */
#include "pict2e.trm"

/* latex/tex with picture in postscript */
#ifdef PSLATEX_DRIVER
#include "pslatex.trm"
#endif

/* LaTeX picture environment with PSTricks macros */
#include "pstricks.trm"

/* TeXDraw drawing package for LaTeX */
#include "texdraw.trm"

/* METAFONT */
#include "metafont.trm"

/* METAPOST */
#include "metapost.trm"

/* ConTeXt */
#include "context.trm"

/*
 * DEPRECATED latex terminals no longer built by default
 */
#if 0
#  define EMTEX
#  define EEPIC
#  define OLD_LATEX_TERMINAL

#  include "latex.trm"	/* latex and emtex */
#  include "eepic.trm"	/* EEPIC-extended LaTeX driver */
#  include "tpic.trm"	/* TPIC specials for TeX */
#else
#  include "latex_old.h" /* deprecation notice for docs */
#  undef OLD_LATEX_TERMINAL
#endif

/* End of TeX related terminals */


#ifdef USE_GGI_DRIVER
# include "ggi.trm"
#endif

/* WXWIDGETS */
#ifdef WXWIDGETS
# include "wxt.trm"
#endif

#ifdef HAVE_CAIROPDF
# include "cairo.trm"
#endif

#ifdef HAVE_LUA
#include "lua.trm"
#endif

#ifdef QTTERM
# include "qt.trm"
#endif

#endif /* !SHORT_TERMLIST */
