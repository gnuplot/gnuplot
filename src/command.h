/* $Id: command.h,v 1.4 1999/07/18 17:40:17 lhecking Exp $ */

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

extern struct udft_entry ydata_func;
extern struct udft_entry *dummy_func;
extern char c_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];

/* Formerly in plot.h */
#define AXIS_ARRAY_SIZE 10

#ifndef STDOUT
# define STDOUT 1
#endif

#if defined(MSDOS) || defined(DOS386)
# ifdef DJGPP
extern char HelpFile[];         /* patch for do_help  - AP */
# endif                         /* DJGPP */
# ifdef __TURBOC__
#  ifndef _Windows
extern unsigned _stklen = 16394;        /* increase stack size */
extern char HelpFile[];         /* patch for do_help  - DJL */
#  endif                        /* _Windows */
# endif                         /* TURBOC */
#endif /* MSDOS */

#ifdef _Windows
# define SET_CURSOR_WAIT SetCursor(LoadCursor((HINSTANCE) NULL, IDC_WAIT))
# define SET_CURSOR_ARROW SetCursor(LoadCursor((HINSTANCE) NULL, IDC_ARROW))
#else
# define SET_CURSOR_WAIT        /* nought, zilch */
# define SET_CURSOR_ARROW       /* nought, zilch */
#endif

/* input data, parsing variables */
#ifdef AMIGA_SC_6_1
extern __far int num_tokens, c_token;
#else
extern int num_tokens, c_token;
#endif

enum command_id {
    CMD_INVALID,        /* invalid command */
    CMD_NULL,           /* null command */
    CMD_CALL, CMD_CD, CMD_CLEAR, CMD_EXIT, CMD_FIT, CMD_HELP, CMD_HISTORY,
    CMD_IF, CMD_LOAD, CMD_PAUSE, CMD_PLOT, CMD_PRINT, CMD_PWD, CMD_QUIT,
    CMD_REPLOT, CMD_REREAD, CMD_RESET, CMD_SAVE, CMD_SCREENDUMP, CMD_SET,
    CMD_SHELL, CMD_SHOW, CMD_SPLOT, CMD_SYSTEM, CMD_TEST, CMD_TESTTIME,
    CMD_UPDATE
};

#endif /* GNUPLOT_COMMAND_H */
