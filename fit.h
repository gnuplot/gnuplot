/* $Id: fit.h,v 1.11 1998/04/14 00:15:21 drd Exp $ */

/*  NOTICE: Change of Copyright Status
 *
 *  The author of this module, Carsten Grammes, has expressed in
 *  personal email that he has no more interest in this code, and
 *  doesn't claim any copyright. He suggests to put this code
 *  under GPL. This is not compatible with the current gnuplot
 *  copyright. But there is no problem to use this code in a
 *  different project under different copyright conditions.
 *
 *  Lars Hecking  11-02-1999
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

#ifdef EXT
#undef EXT
#endif

#ifdef FIT_MAIN
#define EXT
#else
#define EXT extern
#endif

#include "plot.h"

#include "ansichek.h"

#define FIT_SKIP "FIT_SKIP"

EXT char    *fit_index;
EXT char    fitbuf[256];

/*****************************************************************
    Useful macros
    We avoid any use of varargs/stdargs (not good style but portable)
*****************************************************************/

#define Dblf(a)         {fprintf (STANDARD,a); fprintf (log_f,a);}
#define Dblf2(a,b)      {fprintf (STANDARD,a,b); fprintf (log_f,a,b);}
#define Dblf3(a,b,c)    {fprintf (STANDARD,a,b,c); fprintf (log_f,a,b,c);}
#define Dblf5(a,b,c,d,e) \
                {fprintf (STANDARD,a,b,c,d,e); fprintf (log_f,a,b,c,d,e);}
#define Dblf6(a,b,c,d,e,f) \
                {fprintf (STANDARD,a,b,c,d,e,f); fprintf (log_f,a,b,c,d,e,f);}

#define Eex(a)	    {sprintf (fitbuf+9, (a));         error_ex ();}
#define Eex2(a,b)   {sprintf (fitbuf+9, (a),(b));     error_ex ();}
#define Eex3(a,b,c) {sprintf (fitbuf+9, (a),(b),(c)); error_ex ();}

EXT void error_ex __PROTO((void));

#endif
