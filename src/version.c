#ifndef lint
static char *RCSid() { return RCSid("$Id: version.c,v 1.69.2.15 2008/03/04 18:36:55 sfeam Exp $"); }
#endif

/* GNUPLOT - version.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
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

#include "syscfg.h"		/* for FAQ_LOCATION */


const char gnuplot_version[] = "4.2";
const char gnuplot_patchlevel[] = "4 ";
const char gnuplot_date[] = "Sep 2008";
const char gnuplot_copyright[] = "Copyright (C) 1986 - 1993, 1998, 2004, 2007, 2008";

const char faq_location[] = FAQ_LOCATION;

char *compile_options = (void *)0;	/* Will be loaded at runtime */

#define RELEASE_VERSION 1

/* mustn't forget to activate this before the release ... */
#ifdef RELEASE_VERSION
# ifndef HELPMAIL
#  define HELPMAIL "gnuplot-info@lists.sourceforge.net";
# endif
# ifndef CONTACT
/* #  define CONTACT "gnuplot-bugs@lists.sourceforge.net"; */
#  define CONTACT "http://sourceforge.net/projects/gnuplot";
# endif

const char bug_email[] = CONTACT;
const char help_email[] = HELPMAIL;

#else

const char bug_email[] = "gnuplot-beta@lists.sourceforge.net";
const char help_email[] = "gnuplot-beta@lists.sourceforge.net";

#endif

char os_name[32];
char os_rel[32];
