/* GNUPLOT - command.h */

/*[
 * Copyright 1999, 2004   Thomas Williams, Colin Kelley
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

extern char *gp_input_line;
extern size_t gp_input_line_len;

extern int inline_num;

extern int if_depth;			/* old if/else syntax only */
extern TBOOLEAN if_open_for_else;	/* new if/else syntax only */
extern TBOOLEAN if_condition;		/* used by both old and new syntax */

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

#ifdef USE_MOUSE
extern int paused_for_mouse;	/* Flag the end condition we are paused until */
#define PAUSE_BUTTON1   001		/* Mouse button 1 */
#define PAUSE_BUTTON2   002		/* Mouse button 2 */
#define PAUSE_BUTTON3   004		/* Mouse button 3 */
#define PAUSE_CLICK	007		/* Any button click */
#define PAUSE_KEYSTROKE 010		/* Any keystroke */
#define PAUSE_WINCLOSE	020		/* Window close event */
#define PAUSE_ANY       077		/* Terminate on any of the above */
#endif

/* output file for the print command */
extern FILE *print_out;
extern struct udvt_entry * print_out_var;
extern char *print_out_name;

extern struct udft_entry *dummy_func;

#ifndef STDOUT
# define STDOUT 1
#endif

#if defined(MSDOS) || defined(OS2)
extern char HelpFile[];
#endif

#ifdef _WIN32
# define SET_CURSOR_WAIT SetCursor(LoadCursor((HINSTANCE) NULL, IDC_WAIT))
# define SET_CURSOR_ARROW SetCursor(LoadCursor((HINSTANCE) NULL, IDC_ARROW))
#else
# define SET_CURSOR_WAIT        /* nought, zilch */
# define SET_CURSOR_ARROW       /* nought, zilch */
#endif

/* input data, parsing variables */
extern int num_tokens, c_token;

void raise_lower_command(int);
void raise_command(void);
void lower_command(void);
#ifdef OS2
extern void pm_raise_terminal_window(void);
extern void pm_lower_terminal_window(void);
#endif
#ifdef X11
extern void x11_raise_terminal_window(int);
extern void x11_raise_terminal_group(void);
extern void x11_lower_terminal_window(int);
extern void x11_lower_terminal_group(void);
#endif
#ifdef _WIN32
extern void win_raise_terminal_window(int);
extern void win_raise_terminal_group(void);
extern void win_lower_terminal_window(int);
extern void win_lower_terminal_group(void);
#endif
#ifdef WXWIDGETS
extern void wxt_raise_terminal_window(int);
extern void wxt_raise_terminal_group(void);
extern void wxt_lower_terminal_window(int);
extern void wxt_lower_terminal_group(void);
#endif
extern void string_expand_macros(void);

#ifdef USE_MOUSE
void bind_command(void);
void restore_prompt(void);
#else
#define bind_command()
#endif
void array_command(void);
void break_command(void);
void call_command(void);
void changedir_command(void);
void clear_command(void);
void continue_command(void);
void eval_command(void);
void exit_command(void);
void help_command(void);
void history_command(void);
void do_command(void);
void if_command(void);
void else_command(void);
void import_command(void);
void invalid_command(void);
void link_command(void);
void load_command(void);
void begin_clause(void);
void clause_reset_after_error(void);
void end_clause(void);
void null_command(void);
void pause_command(void);
void plot_command(void);
void print_command(void);
void printerr_command(void);
void pwd_command(void);
void refresh_request(void);
void refresh_command(void);
void replot_command(void);
void reread_command(void);
void save_command(void);
void screendump_command(void);
void splot_command(void);
void stats_command(void);
void system_command(void);
void test_command(void);
void toggle_command(void);
void update_command(void);
void do_shell(void);
void undefine_command(void);
void while_command(void);

/* Prototypes for functions exported by command.c */
void extend_input_line(void);
void extend_token_table(void);
int com_line(void);
int do_line(void);
void do_string(const char* s);
void do_string_and_free(char* s);
TBOOLEAN iteration_early_exit(void);
#ifdef USE_MOUSE
void toggle_display_of_ipc_commands(void);
int display_ipc_commands(void);
void do_string_replot(const char* s);
#endif
#ifdef VMS                     /* HBB 990829: used only on VMS */
void done(int status);
#endif
void define(void);

void replotrequest(void); /* used in command.c & mouse.c */

void print_set_output(char *, TBOOLEAN, TBOOLEAN); /* set print output file */
char *print_show_output(void); /* show print output file */

int do_system_func(const char *cmd, char **output);

#ifdef OS2_IPC
void os2_ipc_setup(void);
#endif

#endif /* GNUPLOT_COMMAND_H */
