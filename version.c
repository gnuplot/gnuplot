/* GNUPLOT - version.c */
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
 *   Gnuplot 3.4 additions:
 *       Alex Woo and many others.
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
static char *RCSid = "$Id: version.c%v 3.50.1.17 1993/08/27 05:21:33 woo Exp woo $";
 */

char version[] = "3.5 ";
char patchlevel[] = "3.50.1.17, 27 Aug 93";
char date[] = "Fri Aug 27 05:21:33 GMT 1993 "; 
char copyright[] = "Copyright(C) 1986 - 1993";


/* override in Makefile */
#ifndef CONTACT
# define CONTACT "bug-gnuplot@dartmouth.edu"
#endif
#ifndef HELPMAIL
# define HELPMAIL "info-gnuplot@dartmouth.edu"
#endif
char bug_email[] = CONTACT;
char help_email[] = HELPMAIL;
