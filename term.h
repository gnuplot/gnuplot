
/*
 * $Id: term.h%v 3.50 1993/07/09 05:35:24 woo Exp $
 *
 */

/* GNUPLOT - term.h */
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

/*
 * term.h: terminal support definitions
 *   Edit this file depending on the set of terminals you wish to support.
 * Comment out the terminal types that you don't want or don't have, and
 * uncomment those that you want included. Be aware that some terminal 
 * types (eg, SUN, UNIXPLOT) will require changes in the makefile 
 * LIBS definition. 
 */

/* These terminals are not relevant for MSDOS, OS2, MS-Windows, ATARI or Amiga */
#if !defined(MSDOS) && !defined(OS2) && !defined(_Windows) && !defined(ATARI) && !defined(AMIGA_SC_6_1) && !defined(AMIGA_AC_5)

#define AED		/* AED 512 and AED 767 */
#define BITGRAPH	/* BBN BitGraph */
#define COREL           /* CorelDRAW! eps format */
/* #define CGI		/* SCO CGI */
/* #define IRIS4D	/* IRIS4D series computer */
#define KERMIT		/* MS-DOS Kermit Tektronix 4010 emulator */
/* #define FIG 	  	/* Fig graphics language */
/* #define NEXT		/* NeXT workstation console */
/* #define SUN		/* Sun Microsystems Workstation */
#define REGIS		/* ReGis graphics (vt125, vt220, vt240, Gigis...) */
/* #define RGIP		/* WARNING: requires POSIX: Redwood Graphics Interface Protocol UNIPLEX */
#define SELANAR		/* Selanar */
#define T410X		/* Tektronix 4106, 4107, 4109 and 420x terminals */
#define TEK		/* Tektronix 4010, and probably others */
/* #define UNIXPC	/* unixpc (ATT 3b1 or ATT 7300) */
/* #define UNIXPLOT	/* unixplot */
#define VTTEK		/* VT-like tek40xx emulators */
/* #define X11		/* X11R4 window system */

#define DXY800A		/* Roland DXY800A plotter */
#define EXCL		/* QMS/EXCL laserprinter (Talaris 1590 and others) */

#define HP2648		/* HP2648, HP2647 */
#define HP26		/* HP2623A and maybe others */
/* #define DEBUG		/* debugging terminal */
#define HP75		/* HP7580, and probably other HPs */
#define IMAGEN  	/* Imagen laser printers (300dpi) (requires -Iterm also) */

#define PRESCRIBE	/* Kyocera Laser printer */
#define QMS		/* QMS/QUIC laserprinter (Talaris 1200 and others) */

#define TANDY60		/* Tandy DMP-130 series 60-dot per inch graphics */
#define V384		/* Vectrix 384 and tandy color printer */

#define TGIF		/* TGIF X-Windows draw tool */

#endif /* !MSDOS && !OS2 && !_Windows && !_ATARI && !AMIGA */

/* These terminals can be used on any system */
#define AIFM		/* Adobe Illustrator Format */
#define DUMB


#define DXF		/* DXF format for use with AutoCad (Release 10.x) */

#define EEPIC		/* EEPIC-extended LaTeX driver, for EEPIC users */
#define EMTEX		/* LATEX picture environment with EMTEX specials */
#define EPS180		/* Epson-style 180-dot per inch (24 pin) printers */
#define EPS60		/* Epson-style 60-dot per inch printers */
#define EPSONP		/* Epson LX-800, Star NL-10, NX-1000 and lots of others */
/* #define FIG 	  	/* Fig graphics language */
#define GPIC		/* gpic for groff */
/* #define GRASS	/* GRASS (geographic info system) monitor */
#define HP500C		/* HP DeskJet 500 Color */
#define HPGL		/* HP7475, HP7220 plotters, and (hopefully) lots of others */
#define HPLJII		/* HP LaserJet II */
#define HPPJ		/* HP PaintJet */
#define LATEX		/* LATEX picture environment */
#define MF			/* METAFONT driver */
#define MIF			/* Frame Maker MIF 3.00 format driver */
#define NEC			/* NEC CP6 pinwriter  and LQ-800 printer */
#define OKIDATA		/* OKIDATA  320/321 standard 60-dpi printers */
#define PBM			/* PBMPLUS portable bitmap */
#define PCL			/* orignal HP LaserJet III */
#define POSTSCRIPT	/* PostScript */
#define PSLATEX		/* LaTeX picture environment with PostScript \specials */
#define PSTRICKS	/* LaTeX picture environment with PSTricks macros */
#define STARC		/* Star Color Printer */
#define TEXDRAW         /* TeXDraw drawing package for LaTeX */
#define TPIC		/* TPIC specials for TeX */

/* These are for Amiga only */
#if defined(AMIGA_SC_6_1) || defined(AMIGA_AC_5)
#define AMIGASCREEN	/* Amiga custom screen */
#undef  AIFM
#undef  DXF
#undef  FIG
#undef  MIF
#endif

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

#ifdef OS2
#define OS2PM
#endif /*OS2 */

#ifdef GISBASE
#define GRASS
#endif
