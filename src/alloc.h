/*
 * $Id: alloc.h,v 1.5.2.1 2000/05/03 21:26:10 joze Exp $
 */

/* GNUPLOT - alloc.h */

/*[
 * Copyright 1986 - 1993, 1998   Thomas Williams, Colin Kelley
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

#ifndef GNUPLOT_ALLOC_H
# define GNUPLOT_ALLOC_H

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "stdfn.h"

/* prototypes from "alloc.c". This file figures out if the free hack is needed
 * and redefines free if necessary.
 */

generic *gp_alloc __PROTO((size_t size, const char *message));
generic *gp_realloc __PROTO((generic *p, size_t size, const char *message));

/* dont define CHECK_HEAP_USE on a FARALLOC machine ! */

#ifdef CHECK_HEAP_USE

/* all allocated blocks have guards at front and back.
 * CHECK_POINTER checks guards on block, and checks that p is in range
 * START_LEAK_CHECK and END_LEAK_CHECK allow assert that no net memory
 * is allocated within enclosed block
 */
 
void checked_free(void *p);
void check_pointer_in_block(void *block, void *p, int size, char *file, int line);
void start_leak_check(char *file,int line);
void end_leak_check(char *file,int line);
# define free(x) checked_free(x)
# define CHECK_POINTER(block, p) check_pointer_in_block(block, p, sizeof(*p), __FILE__, __LINE__)
# define START_LEAK_CHECK() start_leak_check(__FILE__, __LINE__)
# define END_LEAK_CHECK() end_leak_check(__FILE__, __LINE__)
#else
# define CHECK_POINTER(block, p) /*nowt*/
# define START_LEAK_CHECK() /*nowt*/
# define END_LEAK_CHECK() /*nowt*/
#endif

#if defined(MSDOS) && defined(__TURBOC__) && !defined(DOSX286) || defined(_Windows) && !defined(WIN32)
#define FARALLOC
void gpfree __PROTO((generic *p));
#define free gpfree
#endif

#endif /* GNUPLOT_ALLOC_H */
