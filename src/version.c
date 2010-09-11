#ifndef lint
static char *RCSid() { return RCSid("$Id: version.c,v 1.89.2.8 2010/03/05 06:52:24 sfeam Exp $"); }
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


const char gnuplot_version[] = "4.4";
const char gnuplot_patchlevel[] = "1";
const char gnuplot_date[] = "September 2010";
const char gnuplot_copyright[] = "Copyright (C) 1986-1993, 1998, 2004, 2007-2010";

char *compile_options = (void *)0;	/* Will be loaded at runtime */

# ifndef FAQ_LOCATION
#  define FAQ_LOCATION "http://www.gnuplot.info/faq/"
# endif
# ifndef HELPMAIL
#  define HELPMAIL "gnuplot-info@lists.sourceforge.net";
# endif
# ifndef CONTACT
#  define CONTACT "gnuplot-beta@lists.sourceforge.ent";
# endif
# ifndef SUPPORT
#  define SUPPORT "http://sf.net/projects/gnuplot/support"
# endif

const char faq_location[] = FAQ_LOCATION;
const char bug_email[] = CONTACT;
const char help_email[] = HELPMAIL;
const char bug_report[] = SUPPORT;

char os_name[32];
char os_rel[32];
