#ifndef lint
static char *RCSid() { return RCSid("$Id: version.c,v 1.36 2000/11/01 18:57:34 broeker Exp $"); }
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

#include "version.h"

#include "syscfg.h"		/* for FAQ_LOCATIION */


const char gnuplot_version[] = "3.8e";
const char gnuplot_patchlevel[] = "1";
const char gnuplot_date[] = "Thu Nov 23 17:47:39 GMT 2000";
const char gnuplot_copyright[] = "Copyright(C) 1986 - 1993, 1999, 2000";

const char faq_location[] = FAQ_LOCATION;
#ifdef RELEASE_VERSION
/* mustn't forget to take this out before the release ... */
const char bug_email[] = CONTACT;
const char help_email[] = HELPMAIL;
#else
const char bug_email[] = "info-gnuplot-beta@dartmouth.edu";
const char help_email[] = "info-gnuplot-beta@dartmouth.edu";
#endif
char os_name[32];
char os_rel[32];
