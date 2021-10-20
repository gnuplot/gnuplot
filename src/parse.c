/* GNUPLOT - parse.c */

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

#include "parse.h"

#include "alloc.h"
#include "command.h"
#include "datablock.h"
#include "eval.h"
#include "help.h"
#include "util.h"

/* Protection mechanism for trying to parse a string followed by a + or - sign.
 * Also suppresses an undefined variable message if an unrecognized token
 * is encountered during try_to_get_string().
 */
TBOOLEAN string_result_only = FALSE;
static int parse_recursion_level;

/* Exported globals: the current 'dummy' variable names */
char c_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];
char set_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1] = { "x", "y" };
int  fit_dummy_var[MAX_NUM_VAR];
TBOOLEAN scanning_range_in_progress = FALSE;

/* This is used by plot_option_using() */
int at_highest_column_used = -1;

/* This is checked by df_readascii() */
TBOOLEAN parse_1st_row_as_headers = FALSE;

/* This is used by df_open() and df_readascii() */
udvt_entry *df_array = NULL;

/* Iteration structures used for bookkeeping */
t_iterator * plot_iterator = NULL;
t_iterator * set_iterator = NULL;

/* Internal prototypes: */

static void convert(struct value *, int);
static void extend_at(void);
static union argument *add_action(enum operators sf_index);
static void parse_expression(void);
static void accept_logical_OR_expression(void);
static void accept_logical_AND_expression(void);
static void accept_inclusive_OR_expression(void);
static void accept_exclusive_OR_expression(void);
static void accept_AND_expression(void);
static void accept_equality_expression(void);
static void accept_relational_expression(void);
static void accept_bitshift_expression(void);
static void accept_additive_expression(void);
static void accept_multiplicative_expression(void);
static void parse_primary_expression(void);
static void parse_conditional_expression(void);
static void parse_logical_OR_expression(void);
static void parse_logical_AND_expression(void);
static void parse_inclusive_OR_expression(void);
static void parse_exclusive_OR_expression(void);
static void parse_AND_expression(void);
static void parse_equality_expression(void);
static void parse_relational_expression(void);
static void parse_bitshift_expression(void);
static void parse_additive_expression(void);
static void parse_multiplicative_expression(void);
static void parse_unary_expression(void);
static void parse_sum_expression(void);
static int  parse_assignment_expression(void);
static int  parse_array_assignment_expression(void);

static void set_up_columnheader_parsing(struct at_entry *previous );

static TBOOLEAN no_iteration(t_iterator *);
static void reevaluate_iteration_limits(t_iterator *iter);
static void reset_iteration(t_iterator *iter);

/* Internal variables: */

static struct at_type *at = NULL;
static int at_size = 0;

static void
convert(struct value *val_ptr, int t_num)
{
    *val_ptr = token[t_num].l_val;
}


intgr_t
int_expression()
{
    return (intgr_t)real_expression();
}

double
real_expression()
{
   double result;
   struct value a;
   result = real(const_express(&a));
   gpfree_string(&a);
   return result;
}


void
parse_reset_after_error()
{
    string_result_only = FALSE;
    parse_recursion_level = 0;
}

/* JW 20051126:
 * Wrapper around const_express() called by try_to_get_string().
 * Disallows top level + and - operators.
 * This enables things like set xtics ('-\pi' -pi, '-\pi/2' -pi/2.)
 */
struct value *
const_string_express(struct value *valptr)
{
    string_result_only = TRUE;
    const_express(valptr);
    string_result_only = FALSE;
    return (valptr);
}

struct value *
const_express(struct value *valptr)
{
    int tkn = c_token;

    if (END_OF_COMMAND)
	int_error(c_token, "constant expression required");

    /* div - no dummy variables in a constant expression */
    dummy_func = NULL;

    evaluate_at(temp_at(), valptr);	/* run it and send answer back */

    if (undefined) {
	int_error(tkn, "undefined value");
    }

    if (valptr->type == ARRAY) {
	/* Make sure no one tries to free it later */
	valptr->type = NOTDEFINED;
	int_error(NO_CARET, "const_express: unsupported array operation");
    }

    return (valptr);
}

/* Used by plot2d/plot3d/stats/fit:
 * Parse an expression that may return a filename string, a datablock name,
 * a constant, or a dummy function using dummy variables x, y, ...
 * If any dummy variables are present, set (*atptr) to point to an action table
 * corresponding to the parsed expression, and return NULL.
 * Otherwise evaluate the expression and return a string if there is one.
 * The return value "str" and "*atptr" both point to locally-managed memory,
 * which must not be freed by the caller!
 */
char*
string_or_express(struct at_type **atptr)
{
    int i;
    TBOOLEAN has_dummies;

    static char *array_placeholder = "@@";
    static char* str = NULL;
    free(str);
    str = NULL;
    df_array = NULL;

    if (atptr)
	*atptr = NULL;

    if (END_OF_COMMAND)
	int_error(c_token, "expression expected");

    /* parsing for datablocks */
    if (equals(c_token,"$"))
	return parse_datablock_name();

    /* special keywords */
    if (equals(c_token,"keyentry"))
	return NULL;

    if (isstring(c_token) && (str = try_to_get_string()))
	return str;

    /* If this is a bare array name for an existing array, store a pointer */
    /* for df_open() to use.  "@@" is a magic pseudo-filename passed to    */
    /* df_open() that tells it to use the stored pointer.                  */
    if (type_udv(c_token) == ARRAY && !equals(c_token+1, "[")) {
	df_array = add_udv(c_token++);
	return array_placeholder;
    }

    /* parse expression */
    temp_at();

    /* check if any dummy variables are used */
    has_dummies = FALSE;
    for (i = 0; i < at->a_count; i++) {
	enum operators op_index = at->actions[i].index;
	if ( op_index == PUSHD1 || op_index == PUSHD2 || op_index == PUSHD
	||   op_index == SUM ) {
	    has_dummies = TRUE;
	    break;
	}
    }

    if (!has_dummies) {
	/* no dummy variables: evaluate expression */
	struct value val;

	evaluate_at(at, &val);
	if (!undefined && val.type == STRING) {
	    /* prevent empty string variable from treated as special file '' or "" */
	    if (*val.v.string_val == '\0') {
		free(val.v.string_val);
		str = strdup(" ");
	    } else {
		str = val.v.string_val;
	    }
	}
    }

    /* prepare return */
    if (atptr)
	*atptr  = at;
    return str;
}


/* build an action table and return its pointer, but keep a pointer in at
 * so that we can free it later if the caller hasn't taken over management
 * of this table.
 */

struct at_type *
temp_at()
{
    if (at != NULL)
	free_at(at);
    
    at = (struct at_type *) gp_alloc(sizeof(struct at_type), "action table");

    memset(at, 0, sizeof(*at));		/* reset action table !!! */
    at_size = MAX_AT_LEN;

    parse_recursion_level = 0;
    parse_expression();
    return (at);
}


/* build an action table, put it in dynamic memory, and return its pointer */

struct at_type *
perm_at()
{
    struct at_type *at_ptr;
    size_t len;

    (void) temp_at();
    len = sizeof(struct at_type)
	+ (at->a_count - MAX_AT_LEN) * sizeof(struct at_entry);
    at_ptr = (struct at_type *) gp_realloc(at, len, "perm_at");
    at = NULL;			/* invalidate at pointer */
    return (at_ptr);
}

/* Create an action table that describes a call to column("string"). */
/* This is used by plot_option_using() to handle 'plot ... using "string"' */
struct at_type *
create_call_column_at(char *string)
{
    struct at_type *at = gp_alloc(sizeof(int) + 2*sizeof(struct at_entry),"");

    at->a_count = 2;
    at->actions[0].index = PUSHC;
    at->actions[0].arg.j_arg = 3;	/* FIXME - magic number! */
    at->actions[0].arg.v_arg.type = STRING;
    at->actions[0].arg.v_arg.v.string_val = string;
    at->actions[1].index = COLUMN;
    at->actions[1].arg.j_arg = 0;

    return (at);
}

/* Create an action table that describes a call to columnhead(-1). */
/* This is substituted for the bare keyword "colummhead" */
struct at_type *
create_call_columnhead()
{
    struct at_type *at = gp_alloc(sizeof(int) + 2*sizeof(struct at_entry),"");

    at->a_count = 2;
    at->actions[0].index = PUSHC;
    at->actions[0].arg.j_arg = 3;	/* FIXME - magic number! */
    at->actions[0].arg.v_arg.type = INTGR;
    at->actions[0].arg.v_arg.v.int_val = -1;
    at->actions[1].index = COLUMNHEAD;
    at->actions[1].arg.j_arg = 0;

    return (at);
}



static void
extend_at()
{
    size_t newsize = sizeof(struct at_type) + at_size * sizeof(struct at_entry);

    at = gp_realloc(at, newsize, "extend_at");
    at_size += MAX_AT_LEN;
    FPRINTF((stderr, "Extending at size to %d\n", at_size));
}

/* Add function number <sf_index> to the current action table */
static union argument *
add_action(enum operators sf_index)
{
    if (at->a_count >= at_size) {
	extend_at();
    }
    at->actions[at->a_count].index = sf_index;
    return (&(at->actions[at->a_count++].arg));
}


/* For external calls to parse_expressions() 
 * parse_recursion_level is expected to be 0 */
static void
parse_expression()
{				/* full expressions */

    if (parse_assignment_expression())
	return;

    parse_recursion_level++;
    accept_logical_OR_expression();
    parse_conditional_expression();
    parse_recursion_level--;
}

static void
accept_logical_OR_expression()
{				/* ? : expressions */
    accept_logical_AND_expression();
    parse_logical_OR_expression();
}


static void
accept_logical_AND_expression()
{
    accept_inclusive_OR_expression();
    parse_logical_AND_expression();
}


static void
accept_inclusive_OR_expression()
{
    accept_exclusive_OR_expression();
    parse_inclusive_OR_expression();
}


static void
accept_exclusive_OR_expression()
{
    accept_AND_expression();
    parse_exclusive_OR_expression();
}


static void
accept_AND_expression()
{
    accept_equality_expression();
    parse_AND_expression();
}


static void
accept_equality_expression()
{
    accept_relational_expression();
    parse_equality_expression();
}


static void
accept_relational_expression()
{
    accept_bitshift_expression();
    parse_relational_expression();
}


static void
accept_bitshift_expression()
{
    accept_additive_expression();
    parse_bitshift_expression();
}


static void
accept_additive_expression()
{
    accept_multiplicative_expression();
    parse_additive_expression();
}


static void
accept_multiplicative_expression()
{
    parse_unary_expression();			/* - things */
    parse_multiplicative_expression();			/* * / % */
}

static int
parse_assignment_expression()
{
    /* Check for assignment operator Var = <expr> */
    if (isletter(c_token) && equals(c_token + 1, "=")) {

	/* push the variable name */
	union argument *foo = add_action(PUSHC);
	char *varname = NULL;
	m_capture(&varname,c_token,c_token);
	foo->v_arg.type = STRING;
	foo->v_arg.v.string_val = varname;

	/* push a dummy variable that would be the index if this were an array */
	/* FIXME: It would be nice to hide this from "show at" */
	foo = add_action(PUSHC);
	foo->v_arg.type = NOTDEFINED;

	/* push the expression whose value it will get */
	c_token += 2;
	parse_expression();

	/* push the actual assignment operation */
	(void) add_action(ASSIGN);
	return 1;
    }

    /* Check for assignment to an array element Array[<expr>] = <expr> */
    if (parse_array_assignment_expression())
	return 1;

    return 0;
}

/*
 * If an array assignment is the first thing on a command line it is handled by
 * the separate routine array_assignment().
 * Here we catch assignments that are embedded in an expression.
 * Examples:
 *	print A[2] = foo
 *	A[1] = A[2] = A[3] = 0
 */
static int
parse_array_assignment_expression()
{
    /* Check for assignment to an array element Array[<expr>] = <expr> */
    if (isletter(c_token) && equals(c_token+1, "[")) {
	char *varname = NULL;
	union argument *foo;
	int save_action, save_token;

	/* Quick check for the most common false positives */
	/* i.e. other constructs that begin with "name["   */
	/* FIXME: quicker than the full test below, but do we care? */
	if (equals(c_token+3, "]") && !equals(c_token+4, "="))
	    return 0;
	if (equals(c_token+3, ":"))	/* substring s[foo:baz] */
	    return 0;

	/* Is this really a known array name? */
	if (type_udv(c_token) != ARRAY)
	    return 0;

	/* Save state of the action table and the command line */
	save_action = at->a_count;
	save_token = c_token;

	/* push the array name */
	m_capture(&varname,c_token,c_token);
	foo = add_action(PUSHC);
	foo->v_arg.type = STRING;
	foo->v_arg.v.string_val = varname;

	/* push the index */
	c_token += 2;
	parse_expression();

	/* If this wasn't really an array element assignment, back out. */
	/* NB: Depending on what we just parsed, this may leak memory.  */
	if (!equals(c_token, "]") || !equals(c_token+1, "=")) {
	    c_token = save_token;
	    at->a_count = save_action;
	    free(varname);
	    return 0;
	}

	/* Now we evaluate the expression whose value it will get */
	c_token += 2;
	parse_expression();

	/* push the actual assignment operation */
	(void) add_action(ASSIGN);
	return 1;
    }

    return 0;
}

/* add action table entries for primary expressions, i.e. either a
 * parenthesized expression, a variable name, a numeric constant, a
 * function evaluation, a power operator or postfix '!' (factorial)
 * expression.
 * Sep 2016 cardinality expression |Array| */
static void
parse_primary_expression()
{
    if (equals(c_token, "(")) {
	c_token++;
	parse_expression();

	/* Expressions may be separated by a comma */
	while (equals(c_token,",")) {
	    c_token++;
	    (void) add_action(POP);
	    parse_expression();
	}

	if (!equals(c_token, ")"))
	    int_error(c_token, "')' expected");
	c_token++;
    } else if (equals(c_token, "$")) {
	struct value a;

	c_token++;
	if (!isanumber(c_token)) {
	    if (equals(c_token+1, "[")) {
		struct udvt_entry *datablock_udv;
		c_token--;
		datablock_udv = get_udv_by_name(parse_datablock_name());
		if (!datablock_udv)
		    int_error(c_token-2,"No such datablock");
		add_action(PUSH)->udv_arg = datablock_udv;
	    } else
		int_error(c_token, "Column number or datablock line expected");
	} else {
	    convert(&a, c_token++);
	    if (a.type != INTGR || a.v.int_val < 0)
		int_error(c_token, "Positive integer expected");
	    if (at_highest_column_used < a.v.int_val)
		at_highest_column_used = a.v.int_val;
	    add_action(DOLLARS)->v_arg = a;
	}
    } else if (equals(c_token, "$#")) {
	struct value a = {INTGR, {DOLLAR_NCOLUMNS}};
	c_token++;
	add_action(DOLLARS)->v_arg = a;
    } else if (equals(c_token, "|")) {
	struct udvt_entry *udv;
	c_token++;
	if (equals(c_token,"$")) {
	    udv = get_udv_by_name(parse_datablock_name());
	    if (!udv)
		int_error(c_token-1, "no such datablock");
	} else {
	    udv = add_udv(c_token++);
	    if (udv->udv_value.type != ARRAY)
		int_error(c_token-1, "not an array");
	}
	add_action(PUSH)->udv_arg = udv;
	if (!equals(c_token, "|"))
	    int_error(c_token, "'|' expected");
	c_token++;
	add_action(CARDINALITY);
    } else if (isanumber(c_token)) {
	union argument *foo = add_action(PUSHC);
	convert(&(foo->v_arg), c_token);
	c_token++;
    } else if (isletter(c_token)) {
	/* Found an identifier --- check whether its a function or a
	 * variable by looking for the parentheses of a function
	 * argument list */
	if (equals(c_token + 1, "(")) {
	    enum operators whichfunc = is_builtin_function(c_token);
	    struct value num_params;
	    num_params.type = INTGR;

	    if (whichfunc) {
		c_token += 2;	/* skip fnc name and '(' */
		parse_expression(); /* parse fnc argument */
		num_params.v.int_val = 1;
		while (equals(c_token, ",")) {
		    c_token++;
		    num_params.v.int_val++;
		    parse_expression();
		}

		if (!equals(c_token, ")"))
		    int_error(c_token, "')' expected");
		c_token++;

		/* So far sprintf is the only built-in function */
		/* with a variable number of arguments.         */
		if (!strcmp(ft[whichfunc].f_name,"sprintf"))
		    add_action(PUSHC)->v_arg = num_params;

		/* v4 timecolumn only had 1 param; v5 has 2. Accept either */
		if (!strcmp(ft[whichfunc].f_name,"timecolumn"))
		    add_action(PUSHC)->v_arg = num_params;

		/* These functions have an optional 3rd parameter */
		if (!strncmp(ft[whichfunc].f_name,"weekdate_",9))
		    add_action(PUSHC)->v_arg = num_params;

		/* The column() function has side effects requiring special handling */
		if (!strcmp(ft[whichfunc].f_name,"column")) {
		    set_up_columnheader_parsing( &(at->actions[at->a_count-1]) );
		}

		(void) add_action(whichfunc);

	    } else {
		/* it's a call to a user-defined function */
		enum operators call_type = (int) CALL;
		int tok = c_token;

		c_token += 2;	/* skip func name and '(' */
		parse_expression();
		if (equals(c_token, ",")) { /* more than 1 argument? */
		    num_params.v.int_val = 1;
		    while (equals(c_token, ",")) {
			num_params.v.int_val += 1;
			c_token += 1;
			parse_expression();
		    }
		    add_action(PUSHC)->v_arg = num_params;
		    call_type = (int) CALLN;
		}
		if (!equals(c_token, ")"))
		    int_error(c_token, "')' expected");
		c_token++;
		add_action(call_type)->udf_arg = add_udf(tok);
	    }
	} else if (equals(c_token, "sum") && equals(c_token+1, "[")) {
	    parse_sum_expression();
	/* dummy_func==NULL is a flag to say no dummy variables active */
	} else if (dummy_func) {
	    if (equals(c_token, c_dummy_var[0])) {
		c_token++;
		add_action(PUSHD1)->udf_arg = dummy_func;
		fit_dummy_var[0]++;
	    } else if (equals(c_token, c_dummy_var[1])) {
		c_token++;
		add_action(PUSHD2)->udf_arg = dummy_func;
		fit_dummy_var[1]++;
	    } else {
		int i, param = 0;

		for (i = 2; i < MAX_NUM_VAR; i++) {
		    if (equals(c_token, c_dummy_var[i])) {
			struct value num_params;
			num_params.type = INTGR;
			num_params.v.int_val = i;
			param = 1;
			c_token++;
			add_action(PUSHC)->v_arg = num_params;
			add_action(PUSHD)->udf_arg = dummy_func;
			fit_dummy_var[i]++;
			break;
		    }
		}
		if (!param) {	/* defined variable */
		    add_action(PUSH)->udv_arg = add_udv(c_token);
		    c_token++;
		}
	    }
	    /* its a variable, with no dummies active - div */
	} else {
	    add_action(PUSH)->udv_arg = add_udv(c_token);
	    c_token++;
	}
    }
    /* end if letter */

    /* Maybe it's a string constant */
    else if (isstring(c_token)) {
	union argument *foo = add_action(PUSHC);
	foo->v_arg.type = STRING;
	foo->v_arg.v.string_val = NULL;
	/* this dynamically allocated string will be freed by free_at() */
	m_quote_capture(&(foo->v_arg.v.string_val), c_token, c_token);
	c_token++;
    } else {
	int_error(c_token, "invalid expression ");
    }

    /* The remaining operators are postfixes and can be stacked, e.g. */
    /* Array[i]**2, so we may have to loop to catch all of them.      */
    while (TRUE) {

	/* add action code for ! (factorial) operator */
	if (equals(c_token, "!")) {
	    c_token++;
	    (void) add_action(FACTORIAL);
	}

	/* add action code for ** operator */
	else if (equals(c_token, "**")) {
	    c_token++;
	    parse_unary_expression();
	    (void) add_action(POWER);
	}

	/* Parse and add actions for range specifier applying to previous entity.
	 * Currently the [beg:end] form is used to generate substrings, but could
	 * also be used to extract vector slices.  The [i] form is used to index
	 * arrays, but could also be a shorthand for extracting a single-character
	 * substring.
	 */
	else if (equals(c_token, "[") && !isanumber(c_token-1)) {
	    /* handle '*' or empty start of range */
	    if (equals(++c_token,"*") || equals(c_token,":")) {
		union argument *empty = add_action(PUSHC);
		empty->v_arg.type = INTGR;
		empty->v_arg.v.int_val = 1;
		if (equals(c_token,"*"))
		    c_token++;
	    } else
		parse_expression();

	    /* handle array indexing (single value in square brackets) */
	    if (equals(c_token, "]")) {
		c_token++;
		(void) add_action(INDEX);
		continue;
	    }

	    if (!equals(c_token, ":"))
		int_error(c_token, "':' expected");
	    /* handle '*' or empty end of range */
	    if (equals(++c_token,"*") || equals(c_token,"]")) {
		union argument *empty = add_action(PUSHC);
		empty->v_arg.type = INTGR;
		empty->v_arg.v.int_val = 65535; /* should be INT_MAX */
		if (equals(c_token,"*"))
		    c_token++;
	    } else
		parse_expression();
	    if (!equals(c_token, "]"))
		int_error(c_token, "']' expected");
	    c_token++;
	    (void) add_action(RANGE);

	/* Whatever this is, it isn't another postfix operator */
	} else {
	    break;
	}
    }
}


/* HBB 20010309: Here and below: can't store pointers into the middle
 * of at->actions[]. That array may be realloc()ed by add_action() or
 * express() calls!. Access via index savepc1/savepc2, instead. */

static void
parse_conditional_expression()
{
    /* create action code for ? : expressions */

    if (equals(c_token, "?")) {
	int savepc1, savepc2;

	/* Fake same recursion level for alternatives
	 *   set xlabel a>b ? 'foo' : 'bar' -1, 1
	 * FIXME: This won't work:
	 *   set xlabel a-b>c ? 'foo' : 'bar'  offset -1, 1
	 */
	parse_recursion_level--;

	c_token++;
	savepc1 = at->a_count;
	add_action(JTERN);
	parse_expression();
	if (!equals(c_token, ":"))
	    int_error(c_token, "expecting ':'");

	c_token++;
	savepc2 = at->a_count;
	add_action(JUMP);
	at->actions[savepc1].arg.j_arg = at->a_count - savepc1;
	parse_expression();
	at->actions[savepc2].arg.j_arg = at->a_count - savepc2;
	add_action(NOP);
	parse_recursion_level++;
    }
}


static void
parse_logical_OR_expression()
{
    /* create action codes for || operator */

    while (equals(c_token, "||")) {
	int savepc;

	c_token++;
	savepc = at->a_count;
	add_action(JUMPNZ);	/* short-circuit if already TRUE */
	accept_logical_AND_expression();
	/* offset for jump */
	at->actions[savepc].arg.j_arg = at->a_count - savepc;
	(void) add_action(BOOLE);
    }
}


static void
parse_logical_AND_expression()
{
    /* create action code for && operator */

    while (equals(c_token, "&&")) {
	int savepc;

	c_token++;
	savepc = at->a_count;
	add_action(JUMPZ);	/* short-circuit if already FALSE */
	accept_inclusive_OR_expression();
	at->actions[savepc].arg.j_arg = at->a_count - savepc; /* offset for jump */
	(void) add_action(BOOLE);
    }
}


static void
parse_inclusive_OR_expression()
{
    /* create action code for | operator */

    while (equals(c_token, "|")) {
	c_token++;
	accept_exclusive_OR_expression();
	(void) add_action(BOR);
    }
}


static void
parse_exclusive_OR_expression()
{
    /* create action code for ^ operator */

    while (equals(c_token, "^")) {
	c_token++;
	accept_AND_expression();
	(void) add_action(XOR);
    }
}


static void
parse_AND_expression()
{
    /* create action code for & operator */

    while (equals(c_token, "&")) {
	c_token++;
	accept_equality_expression();
	(void) add_action(BAND);
    }
}


static void
parse_equality_expression()
{
    /* create action codes for == and != numeric operators
     * eq and ne string operators */

    while (TRUE) {
	if (equals(c_token, "==")) {
	    c_token++;
	    accept_relational_expression();
	    (void) add_action(EQ);
	} else if (equals(c_token, "!=")) {
	    c_token++;
	    accept_relational_expression();
	    (void) add_action(NE);
	} else if (equals(c_token, "eq")) {
	    c_token++;
	    accept_relational_expression();
	    (void) add_action(EQS);
	} else if (equals(c_token, "ne")) {
	    c_token++;
	    accept_relational_expression();
	    (void) add_action(NES);
	} else
	    break;
    }
}


static void
parse_relational_expression()
{
    /* create action code for < > >= or <=
     * operators */

    while (TRUE) {
	if (equals(c_token, ">")) {
	    c_token++;
	    accept_bitshift_expression();
	    (void) add_action(GT);
	} else if (equals(c_token, "<")) {
	    /*  Workaround for * in syntax of range constraints  */
	    if (scanning_range_in_progress && equals(c_token+1, "*") ) {
		break;
	    }
	    c_token++;
	    accept_bitshift_expression();
	    (void) add_action(LT);
	} else if (equals(c_token, ">=")) {
	    c_token++;
	    accept_bitshift_expression();
	    (void) add_action(GE);
	} else if (equals(c_token, "<=")) {
	    c_token++;
	    accept_bitshift_expression();
	    (void) add_action(LE);
	} else
	    break;
    }

}



static void
parse_bitshift_expression()
{
    /* create action codes for << and >> operators */
    while (TRUE) {
	if (equals(c_token, "<<")) {
	    c_token++;
	    accept_additive_expression();
	    (void) add_action(LEFTSHIFT);
	} else if (equals(c_token, ">>")) {
	    c_token++;
	    accept_additive_expression();
	    (void) add_action(RIGHTSHIFT);
	} else
	    break;
    }
}



static void
parse_additive_expression()
{
    /* create action codes for +, - and . operators */
    while (TRUE) {
	if (equals(c_token, ".")) {
	    c_token++;
	    accept_multiplicative_expression();
	    (void) add_action(CONCATENATE);
	/* If only string results are wanted
	 * do not accept '-' or '+' at the top level. */
	} else if (string_result_only && parse_recursion_level == 1) {
	    break;
	} else if (equals(c_token, "+")) {
	    c_token++;
	    accept_multiplicative_expression();
	    (void) add_action(PLUS);
	} else if (equals(c_token, "-")) {
	    c_token++;
	    accept_multiplicative_expression();
	    (void) add_action(MINUS);
	} else
	    break;
    }
}


static void
parse_multiplicative_expression()
{
    /* add action code for * / and % operators */

    while (TRUE) {
	if (equals(c_token, "*")) {
	    c_token++;
	    parse_unary_expression();
	    (void) add_action(MULT);
	} else if (equals(c_token, "/")) {
	    c_token++;
	    parse_unary_expression();
	    (void) add_action(DIV);
	} else if (equals(c_token, "%")) {
	    c_token++;
	    parse_unary_expression();
	    (void) add_action(MOD);
	} else
	    break;
    }
}


static void
parse_unary_expression()
{
    /* add code for unary operators */

    if (equals(c_token, "!")) {
	c_token++;
	parse_unary_expression();
	(void) add_action(LNOT);
    } else if (equals(c_token, "~")) {
	c_token++;
	parse_unary_expression();
	(void) add_action(BNOT);
    } else if (equals(c_token, "-")) {
	struct at_entry *previous;
	c_token++;
	parse_unary_expression();
	/* Collapse two operations PUSHC <pos-const> + UMINUS
	 * into a single operation PUSHC <neg-const>
	 * Oct 2021: invalid if the previous constant is the else part of a conditional
	 *           NOP barrier hides the PUSHC to prevent this
	 */
	previous = &(at->actions[at->a_count-1]);
	if (previous->index == PUSHC) {
	    if (previous->arg.v_arg.type == INTGR) {
		previous->arg.v_arg.v.int_val = -previous->arg.v_arg.v.int_val;
	    } else if (previous->arg.v_arg.type == CMPLX) {
		previous->arg.v_arg.v.cmplx_val.real = -previous->arg.v_arg.v.cmplx_val.real;
		previous->arg.v_arg.v.cmplx_val.imag = -previous->arg.v_arg.v.cmplx_val.imag;
	    } else
		(void) add_action(UMINUS);
	} else
	    (void) add_action(UMINUS);
    } else if (equals(c_token, "+")) {	/* unary + is no-op */
	c_token++;
	parse_unary_expression();
    } else
	parse_primary_expression();
}


/*
 * Syntax: set link {x2|y2} {via <expression1> inverse <expression2>}
 * Create action code tables for the functions linking primary and secondary axes.
 * expression1 maps primary coordinates into the secondary coordinate space.
 * expression2 maps secondary coordinates into the primary coordinate space.
 */
void
parse_link_via( struct udft_entry *udf )
{
    int start_token;
    
    /* Caller left us pointing at "via" or "inverse" */
    c_token++;
    start_token = c_token;
    if (END_OF_COMMAND)
	int_error(c_token,"Missing expression");

    /* Save action table for the linkage mapping */
    dummy_func = udf;
    free_at(udf->at);
    udf->at = perm_at();
    dummy_func = NULL;

    /* Save the mapping expression itself */
    m_capture(&(udf->definition), start_token, c_token - 1);
}


/* create action code for 'sum' expressions */
static void
parse_sum_expression()
{
    /* sum [<var>=<start>:<end>] <expr>
     * - <var> pushed to stack *by name*
     * - <start> and <end> expressions pushed to stack
     * - A new action table for <expr> is created and passed to f_sum(arg)
     *   via arg->udf_arg
     */

    char *errormsg = "Expecting 'sum [<var> = <start>:<end>] <expression>'\n";
    char *varname = NULL;
    union argument *arg;
    struct udft_entry *udf;

    struct at_type * save_at;
    int save_at_size;
    int i;
    
    /* Caller already checked for string "sum [" so skip both tokens */
    c_token += 2;

    /* <var> */
    if (!isletter(c_token))
	int_error(c_token, errormsg);
    m_capture(&varname, c_token, c_token);
    arg = add_action(PUSHC);
    Gstring(&(arg->v_arg), varname);
    c_token++;

    if (!equals(c_token, "="))
	int_error(c_token, errormsg);
    c_token++;

    /* <start> */
    parse_expression();

    if (!equals(c_token, ":"))
	int_error(c_token, errormsg);
    c_token++;

    /* <end> */
    parse_expression();

    if (!equals(c_token, "]"))
	int_error(c_token, errormsg);
    c_token++;

    /* parse <expr> and convert it to a new action table.
     * modeled on code from temp_at().
     */
    /* 1. save environment to restart parsing */
    save_at = at;
    save_at_size = at_size;
    at = NULL;

    /* 2. save action table in a user defined function */
    udf = (struct udft_entry *) gp_alloc(sizeof(struct udft_entry), "sum");
    udf->next_udf = (struct udft_entry *) NULL;
    udf->udf_name = NULL; /* TODO maybe add a name and definition */ 
    udf->at = perm_at();
    udf->definition = NULL;
    udf->dummy_num = 0;
    for (i = 0; i < MAX_NUM_VAR; i++)
	Ginteger(&(udf->dummy_values[i]), 0);

    /* 3. restore environment */
    at = save_at;
    at_size = save_at_size;

    /* pass the udf to f_sum using the argument */
    add_action(SUM)->udf_arg = udf;
}


/* find or add value and return pointer */
struct udvt_entry *
add_udv(int t_num)
{
    char varname[MAX_ID_LEN+1];
    copy_str(varname, t_num, MAX_ID_LEN);
    if (token[t_num].length > MAX_ID_LEN-1)
	int_warn(t_num, "truncating variable name that is too long");
    return add_udv_by_name(varname);
}


/* find or add function at index <t_num>, and return pointer */
struct udft_entry *
add_udf(int t_num)
{
    struct udft_entry **udf_ptr = &first_udf;

    int i;
    while (*udf_ptr) {
	if (equals(t_num, (*udf_ptr)->udf_name))
	    return (*udf_ptr);
	udf_ptr = &((*udf_ptr)->next_udf);
    }

    /* get here => not found. udf_ptr points at first_udf or
     * next_udf field of last udf
     */

    if (is_builtin_function(t_num))
	int_warn(t_num, "Warning : udf shadowed by built-in function of the same name");

    /* create and return a new udf slot */

    *udf_ptr = (struct udft_entry *)
	gp_alloc(sizeof(struct udft_entry), "function");
    (*udf_ptr)->next_udf = (struct udft_entry *) NULL;
    (*udf_ptr)->definition = NULL;
    (*udf_ptr)->at = NULL;
    (*udf_ptr)->udf_name = gp_alloc (token_len(t_num)+1, "user func");
    copy_str((*udf_ptr)->udf_name, t_num, token_len(t_num)+1);
    for (i = 0; i < MAX_NUM_VAR; i++)
	(void) Ginteger(&((*udf_ptr)->dummy_values[i]), 0);
    return (*udf_ptr);
}

/* return standard function index or 0 */
int
is_builtin_function(int t_num)
{
    int i;

    for (i = (int) SF_START; ft[i].f_name != NULL; i++) {
	if (equals(t_num, ft[i].f_name))
	    return (i);
    }
    return (0);
}

/*
 * Test for the existence of a function without triggering errors
 * Return values:
 *   0  no such function is defined
 *  -1  built-in function
 *   1  user-defined function
 */
int
is_function(int t_num)
{
    struct udft_entry **udf_ptr = &first_udf;

    if (is_builtin_function(t_num))
	return -1;

    while (*udf_ptr) {
	if (equals(t_num, (*udf_ptr)->udf_name))
	    return 1;
	udf_ptr = &((*udf_ptr)->next_udf);
    }

    return 0;
}

/* Look for iterate-over-plot constructs, of the form
 *    for [<var> = <start> : <end> { : <increment>}] ...
 * If one (or more) is found, an iterator structure is allocated and filled
 * and a pointer to that structure is returned.
 * The pointer is NULL if no "for" statements are found.
 * If the iteration limits are constants, store them as is.
 * If they are given as expressions, store an action table for the expression.
 */
t_iterator *
check_for_iteration()
{
    char *errormsg = "Expecting iterator \tfor [<var> = <start> : <end> {: <incr>}]\n\t\t\tor\tfor [<var> in \"string of words\"]";
    int nesting_depth = 0;
    t_iterator *iter = NULL;
    t_iterator *prev = NULL;
    t_iterator *this_iter = NULL;
    TBOOLEAN no_parent = FALSE;

    /* Now checking for iteration parameters */
    /* Nested "for" statements are supported, each one corresponds to a node of the linked list */
    while (equals(c_token, "for")) {
	struct udvt_entry *iteration_udv = NULL;
	t_value original_udv_value;
	char *iteration_string = NULL;
	int iteration_start;
	int iteration_end;
	int iteration_increment = 1;
	int iteration_current;
	int iteration = 0;
	struct at_type *iteration_start_at = NULL;
	struct at_type *iteration_end_at = NULL;

	c_token++;
	if (!equals(c_token++, "[") || !isletter(c_token))
	    int_error(c_token-1, errormsg);
	iteration_udv = add_udv(c_token++);
	original_udv_value = iteration_udv->udv_value;
	iteration_udv->udv_value.type = NOTDEFINED;

	if (equals(c_token, "=")) {
	    c_token++;
	    if (isanumber(c_token) && equals(c_token+1,":")) {
		/* Save the constant value only */
		iteration_start = int_expression();
	    } else {
		/* Save the expression as well as the value */
		struct value v;
		iteration_start_at = perm_at();
		if (no_parent) {
		    iteration_start = 0;
		} else {
		    evaluate_at(iteration_start_at, &v);
		    iteration_start = real(&v);
		}
	    }
	    if (!equals(c_token++, ":"))
	    	int_error(c_token-1, errormsg);
	    if (equals(c_token,"*")) {
		iteration_end = INT_MAX;
		c_token++;
	    } else if (isanumber(c_token) && (equals(c_token+1,":") || equals(c_token+1,"]"))) {
		/* Save the constant value only */
		iteration_end = int_expression();
	    } else {
		/* Save the expression as well as the value */
		struct value v;
		iteration_end_at = perm_at();
		if (no_parent) {
		    iteration_end = 0;
		} else {
		    evaluate_at(iteration_end_at, &v);
		    iteration_end = real(&v);
		}
	    }
	    if (equals(c_token,":")) {
	    	c_token++;
	    	iteration_increment = int_expression();
		if (iteration_increment == 0)
		    int_error(c_token-1, errormsg);
	    }
	    if (!equals(c_token++, "]"))
	    	int_error(c_token-1, errormsg);
	    free_value(&(iteration_udv->udv_value));
	    Ginteger(&(iteration_udv->udv_value), iteration_start);
	}
	else if (equals(c_token++, "in")) {
	    /* Assume this is a string-valued expression. */
	    /* It might be worth treating a string constant as a special case */
	    struct value v;
	    iteration_start_at = perm_at();
	    evaluate_at(iteration_start_at, &v);
	    if (v.type != STRING)
	    	int_error(c_token-1, errormsg);
	    if (!equals(c_token++, "]"))
	    	int_error(c_token-1, errormsg);
	    iteration_string = v.v.string_val;
	    iteration_start = 1;
	    iteration_end = gp_words(iteration_string);
	    free_value(&(iteration_udv->udv_value));
	    Gstring(&(iteration_udv->udv_value), gp_word(iteration_string, 1));
	}
	else /* Neither [i=B:E] or [s in "foo"] */
	    int_error(c_token-1, errormsg);

	iteration_current = iteration_start;

	this_iter = gp_alloc(sizeof(t_iterator), "iteration linked list");
	this_iter->original_udv_value = original_udv_value;
	this_iter->iteration_udv = iteration_udv; 
	this_iter->iteration_string = iteration_string;
	this_iter->iteration_start = iteration_start;
	this_iter->iteration_end = iteration_end;
	this_iter->iteration_increment = iteration_increment;
	this_iter->iteration_current = iteration_current;
	this_iter->iteration = iteration;
	this_iter->start_at = iteration_start_at;
	this_iter->end_at = iteration_end_at;
	this_iter->next = NULL;

	if (nesting_depth == 0) {
	    /* first "for" statement: this will be the listhead */
	    iter = this_iter;
	} else {
	    /* nested "for": attach newly created node to the end of the list */
	    prev->next = this_iter;
	}
	prev = this_iter;

	/* If some depth of a nested iteration evaluates to an empty range, the
	 * evaluated limits of depths below it are moot (and possibly invalid).
	 * This flag tells us to skip their evaluation to avoid irrelevant errors.
	 */
	if (no_iteration(this_iter)) {
	    no_parent = TRUE;
	    FPRINTF((stderr,"iteration at level %d is moot\n", nesting_depth));
	}

	nesting_depth++;
    }

    return iter;
}

/*
 * Reevaluate the iteration limits
 * (in case they are functions whose parameters have taken 
 * on a new value)
 */
static void
reevaluate_iteration_limits(t_iterator *iter)
{
    if (iter->start_at) {
	struct value v;
	evaluate_at(iter->start_at, &v);
	if (iter->iteration_string) {
	    /* unnecessary if iteration string is a constant */
	    free(iter->iteration_string);
	    if (v.type != STRING)
		int_error(NO_CARET, "corrupt iteration string");
	    iter->iteration_string = v.v.string_val;
	    iter->iteration_start = 1;
	    iter->iteration_end = gp_words(iter->iteration_string);
	} else {
	    iter->iteration_start = real(&v);
	}
    }
    if (iter->end_at) {
	struct value v;
	evaluate_at(iter->end_at, &v);
	iter->iteration_end = real(&v);
    }
}

/*
 * Reset iteration at this level to start value.
 * Any iteration levels underneath are reset also.
 */
static void
reset_iteration(t_iterator *iter)
{
    if (!iter)
	return;

    reevaluate_iteration_limits(iter);
    iter->iteration = -1;
    iter->iteration_current = iter->iteration_start;
    if (iter->iteration_string) {
	gpfree_string(&(iter->iteration_udv->udv_value));
	Gstring(&(iter->iteration_udv->udv_value), 
		gp_word(iter->iteration_string, iter->iteration_current));
    } else {
	/* This traps fatal user error of reassigning iteration variable to a string */
	gpfree_string(&(iter->iteration_udv->udv_value));
	Ginteger(&(iter->iteration_udv->udv_value), iter->iteration_current);	
    }
    reset_iteration(iter->next);
}

/*
 * Increment the iteration position recursively.
 * returns TRUE if the iteration is still in range
 * returns FALSE if the incement put it past the end limit
 */
TBOOLEAN
next_iteration(t_iterator *iter)
{
    /* Once it goes out of range it will stay that way until reset */
    if (!iter || no_iteration(iter))
	return FALSE;

    /* Give sub-iterations a chance to advance */
    if (next_iteration(iter->next)) {
	if (iter->iteration < 0)
	    iter->iteration = 0;
	return TRUE;
    }

    /* Increment at this level */
    if (iter->iteration < 0) {
	/* Just reset, haven't used start value yet */
	iter->iteration = 0;
	if (!empty_iteration(iter))
	    return TRUE;
    } else {
	iter->iteration++;
	iter->iteration_current += iter->iteration_increment;
    }
    if (iter->iteration_string) {
	gpfree_string(&(iter->iteration_udv->udv_value));
	Gstring(&(iter->iteration_udv->udv_value), 
		gp_word(iter->iteration_string, iter->iteration_current));
    } else {
	/* This traps fatal user error of reassigning iteration variable to a string */
	gpfree_string(&(iter->iteration_udv->udv_value));
	Ginteger(&(iter->iteration_udv->udv_value), iter->iteration_current);	
    }
   
    /* If this runs off the end, leave the value out-of-range and return FALSE */ 
    if (iter->iteration_increment > 0 &&  iter->iteration_end - iter->iteration_current < 0)
	return FALSE;
    if (iter->iteration_increment < 0 &&  iter->iteration_end - iter->iteration_current > 0)
	return FALSE;

    if (iter->next == NULL)
	return TRUE;

    /* Reset sub-iterations, if any */
    reset_iteration(iter->next);

    /* Go back to top or call self recursively */
    return next_iteration(iter);
}

/*
 * Returns TRUE if
 * - this really is an iteration and
 * - the top level iteration covers no usable range
 */
static TBOOLEAN
no_iteration(t_iterator *iter)
{
    if (!iter)
	return FALSE;

    if ((iter->iteration_end > iter->iteration_start && iter->iteration_increment < 0)
    ||  (iter->iteration_end < iter->iteration_start && iter->iteration_increment > 0)) {
	return TRUE;
    }

    return FALSE;
}

/*
 * Recursive test that no empty iteration exists in a nested set of iterations
 */
TBOOLEAN
empty_iteration(t_iterator *iter)
{
    if (!iter)
	return FALSE;
    else if (no_iteration(iter))
	return TRUE;
    else
	return no_iteration(iter->next);
}

t_iterator *
cleanup_iteration(t_iterator *iter)
{
    while (iter) {
	t_iterator *next = iter->next;
	gpfree_string(&(iter->iteration_udv->udv_value));
	iter->iteration_udv->udv_value = iter->original_udv_value;
	free(iter->iteration_string);
	free_at(iter->start_at);
	free_at(iter->end_at);
	free(iter);
	iter = next;
    }
    return NULL;
}

TBOOLEAN
forever_iteration(t_iterator *iter)
{
    if (!iter)
	return FALSE;
    else
	return (iter->iteration_end == INT_MAX);
}

/* The column() function requires special handling because
 * - It has side effects if reference to a column entry
 *   requires matching it to the column header string.
 * - These side effects must be handled at the time the
 *   expression is parsed rather than when it it evaluated.
 */
static void
set_up_columnheader_parsing( struct at_entry *previous )
{
    /* column("string") means we expect the first row of */
    /* a data file to contain headers rather than data.  */
    if (previous->index == PUSHC &&  previous->arg.v_arg.type == STRING)
	parse_1st_row_as_headers = TRUE;

    /* This allows plot ... using (column(<const>)) title columnhead */
    if (previous->index == PUSHC && previous->arg.v_arg.type == INTGR) {
	if (at_highest_column_used < previous->arg.v_arg.v.int_val)
	    at_highest_column_used = previous->arg.v_arg.v.int_val;
    }
    
    /* This attempts to catch plot ... using (column(<variable>)) */
    if (previous->index == PUSH) {
	udvt_entry *u = previous->arg.udv_arg;
	if (u->udv_value.type == INTGR) {
	    if (at_highest_column_used < u->udv_value.v.int_val)
		at_highest_column_used = u->udv_value.v.int_val;
	}
    }

    /* NOTE: There is no way to handle ... using (column(<general expression>)) */
}
