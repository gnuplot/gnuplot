/* GNUPLOT - term.h */
/*
 * Copyright (C) 1986, 1987, 1990, 1991   Thomas Williams, Colin Kelley
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
 * Send your comments or suggestions to 
 *  pixar!info-gnuplot@sun.com.
 * This is a mailing list; to join it send a note to 
 *  pixar!info-gnuplot-request@sun.com.  
 * Send bug reports to
 *  pixar!bug-gnuplot@sun.com.
 */

/*
 * term.h: terminal support definitions
 *   Edit this file depending on the set of terminals you wish to support.
 * Comment out the terminal types that you don't want or don't have, and
 * uncomment those that you want included. Be aware that some terminal 
 * types (eg, SUN, UNIXPLOT) will require changes in the makefile 
 * LIBS definition. 
 */

/* These terminals are not relevant for MSDOS */
#ifndef MSDOS

#ifdef AMIGA_LC_5_1
#define AMIGASCREEN	/* Amiga custom screen */

#else /* AMIGA_LC_5_1 */

#ifdef AMIGA_AC_5
#define AMIGASCREEN	/* Amiga custom screen */
#endif
#define AED		/* AED 512 and AED 767 */
#define BITGRAPH	/* BBN BitGraph */
/* #define CGI		/* SCO CGI */
/* #define IRIS4D	/* IRIS4D series computer */
#define KERMIT		/* MS-Kermit Tektronix 4010 emulator */
#define FIG 	  	/* Fig graphics language */
#define REGIS		/* ReGis graphics (vt125, vt220, vt240, Gigis...) */
#define SELANAR		/* Selanar */
/* #define SUN		/* Sun Microsystems Workstation */
#define T410X		/* Tektronix 4106, 4107, 4109 and 420x terminals */
#define TEK		/* Tektronix 4010, and probably others */
/* #define UNIXPC	/* unixpc (ATT 3b1 or ATT 7300) */
/* #define UNIXPLOT	/* unixplot */
#define VTTEK		/* VT-like tek40xx emulators */
/* #define X11		/* X11R4 window system */

#endif /* AMIGA_LC_5_1 */

#define DXY800A		/* Roland DXY800A plotter */

#define HP2648		/* HP2648, HP2647 */
#define HP26		/* HP2623A and maybe others */
#define HP75		/* HP7580, and probably other HPs */
#define IMAGEN  	/* Imagen laser printers (300dpi) (requires -Iterm also) */

#define NEC		/* NEC CP6 pinwriter printer */
#define PRESCRIBE	/* Kyocera Laser printer */
#define QMS		/* QMS/QUIC laserprinter (Talaris 1200 and others) */
#define STARC		/* Star Color Printer */
#define TANDY60		/* Tandy DMP-130 series 60-dot per inch graphics */
#define V384		/* Vectrix 384 and tandy color printer */

#endif /* MSDOS */

/* These terminals can be used on any system */
#define DUMB

#define HPGL		/* HP7475, HP7220 plotters, and (hopefully) lots of others */

#define POSTSCRIPT	/* Postscript */

/* #define DXF		/* DXF format for use with AutoCad (Release 10.x) */

#define EEPIC		/* EEPIC-extended LaTeX driver, for EEPIC users */
#define EMTEX		/* LATEX picture environment with EMTEX specials */
#define EPS60		/* Epson-style 60-dot per inch printers */
#define EPSONP		/* Epson LX-800, Star NL-10, NX-1000 and lots of others */
#define HPLJII		/* HP LaserJet II */
#define LATEX		/* LATEX picture environment */

/* These are for MSDOS only */
#ifdef MSDOS
#ifdef __TURBOC__
#define ATT6300		/* AT&T 6300 graphics */
#else
#define ATT6300		/* AT&T 6300 graphics */
#define CORONA		/* Corona graphics 325 */
#define HERCULES	/* IBM PC/Clone with Hercules graphics board */
#endif /* __TURBOC__ */
#endif /* MSDOS */

