/* GNUPLOT - version.c */
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

char version[] = "3.0 ";
char patchlevel[] = "1, Dec 1 91";
char date[] = "Sun Dec 1 16:56:36 1991";

/* override in Makefile */
#ifndef CONTACT
# define CONTACT "pixar!bug-gnuplot@sun.com"
#endif
char bug_email[] = CONTACT;
