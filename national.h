/* $Id: national.h,v 1.4 1998/03/22 22:31:53 drd Exp $ */

/* GNUPLOT - national.h */

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


/* if setlocale is available, these strings will be overridden
 * by strftime, or they may not be used at all if the run-time
 * system provides global variables with these strings
 */

#ifdef NORWEGIAN
/* MONTH-names */
#define AMON01 "Jan"
#define AMON02 "Feb"
#define AMON03 "Mar"
#define AMON04 "Apr"
#define AMON05 "Mai"
#define AMON06 "Jun"
#define AMON07 "Jul"
#define AMON08 "Aug"
#define AMON09 "Sep"
#define AMON10 "Okt"
#define AMON11 "Nov"
#define AMON12 "Des"
/* sorry, I dont know the full names */
#define FMON01 "January"
#define FMON02 "February"
#define FMON03 "March"
#define FMON04 "April"
#define FMON05 "May"
#define FMON06 "June"
#define FMON07 "July"
#define FMON08 "August"
#define FMON09 "September"
#define FMON10 "October"
#define FMON11 "November"
#define FMON12 "December"


/* DAY names */
#define ADAY0 "Sxn"
#define ADAY1 "Man"
#define ADAY2 "Tir"
#define ADAY3 "Ons"
#define ADAY4 "Tor"
#define ADAY5 "Fre"
#define ADAY6 "Lxr"

#define FDAY0 "Sunday"
#define FDAY1 "Monday"
#define FDAY2 "Tuesday"
#define FDAY3 "Wednesday"
#define FDAY4 "Thursday"
#define FDAY5 "Friday"
#define FDAY6 "Saturday"


#else

/* MONTH-names */
#define AMON01 "Jan"
#define AMON02 "Feb"
#define AMON03 "Mar"
#define AMON04 "Apr"
#define AMON05 "May"
#define AMON06 "Jun"
#define AMON07 "Jul"
#define AMON08 "Aug"
#define AMON09 "Sep"
#define AMON10 "Oct"
#define AMON11 "Nov"
#define AMON12 "Dec"

#define FMON01 "January"
#define FMON02 "February"
#define FMON03 "March"
#define FMON04 "April"
#define FMON05 "May"
#define FMON06 "June"
#define FMON07 "July"
#define FMON08 "August"
#define FMON09 "September"
#define FMON10 "October"
#define FMON11 "November"
#define FMON12 "December"


/* DAY names */
#define ADAY0 "Sun"
#define ADAY1 "Mon"
#define ADAY2 "Tue"
#define ADAY3 "Wed"
#define ADAY4 "Thu"
#define ADAY5 "Fri"
#define ADAY6 "Sat"

#define FDAY0 "Sunday"
#define FDAY1 "Monday"
#define FDAY2 "Tuesday"
#define FDAY3 "Wednesday"
#define FDAY4 "Thursday"
#define FDAY5 "Friday"
#define FDAY6 "Saturday"
#endif /* language */
