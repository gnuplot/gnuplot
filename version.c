#ifndef lint
static char *RCSid = "$Id: version.c,v 1.22.2.5 1999/08/25 12:17:40 lhecking Exp $";
#endif

/* GNUPLOT - version.c */

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

#include "plot.h"

char version[] = "3.7.1";
char patchlevel[] = "beta6";
char date[] = "Wed Aug 25 16:49:10 BST 1999";
cha gnuplot_copyright[] = "Copyright(C) 1986 - 1993, 1998, 1999";

char faq_location[] = FAQ_LOCATION;
char bug_email[] = CONTACT;
char help_email[] = HELPMAIL;
