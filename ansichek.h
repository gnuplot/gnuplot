/*
 * $Id: ansichek.h,v 1.11 1996/12/09 15:57:43 drd Exp $
 *
 */

/* figure out if we can handle ANSI prototypes
 * We invent and use ANSI_C rather than using __STDC__ directly
 * because some compilers (eg MS VC++ 4.1) switch off __STDC__
 * when extensions are enabled, but if extensions are disabled,
 * the standard headers cause compile errors. We can -DANSI_C
 * but -D__STDC__ might confuse headers
 */

#ifndef ANSI_CHECK_H
#define ANSI_CHECK_H

#ifndef AUTOCONF
/* configure already tested the ANSI feature */

#if !defined(ANSI_C) && defined(__STDC__) && __STDC__
#define ANSI_C
#endif

/* are all these compiler tests necessary ? - can the makefiles not
 * just set ANSI_C ?
 */

#if defined(ANSI_C) || defined(__TURBOC__) || defined (__PUREC__) || defined (__ZTC__) || defined (_MSC_VER) || (defined(OSK) && defined(_ANSI_EXT))
#define PROTOTYPES
#endif

#endif /* AUTOCONF */

/* used to be __P be it was just too difficult to guess whether
 * standard headers define it. It's not as if the defn is
 * particularly difficult to do ourselves...
 */

#ifdef PROTOTYPES
#define __PROTO(proto) proto
#else
#define __PROTO(proto) ()
#endif


/* generic pointer type. For old compilers this has to be changed to char *,
   but I don't know if there are any CC's that support void and not void * */

#define generic void

/* undef const for old compilers
 * I think autoconf tests const : why don't we use its test ?
 */

#ifndef ANSI_C
#define const
#endif

#endif /* ANSI_CHECK_H */
