/*
 * $Id: variable.h,v 1.7.2.1 2000/05/03 21:26:12 joze Exp $
 */

/* GNUPLOT - variable.h */

/*[
 * Copyright 1999   Lars Hecking
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

/* The death of global variables - part one. */

#ifndef VARIABLE_H
# define VARIABLE_H

#include "plot.h"
#include "national.h"

/* Generic defines */

#ifdef ACTION_NULL
# undef ACTION_NULL
#endif

#ifdef ACTION_INIT
# undef ACTION_INIT
#endif

#ifdef ACTION_SHOW
# undef ACTION_SHOW
#endif

#ifdef ACTION_SET
# undef ACTION_SET
#endif

#ifdef ACTION_GET
# undef ACTION_GET
#endif

#ifndef ACTION_SAVE
# undef ACTION_SAVE
#endif

#ifdef ACTION_CLEAR
# undef ACTION_CLEAR
#endif

#define ACTION_NULL   0
#define ACTION_INIT   (1<<0)
#define ACTION_SHOW   (1<<1)
#define ACTION_SET    (1<<2)
#define ACTION_GET    (1<<3)
#define ACTION_SAVE   (1<<4)
#define ACTION_CLEAR  (1<<5)

/* Loadpath related */

extern char *loadpath_handler __PROTO((int, char *));
extern char *locale_handler __PROTO((int, char *));

#define init_loadpath()    loadpath_handler(ACTION_INIT,NULL)
#define set_var_loadpath(path) loadpath_handler(ACTION_SET,(path))
#define get_loadpath()     loadpath_handler(ACTION_GET,NULL)
#define save_loadpath()    loadpath_handler(ACTION_SAVE,NULL)
#define clear_loadpath()   loadpath_handler(ACTION_CLEAR,NULL)

/* Locale related */

#define INITIAL_LOCALE ("C")

#define init_locale()      locale_handler(ACTION_INIT,NULL)
#define set_var_locale(path)   locale_handler(ACTION_SET,(path))
#define get_locale()       locale_handler(ACTION_GET,NULL)
#define save_locale()      locale_handler(ACTION_SAVE,NULL)
#define clear_locale()     locale_handler(ACTION_CLEAR,NULL)

extern char full_month_names[12][32];
extern char abbrev_month_names[12][8];
extern char full_day_names[7][32];
extern char abbrev_day_names[7][8];

#endif /* VARIABLE_H */
