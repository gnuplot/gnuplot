/*
 * $Id: command.h,v 1.23 2002/10/20 21:19:50 mikulik Exp $
 */

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

#include "gp_types.h"
#include "stdfn.h"

#define PROMPT "gnuplot> "

extern char *input_line;

extern int inline_num;

typedef struct lexical_unit {	/* produced by scanner */
    TBOOLEAN is_token;		/* true if token, false if a value */ 
    struct value l_val;
    int start_index;		/* index of first char in token */
    int length;			/* length of token in chars */
} lexical_unit;

extern struct lexical_unit *token;
extern int token_table_size;
extern int plot_token;
#define END_OF_COMMAND (c_token >= num_tokens || equals(c_token,";"))

extern char *replot_line;

/* flag to disable `replot` when some data are sent through stdin;
 * used by mouse/hotkey capable terminals */
extern TBOOLEAN replot_disabled;


extern TBOOLEAN is_3d_plot;

extern struct udft_entry *dummy_func;

#ifndef STDOUT
# define STDOUT 1
#endif

#if defined(MSDOS) || defined(DOS386)
# ifdef DJGPP
extern char HelpFile[];         /* patch for do_help  - AP */
# endif                         /* DJGPP */
# ifdef __TURBOC__
#  ifndef _Windows
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

/* wrapper for calling kill_pending_Pause_dialog() from win/winmain.c */
#ifdef _Windows
void call_kill_pending_Pause_dialog(void);
#endif

/* input data, parsing variables */
#ifdef AMIGA_SC_6_1
extern __far int num_tokens, c_token;
#else
extern int num_tokens, c_token;
#endif

extern size_t input_line_len;

/* used by load_command() and save_command() */

/* Capture filename from unput_line and
 * fopen in `mode' ("r", "w" etc.)
 */
#define CAPTURE_FILENAME_AND_FOPEN(mode) \
  m_quote_capture(&save_file,c_token,c_token); \
  gp_expand_tilde(&save_file); \
  fp = strcmp(save_file, "-") ? loadpath_fopen(save_file, (mode)) : stdout;

#ifdef USE_MOUSE
void bind_command __PROTO((void));
void restore_prompt __PROTO((void));
#endif
void call_command __PROTO((void));
void changedir_command __PROTO((void));
void clear_command __PROTO((void));
void exit_command __PROTO((void));
void help_command __PROTO((void));
void history_command __PROTO((void));
void if_command __PROTO((void));
void else_command __PROTO((void));
void invalid_command __PROTO((void));
void load_command __PROTO((void));
void null_command __PROTO((void));
void pause_command __PROTO((void));
void plot_command __PROTO((void));
void print_command __PROTO((void));
void pwd_command __PROTO((void));
void replot_command __PROTO((void));
void reread_command __PROTO((void));
void save_command __PROTO((void));
void screendump_command __PROTO((void));
void splot_command __PROTO((void));
void system_command __PROTO((void));
void testtime_command __PROTO((void));
void update_command __PROTO((void));
void do_shell __PROTO((void));

/* Prototypes for functions exported by command.c */
void extend_input_line __PROTO((void));
void extend_token_table __PROTO((void));
int com_line __PROTO((void));
int do_line __PROTO((void));
void do_string __PROTO((char* s));
#ifdef USE_MOUSE
void toggle_display_of_ipc_commands __PROTO((void));
int display_ipc_commands __PROTO((void));
void do_string_replot __PROTO((char* s));
#endif
#ifdef VMS                     /* HBB 990829: used only on VMS */
void done __PROTO((int status));
#endif
void define __PROTO((void));

void replotrequest __PROTO((void)); /* used in command.c & mouse.c */

void print_set_output __PROTO((char *, TBOOLEAN)); /* set print output file */
char *print_show_output __PROTO((void)); /* show print output file */

/* Activate/deactive effects of 'set view map' before 'splot'/'plot',
 * respectively. Required for proper mousing during 'set view map';
 * actually it won't be necessary if gnuplot keeps a copy of all variables for
 * the current plot and don't share them with 'set' commands.
 *   These routines need to be executed before each plotrequest() and
 * plot3drequest().
 */
void splot_map_activate __PROTO((void));
void splot_map_deactivate __PROTO((void));

#endif /* GNUPLOT_COMMAND_H */
