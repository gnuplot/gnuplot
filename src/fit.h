/* $Id: fit.h,v 1.2 1999/06/09 12:08:44 lhecking Exp $ */

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


#ifndef FIT_H		/* avoid multiple inclusions */
#define FIT_H

/* compatible with gnuplot philosophy */
#define STANDARD stderr

/* Suffix of a backup file */
#define BACKUP_SUFFIX ".old"

extern char fitbuf[];

extern void error_ex __PROTO((void));

/*****************************************************************
    Useful macros
    We avoid any use of varargs/stdargs (not good style but portable)
*****************************************************************/
#define Eex(a)	    {sprintf (fitbuf+9, (a));         error_ex ();}
#define Eex2(a,b)   {sprintf (fitbuf+9, (a),(b));     error_ex ();}
#define Eex3(a,b,c) {sprintf (fitbuf+9, (a),(b),(c)); error_ex ();}

#endif /* FIT_H */
