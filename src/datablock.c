/* GNUPLOT - datablock.c */

/*[
 * Copyright Ethan A Merritt 2012
 *
 * Gnuplot license:
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
 *
 * Alternative license:
 *
 * As an alternative to distributing code in this file under the gnuplot license,
 * you may instead comply with the terms below. In this case, redistribution and
 * use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.  Redistributions in binary
 * form must reproduce the above copyright notice, this list of conditions and
 * the following disclaimer in the documentation and/or other materials provided
 * with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 *
]*/

#include "gp_types.h"
#include "alloc.h"
#include "command.h"
#include "datablock.h"
#include "datafile.h"
#include "eval.h"
#include "misc.h"
#include "util.h"

#ifdef USE_FUNCTIONBLOCKS
/* Set by "return" command and pushed onto the evaluation stack by f_eval */
struct value eval_return_value;

/* Used by f_eval to pass parameters to a function block */
struct value eval_parameters[9];

/* Non-zero to allow values to remain on the evaluation stack across
 * a call to f_eval().  Set on entry to f_eval; cleared when control returns
 * to the LFS depth just above that.  If you're not using function blocks
 * then this is never set.
 */
int evaluate_inside_functionblock = 0;
#endif

/* static function prototypes */
static int enlarge_datablock(struct value *datablock_value, int extra);
static void new_block( enum DATA_TYPES type );


/*
 * In-line data blocks are implemented as a here-document:
 * $FOO << EOD
 *  data line 1
 *  data line 2
 *  ...
 * EOD
 *
 * The data block name must begin with $ followed by a letter.
 * The string EOD is arbitrary; lines of data will be read from the input stream
 * until the leading characters on the line match the given character string.
 * No attempt is made to parse the data at the time it is read in.
 */
void
datablock_command()
{
    new_block(DATABLOCK);
}

/*
 * Function blocks are currently identical to data blocks except for
 * the identifying type.
 * function $FUNC << EOD
 *     gnuplot commands
 * EOD
 */
void
functionblock_command()
{
    c_token++;	/* step past "function" command */
    new_block(FUNCTIONBLOCK);
}

static void
new_block( enum DATA_TYPES type )
{
    FILE *fin;
    char *name, *eod;
    int nlines;
    int nsize = 4;
    struct udvt_entry *datablock;
    char *dataline = NULL;
    char **data_array = NULL;

    if (!isletter(c_token+1))
	int_error(c_token, "illegal block name");

    /* Create or recycle a datablock with the requested name */
    name = parse_datablock_name();
    datablock = add_udv_by_name(name);
    free_value(&datablock->udv_value);

    if (type == FUNCTIONBLOCK) {
	datablock->udv_value.type = FUNCTIONBLOCK;
	datablock->udv_value.v.functionblock.parnames = NULL;
	datablock->udv_value.v.functionblock.data_array = NULL;
	if (equals(c_token, "(")) {
	    int dummy_num = 0;
	    datablock->udv_value.v.functionblock.parnames
		= gp_alloc( 10*sizeof(char *), "function block");
	    memset(datablock->udv_value.v.functionblock.parnames, 0, 10*sizeof(char *));
	    do {
		c_token++;
		m_capture(&(datablock->udv_value.v.functionblock.parnames[dummy_num++]),
			c_token, c_token);
	    } while (equals(++c_token, ",") && (dummy_num < 9));
	    if (!equals(c_token, ")"))
		int_error(c_token, "expecting ')'");
	    c_token++;
	}
    } else {
	/* Must be a datablock */
	datablock->udv_value.type = DATABLOCK;
	datablock->udv_value.v.data_array = NULL;
    }

    if (!equals(c_token, "<<") || !isletter(c_token+1))
	int_error(c_token, "block definition line must end with << EODmarker");
    c_token++;
    eod = (char *) gp_alloc(token[c_token].length +2, "datablock");
    copy_str(&eod[0], c_token, token[c_token].length + 2);
    c_token++;

    /* Read in and store data lines until EOD */
    fin = (lf_head == NULL) ? stdin : lf_head->fp;
    if (!fin)
	int_error(NO_CARET,"attempt to define data block from invalid context");
    for (nlines = 0; (dataline = df_fgets(fin)); nlines++) {
	int n;

	if (!strncmp(eod, dataline, strlen(eod)))
	    break;
	/* Allocate space for data lines plus at least 2 empty lines at the end. */
	if (nlines >= nsize-4) {
	    nsize *= 2;
	    data_array = (char **) gp_realloc(data_array,
			nsize * sizeof(char *), "datablock");
	    memset(&data_array[nlines], 0,
		    (nsize - nlines) * sizeof(char *));
	}
	/* Strip trailing newline character */
	n = strlen(dataline);
	if (n > 0 && dataline[n - 1] == '\n')
	    dataline[n - 1] = NUL;
	data_array[nlines] = gp_strdup(dataline);
    }
    inline_num += nlines + 1;	/* Update position in input file */

    if (type == FUNCTIONBLOCK)
	datablock->udv_value.v.functionblock.data_array = data_array;
    else
	datablock->udv_value.v.data_array = data_array;

    /* make sure that we can safely add lines to this datablock later on */
    enlarge_datablock(&datablock->udv_value, 0);

    free(eod);
    return;
}


char *
parse_datablock_name()
{
    /* Datablock names begin with $, but the scanner puts  */
    /* the $ in a separate token.  Merge it with the next. */
    /* Caller must _not_ free the string that is returned. */
    static char *name = NULL;

    free(name);
    c_token++;
    name = (char *) gp_alloc(token[c_token].length + 2, "datablock");
    name[0] = '$';
    copy_str(&name[1], c_token, token[c_token].length + 2);
    c_token++;

    return name;
}


char **
get_datablock(char *name)
{
    struct udvt_entry *datablock;

    datablock = get_udv_by_name(name);
    if (!datablock || datablock->udv_value.type != DATABLOCK
    ||  datablock->udv_value.v.data_array == NULL)
	int_error(NO_CARET,"no datablock named %s",name);

    return datablock->udv_value.v.data_array;
}


void
gpfree_datablock(struct value *datablock_value)
{
    int i;
    char **stored_data = datablock_value->v.data_array;

    if (datablock_value->type != DATABLOCK)
	return;
    if (stored_data)
	for (i=0; stored_data[i] != NULL; i++)
	    free(stored_data[i]);
    free(stored_data);
    datablock_value->v.data_array = NULL;
    datablock_value->type = NOTDEFINED;
}

void
gpfree_functionblock(struct value *block_value)
{
    int i;
    char **stored_data;

    if (block_value->type != FUNCTIONBLOCK)
	return;
    stored_data = block_value->v.functionblock.data_array;
    if (stored_data)
	for (i=0; stored_data[i] != NULL; i++)
	    free(stored_data[i]);
    free(stored_data);
    stored_data = block_value->v.functionblock.parnames;
    if (stored_data)
	for (i=0; stored_data[i] != NULL; i++)
	    free(stored_data[i]);
    free(stored_data);

    block_value->v.functionblock.data_array = NULL;
    block_value->v.functionblock.parnames = NULL;
    block_value->type = NOTDEFINED;
}

/* count number of lines in a datablock */
int
datablock_size(struct value *datablock_value)
{
    char **dataline;
    int nlines = 0;

    if (datablock_value->type == FUNCTIONBLOCK)
	dataline = datablock_value->v.functionblock.data_array;
    else
	dataline = datablock_value->v.data_array;
    if (dataline) {
	while (*dataline++)
	    nlines++;
    }
    return nlines;
}

/* resize or allocate a datablock; allocate memory in chunks */
static int
enlarge_datablock(struct value *datablock_value, int extra)
{
    int osize, nsize;
    const int blocksize = 512;
    int nlines = datablock_size(datablock_value);

    /* reserve space in multiples of blocksize */
    osize = ((nlines+1 + blocksize-1) / blocksize) * blocksize; 
    nsize = ((nlines+1 + extra + blocksize-1) / blocksize) * blocksize;

    /* only resize if necessary */
    if ((osize != nsize) || (extra == 0) || (nlines == 0)) {
	if (datablock_value->type == FUNCTIONBLOCK) {
	    datablock_value->v.data_array =
		(char **) gp_realloc(datablock_value->v.functionblock.data_array,
					nsize * sizeof(char *), "resize_datablock");
	    datablock_value->v.functionblock.data_array[nlines] = NULL;
	} else {
	    datablock_value->v.data_array =
		(char **) gp_realloc(datablock_value->v.data_array,
					nsize * sizeof(char *), "resize_datablock");
	    datablock_value->v.data_array[nlines] = NULL;
	}
    }

    return nlines;
}


/* append a single line to a datablock */
void
append_to_datablock(struct value *datablock_value, const char *line)
{
    int nlines = enlarge_datablock(datablock_value, 1);
    datablock_value->v.data_array[nlines] = (char *) line;
    datablock_value->v.data_array[nlines + 1] = NULL;
}


/* append multiple lines which are separated by linebreaks to a datablock */
void
append_multiline_to_datablock(struct value *datablock_value, const char *lines)
{
    char * l = (char *) lines;
    char * p;
    TBOOLEAN inquote = FALSE;
    TBOOLEAN escaped = FALSE;

    /* handle lines with line-breaks, one at a time;
       take care of quoted strings
     */
    p = l;
    while (*p != NUL) {
	if (*p == '\'' && !escaped)
	    inquote = !inquote;
	else if (*p == '\\' && !escaped)
	    escaped = TRUE;
	else if (*p == '\n' && !inquote) {
	    *p = NUL;
	    append_to_datablock(datablock_value, strdup(l));
	    l = p + 1;
	} else
	    escaped = FALSE;
        p++;
    }
    if (l == lines) {
	/* no line-breaks, just a single line */
	append_to_datablock(datablock_value, l);
    } else {
	if (strlen(l) > 0) /* remainder after last line-break */
	    append_to_datablock(datablock_value, strdup(l));
	/* we allocated new sub-strings, free the original */
	free((char *) lines);
    }
}

#ifdef USE_FUNCTIONBLOCKS
/* Internal version of eval() that can be called from the evaluation stack
 * when a function block is invoked.
 */
void
f_eval(union argument *arg)
{
    udvt_entry *functionblock;
    struct value num_params;
    int nparams, i;

    nparams = pop(&num_params)->v.int_val;

    functionblock = arg->udv_arg;
    if (functionblock->udv_value.type != FUNCTIONBLOCK)
	int_error(NO_CARET, "attempt to execute something other than a function block");

    /* Clear any previous return value */
    gpfree_string(&eval_return_value);
    eval_return_value.type = NOTDEFINED;

    /* Buffer any parameters for load_file to stuff in ARGV[] */
    for (i = 0; i < 9; i++) {
	if (i < nparams)
	    pop( &eval_parameters[nparams - (i+1)] );
	else
	    eval_parameters[i].type = NOTDEFINED;
    }

    load_file( NULL, (void *)(functionblock), 8);
    push(&eval_return_value);
}
#else	/* USE_FUNCTIONBLOCKS */
void f_eval(union argument *arg)
{
    int_error(NO_CARET, "This copy of gnuplot does not support function block evaluation");
}

#endif	/* USE_FUNCTIONBLOCKS */
