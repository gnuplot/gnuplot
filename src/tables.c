/* $Id: $ */

/* GNUPLOT - tables.c */

#include "plot.h"
#include "tables.h"

/* gnuplot commands
 * for reference only - original in tables.h
 */
/*
enum command_id {
    CMD_INVALID,
    CMD_NULL,
    CMD_CALL, CMD_CD, CMD_CLEAR, CMD_EXIT, CMD_FIT, CMD_HELP, CMD_HISTORY,
    CMD_IF, CMD_LOAD, CMD_PAUSE, CMD_PLOT, CMD_PRINT, CMD_PWD, CMD_REPLOT,
    CMD_REREAD, CMD_RESET, CMD_SAVE, CMD_SCREENDUMP, CMD_SET, CMD_SHELL,
    CMD_SHOW, CMD_SPLOT, CMD_SYSTEM, CMD_TEST, CMD_TESTTIME, CMD_UPDATE
};
*/

/* the actual commands
 * should be in sync with above enum */
struct gen_table command_tbl[] =
{
    { "ca$ll", CMD_CALL },
    { "cd", CMD_CD },
    { "cl$ear", CMD_CLEAR },
    { "ex$it", CMD_EXIT },
    { "f$it", CMD_FIT },
    { "h$elp", CMD_HELP },
    { "?", CMD_HELP },
    { "hi$story", CMD_HISTORY },
    { "if", CMD_IF },
    { "l$oad", CMD_LOAD },
    { "pa$use", CMD_PAUSE },
    { "p$lot", CMD_PLOT },
    { "pr$int", CMD_PRINT },
    { "pwd", CMD_PWD },
    { "q$uit", CMD_EXIT },
    { "rep$lot", CMD_REPLOT },
    { "re$read", CMD_REREAD },
    { "res$et", CMD_RESET },
    { "sa$ve", CMD_SAVE },
    { "scr$eendump", CMD_SCREENDUMP },
    { "se$t", CMD_SET },
    { "she$ll", CMD_SHELL },
    { "sh$ow", CMD_SHOW },
    { "sp$lot", CMD_SPLOT },
    { "sy$stem", CMD_SYSTEM },
    { "test", CMD_TEST },
    { "testt$ime", CMD_TESTTIME },
    { "up$date", CMD_UPDATE },
    { ";", CMD_NULL },
    /* last key must be NULL */
    { NULL, CMD_INVALID }
};

/* save command */
struct gen_table save_tbl[] =
{
    { "f$unctions", SAVE_FUNCS },
    { "s$et", SAVE_SET },
    { "v$ariables", SAVE_VARS },
    { NULL, SAVE_INVALID }
};

int
lookup_table(tbl, token)
struct gen_table *tbl;
int token;
{
    c_token++;

    while (tbl->key) {
	if (almost_equals(token, tbl->key)) {
	    return tbl->value;
	}
	tbl++;
    }
    return tbl->value; /* *_INVALID */

}
