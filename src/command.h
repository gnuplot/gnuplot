/* $Id: $ */

/* GNUPLOT - command.h */

/*[
 * Copyright 1999
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

#ifndef GNUPLOT_COMMAND_H
# define GNUPLOT_COMMAND_H

/* Collect all global vars in one file.
 * The comments may go at a later date,
 * but are needed for reference now
 *
 * Maybe this should be split into separate files
 * for 2d/3d/parser/other?
 *
 * The ultimate target is of course to eliminate global vars.
 */

/* Formerly in plot.h */
#define AXIS_ARRAY_SIZE 10

/* Formerly in command.c */
/* jev -- for passing data thru user-defined function */
/* NULL means no dummy vars active */
WHERE struct udft_entry ydata_func;

WHERE struct udft_entry *dummy_func;

/* current dummy vars */
WHERE char c_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];

/* input data, parsing variables */
#ifdef AMIGA_SC_6_1
__far int num_tokens, c_token;
#else
int num_tokens, c_token;
#endif

#endif /* GNUPLOT_COMMAND_H */
