/*
 * $Id: ansichek.h,v 1.2.2.1 2000/05/03 21:26:10 joze Exp $
 */

/* GNUPLOT - ansichek.h */

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


/* figure out if we can handle ANSI prototypes
 * We invent and use ANSI_C rather than using __STDC__ directly
 * because some compilers (eg MS VC++ 4.1) switch off __STDC__
 * when extensions are enabled, but if extensions are disabled,
 * the standard headers cause compile errors. We can -DANSI_C
 * but -D__STDC__ might confuse headers
 */

#ifndef ANSI_CHECK_H
# define ANSI_CHECK_H

# if defined(__STDC__) && __STDC__
#  ifndef ANSI_C
#   define ANSI_C
#  endif
# endif /* __STDC__ */

# ifndef HAVE_CONFIG_H
/* Only relevant for systems which don't run configure */

/* are all these compiler tests necessary ? - can the makefiles not
 * just set ANSI_C ?
 */

#  if defined(ANSI_C) || defined(__TURBOC__) || defined (__PUREC__) || defined (__ZTC__) || defined (_MSC_VER) || (defined(OSK) && defined(_ANSI_EXT))
#   ifndef PROTOTYPES
#    define PROTOTYPES
#   endif
#   ifndef HAVE_STRINGIZE
#    ifndef VAXC	   /* not quite ANSI_C */
#     define HAVE_STRINGIZE
#    endif
#   endif
#  endif /* ANSI_C ... */

#  ifndef ANSI_C
#   define const
#  endif

# endif /* !HAVE_CONFIG_H */

/* used to be __P but it was just too difficult to guess whether
 * standard headers define it. It's not as if the defn is
 * particularly difficult to do ourselves...
 */
# ifdef PROTOTYPES
#  define __PROTO(proto) proto
# else
#  define __PROTO(proto) ()
# endif

/* generic pointer type. For old compilers this has to be changed to char *,
 * but I don't know if there are any CC's that support void and not void *
 */
#  define generic void

#endif /* ANSI_CHECK_H */
