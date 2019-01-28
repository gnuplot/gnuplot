/* GNUPLOT - util.h */

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

#ifndef GNUPLOT_UTIL_H
# define GNUPLOT_UTIL_H

#include "gp_types.h"
#include "stdfn.h"		/* for size_t */

/* special token number meaning 'do not draw the "caret"', for
 * int_error and friends: */
#define NO_CARET (-1)

/* token number meaning 'the error was in the datafile, not the
   command line' */
#define DATAFILE (-2)

/* TRUE if command just typed; becomes FALSE whenever we
 * send some other output to screen.  If FALSE, the command line
 * will be echoed to the screen before the ^ error message.
 */
extern TBOOLEAN screen_ok;

/* decimal sign */
extern char *decimalsign;
extern char *numeric_locale;	/* LC_NUMERIC */
extern char *time_locale;	/* LC_TIME */

/* degree sign */
extern char degree_sign[8];

/* special characters used by gprintf() */
extern const char *micro;
extern const char *minus_sign;
extern TBOOLEAN use_micro;
extern TBOOLEAN use_minus_sign;

extern const char *current_prompt; /* needed by is_error() and friends */

extern int debug;

/* Functions exported by util.c: */

/* Command parsing helpers: */
int equals(int, const char *);
int almost_equals(int, const char *);
char *token_to_string(int);
int isstring(int);
int isanumber(int);
int isletter(int);
int is_definition(int);
TBOOLEAN might_be_numeric(int);
void copy_str(char *, int, int);
size_t token_len(int);
void capture(char *, int, int, int);
void m_capture(char **, int, int);
void m_quote_capture(char **, int, int);
char *try_to_get_string(void);
void parse_esc(char *);
int type_udv(int);

char *gp_stradd(const char *, const char *);
#define isstringvalue(c_token) (isstring(c_token) || type_udv(c_token)==STRING)

/* HBB 20010726: IMHO this one belongs into alloc.c: */
char *gp_strdup(const char *);

void gprintf(char *, size_t, char *, double, double);
void gprintf_value(char *, size_t, char *, double, struct value *);

/* Error message handling */
#if defined(VA_START) && defined(STDC_HEADERS)
#  if defined(__GNUC__)
    void os_error(int, const char *, ...) __attribute__((noreturn));
    void int_error(int, const char *, ...) __attribute__((noreturn));
    void common_error_exit(void) __attribute__((noreturn));
#  elif defined(_MSC_VER)
    __declspec(noreturn) void os_error(int, const char *, ...);
    __declspec(noreturn) void int_error(int, const char *, ...);
    __declspec(noreturn) void common_error_exit();
#  else
    void os_error(int, const char *, ...);
    void int_error(int, const char *, ...);
    void common_error_exit(void);
#  endif
void int_warn(int, const char *, ...);
#else
void os_error();
void int_error();
void int_warn();
void common_error_exit();
#endif

void squash_spaces(char *s, int remain);

TBOOLEAN existdir(const char *);
TBOOLEAN existfile(const char *);

char *getusername(void);

size_t gp_strlen(const char *s);
char * gp_strchrn(const char *s, int N);
TBOOLEAN streq(const char *a, const char *b);
size_t strappend(char **dest, size_t *size, size_t len, const char *src);

char *value_to_str(struct value *val, TBOOLEAN need_quotes);

char *texify_title(char *title, int plot_type);

/* To disallow 8-bit characters in variable names, set this to */
/* #define ALLOWED_8BITVAR(c) FALSE */
#define ALLOWED_8BITVAR(c) ((c)&0x80)

#endif /* GNUPLOT_UTIL_H */
