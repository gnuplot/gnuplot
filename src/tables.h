/* $Id: $ */

/* GNUPLOT - tables.h */

#ifndef GNUPLOT_TABLES_H
#define GNUPLOT_TABLES_H

/* The basic structure */
struct gen_table {
    const char *key;
    int value; /* will be function address */
};

/* gnuplot commands */
enum command_id {
    CMD_INVALID,
    CMD_NULL,
    CMD_CALL, CMD_CD, CMD_CLEAR, CMD_EXIT, CMD_FIT, CMD_HELP, CMD_HISTORY,
    CMD_IF, CMD_LOAD, CMD_PAUSE, CMD_PLOT, CMD_PRINT, CMD_PWD, CMD_REPLOT,
    CMD_REREAD, CMD_RESET, CMD_SAVE, CMD_SCREENDUMP, CMD_SET, CMD_SHELL,
    CMD_SHOW, CMD_SPLOT, CMD_SYSTEM, CMD_TEST, CMD_TESTTIME, CMD_UPDATE
};

/* options for save command */
enum save_id { SAVE_INVALID, SAVE_FUNCS, SAVE_SET, SAVE_VARS };

/* Function prototypes */
int lookup_table __PROTO((struct gen_table *, int));

#endif /* GNUPLT_TABLES_H */
