/* GNUPLOT - bin_hook.c */

/*[
 * Copyright 2004  Daniel Sebald
 *
 * This file created for part of Gnuplot, which is
 *
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

/*
 * AUTHORS
 *
 * Daniel Sebald
 *
 */

/* Some additional support in gnuplot for generalized binary obsoletes the
 * file binary.c.  However, to conditionally compile the new code into the
 * program while at the same time conditionally removing binary.c from the
 * program is done via this hook.  Excluding binary.c within Makefile.am
 * does not work, because then if the new general binary code is excluded
 * binary.o will not be linked in.  Putting a defined switch inside of
 * binary.c will not work because the program bf_test requires binary.c.
 * That is, binary.o can't be compiled as two different versions at one
 * time.
 *
 * This is intended as a temporary file, as eventually the define switch
 * BINARY_DATA_FILE may be removed from the project.  In which case,
 * Makefile.am may be altered so that binary.o is not linked in gnuplot. 
 */
#ifndef BINARY_DATA_FILE
#include "binary.c"
#endif
