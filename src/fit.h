/*
 * $Id: fit.h,v 1.5.2.1 2000/05/03 21:26:11 joze Exp $
 */

/* GNUPLOT - fit.h */

/*  NOTICE: Change of Copyright Status
 *
 *  The author of this module, Carsten Grammes, has expressed in
 *  personal email that he has no more interest in this code, and
 *  doesn't claim any copyright. He has agreed to put this module
 *  into the public domain.
 *
 *  Lars Hecking  15-02-1999
 */

/*
 *	Header file: public functions in fit.c
 *
 *
 *	Copyright of this module:   Carsten Grammes, 1993
 *      Experimental Physics, University of Saarbruecken, Germany
 *
 *	Internet address: cagr@rz.uni-sb.de
 *
 *	Permission to use, copy, and distribute this software and its
 *	documentation for any purpose with or without fee is hereby granted,
 *	provided that the above copyright notice appear in all copies and
 *	that both that copyright notice and this permission notice appear
 *	in supporting documentation.
 *
 *      This software is provided "as is" without express or implied warranty.
 */


#ifndef GNUPLOT_FIT_H		/* avoid multiple inclusions */
#define GNUPLOT_FIT_H

/* #if... / #include / #define collection: */

#include "plot.h"

/* compatible with gnuplot philosophy */
#define STANDARD stderr

/* Suffix of a backup file */
#define BACKUP_SUFFIX ".old"

/*****************************************************************
    Useful macros
    We avoid any use of varargs/stdargs (not good style but portable)
*****************************************************************/
#define Eex(a)	    {sprintf (fitbuf+9, (a));         error_ex ();}
#define Eex2(a,b)   {sprintf (fitbuf+9, (a),(b));     error_ex ();}
#define Eex3(a,b,c) {sprintf (fitbuf+9, (a),(b),(c)); error_ex ();}

/* Type definitions */

/* Exported Variables of fit.c */

extern char fitbuf[];

/* Prototypes of functions exported by fit.c */

extern void error_ex __PROTO((void));
void init_fit __PROTO((void));
void update __PROTO((char *pfile, char *npfile));
void fit_command __PROTO((void));
size_t wri_to_fil_last_fit_cmd __PROTO((FILE *fp));

#endif /* GNUPLOT_FIT_H */
