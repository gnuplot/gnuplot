/* $Id: fit.h,v 1.11 1998/04/14 00:15:21 drd Exp $ */

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

/*#include "type.h"*/  /*HBB 961110: unused now! */
#include "plot.h"

#include "ansichek.h"

#define FIT_SKIP "FIT_SKIP"

EXT char    *fit_index;
EXT char    fitbuf[256];


/******* public functions ******/

EXT char    *get_next_word __PROTO((char **s, char *subst)); 

EXT void    init_fit __PROTO((void));
EXT void    setvar __PROTO((char *varname, struct value data));
EXT int     getivar __PROTO((char *varname));
EXT void    update __PROTO((char *pfile, char *npfile));
EXT void    do_fit __PROTO((void));

/********* Macros *********/

#define Eex(a)	    {sprintf (fitbuf+9, a);	error_ex ();}
#define Eex2(a,b)   {sprintf (fitbuf+9, a,b);	error_ex ();}
#define Eex3(a,b,c) {sprintf (fitbuf+9, a,b,c); error_ex ();}

EXT void error_ex __PROTO((void));

#endif
