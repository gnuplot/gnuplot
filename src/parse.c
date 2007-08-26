#ifndef lint
static char *RCSid() { return RCSid("$Id: parse.c,v 1.49 2006/12/27 21:40:27 sfeam Exp $"); }
#endif

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
#include "eval.h"
#include "help.h"
#include "util.h"

/* protection mechanism for parsing string followed by + or - sign */
static int parse_recursion_level;
static TBOOLEAN string_result_only = FALSE;

/* Exported globals: the current 'dummy' variable names */
char c_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];
char set_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1] = { "x", "y" };

/* These are used by the iterate-over-plot code */
static struct udvt_entry *iteration_udv = NULL;
static int iteration_start = 0, iteration_end = 0;
static int iteration_increment = 1;
static int iteration_current = 0;

/* Internal prototypes: */

static void convert __PROTO((struct value *, int));
static void extend_at __PROTO((void));
static union argument *add_action __PROTO((enum operators sf_index));
static void parse_expression __PROTO((void));
static void accept_logical_OR_expression __PROTO((void));
static void accept_logical_AND_expression __PROTO((void));
static void accept_inclusive_OR_expression __PROTO((void));
static void accept_exclusive_OR_expression __PROTO((void));
static void accept_AND_expression __PROTO((void));
static void accept_equality_expression __PROTO((void));
static void accept_relational_expression __PROTO((void));
static void accept_additive_expression __PROTO((void));
static void accept_multiplicative_expression __PROTO((void));
static void parse_primary_expression __PROTO((void));
static void parse_conditional_expression __PROTO((void));
static void parse_logical_OR_expression __PROTO((void));
static void parse_logical_AND_expression __PROTO((void));
static void parse_inclusive_OR_expression __PROTO((void));
static void parse_exclusive_OR_expression __PROTO((void));
static void parse_AND_expression __PROTO((void));
static void parse_equality_expression __PROTO((void));
static void parse_relational_expression __PROTO((void));
static void parse_additive_expression __PROTO((void));
static void parse_multiplicative_expression __PROTO((void));
static void parse_unary_expression __PROTO((void));
static int is_builtin_function __PROTO((int t_num));

/* Internal variables: */

static struct at_type *at = NULL;
static int at_size = 0;

static void
convert(struct value *val_ptr, int t_num)
{
    *val_ptr = token[t_num].l_val;
}


int
int_expression()
{
    return (int)real_expression();
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
    return (valptr);
}

/* Used by plot2d/plot3d/fit:
 * Parse an expression that may return a string or may return a constant or may
 * be a dummy function using dummy variables x, y, ...
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

    static char* str = NULL;
    free(str);
    str = NULL;

    if (END_OF_COMMAND)
	int_error(c_token, "expression expected");

    if (isstring(c_token)) {
	if (atptr)
	    *atptr = NULL;
	str = try_to_get_string();
	return str;
    }

    /* parse expression */
    temp_at();

    /* check if any dummy variables are used */
    has_dummies = FALSE;
    for (i = 0; i < at->a_count; i++) {
	enum operators op_index = at->actions[i].index;
	if ( op_index == PUSHD1 || op_index == PUSHD2 || op_index == PUSHD ) {
	    has_dummies = TRUE;
	    break;
	}
    }

    if (!has_dummies) {
	/* no dummy variables: evaluate expression */
	struct value val;

	evaluate_at(at, &val);
	if (!undefined && val.type == STRING)
	    str = val.v.string_val;
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
    len = sizeof(struct at_type) +
     (at->a_count - MAX_AT_LEN) * sizeof(struct at_entry);
    at_ptr = (struct at_type *) gp_realloc(at, len, "perm_at");
    at = NULL;			/* invalidate at pointer */
    return (at_ptr);
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
    accept_additive_expression();
    parse_relational_expression();
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


/* add action table entries for primary expressions, i.e. either a
 * parenthesized expression, a variable names, a numeric constant, a
 * function evaluation, a power operator or postfix '!' (factorial)
 * expression */
static void
parse_primary_expression()
{
    if (equals(c_token, "(")) {
	c_token++;
	parse_expression();
	if (!equals(c_token, ")"))
	    int_error(c_token, "')' expected");
	c_token++;
    } else if (equals(c_token, "$")) {
	struct value a;

	if (!isanumber(++c_token))
	    int_error(c_token, "Column number expected");
	convert(&a, c_token++);
	if (a.type != INTGR || a.v.int_val < 0)
	    int_error(c_token, "Positive integer expected");
	add_action(DOLLARS)->v_arg = a;
    } else if (isanumber(c_token)) {
	/* work around HP 9000S/300 HP-UX 9.10 cc limitation ... */
	/* HBB 20010724: use this code for all platforms, then */
	union argument *foo = add_action(PUSHC);

	convert(&(foo->v_arg), c_token);
	c_token++;
    } else if (isletter(c_token)) {
	/* Found an identifier --- check whether its a function or a
	 * variable by looking for the parentheses of a function
	 * argument list */
	if ((c_token + 1 < num_tokens) && equals(c_token + 1, "(")) {
	    enum operators whichfunc = is_builtin_function(c_token);
	    struct value num_params;
	    num_params.type = INTGR;

	    if (whichfunc) {
#ifdef BACKWARDS_COMPATIBLE
		/* Deprecated syntax:   if (defined(foo)) ...  */
		/* New syntax:          if (exists("foo")) ... */
		if (strcmp(ft[whichfunc].f_name,"defined")==0) {
		    struct udvt_entry *udv = add_udv(c_token+2);
		    union argument *foo = add_action(PUSHC);
		    foo->v_arg.type = INTGR;
		    foo->v_arg.v.int_val = udv->udv_undef ? 0 : 1;
		    c_token += 4;  /* skip past "defined ( <foo> ) " */
		    return;
		}
#endif
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
		/* And "words(s)" is implemented as "word(s,-1)" */
		if (!strcmp(ft[whichfunc].f_name,"words")) {
		    num_params.v.int_val = -1;
		    add_action(PUSHC)->v_arg = num_params;
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
	    /* dummy_func==NULL is a flag to say no dummy variables active */
	} else if (dummy_func) {
	    if (equals(c_token, c_dummy_var[0])) {
		c_token++;
		add_action(PUSHD1)->udf_arg = dummy_func;
	    } else if (equals(c_token, c_dummy_var[1])) {
		c_token++;
		add_action(PUSHD2)->udf_arg = dummy_func;
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
    } else
	int_error(c_token, "invalid expression ");

    /* add action code for ! (factorial) operator */
    while (equals(c_token, "!")) {
	c_token++;
	(void) add_action(FACTORIAL);
    }
    /* add action code for ** operator */
    if (equals(c_token, "**")) {
	c_token++;
	parse_unary_expression();
	(void) add_action(POWER);
    }

    /* Parse and add actions for range specifier applying to previous entity.
     * Currently only used to generate substrings, but could also be used to
     * extract vector slices.
     */
    if (equals(c_token, "[")) {
	/* handle '*' or empty start of range */
	if (equals(++c_token,"*") || equals(c_token,":")) {
	    union argument *empty = add_action(PUSHC);
	    empty->v_arg.type = INTGR;
	    empty->v_arg.v.int_val = 1;
	    if (equals(c_token,"*"))
		c_token++;
	} else
	    parse_expression();
	if (!equals(c_token, ":"))
	    int_error(c_token, "':' expected");
	/* handle '*' or empty end of range */
	if (equals(++c_token,"*") || equals(c_token,"]")) {
	    union argument *empty = add_action(PUSHC);
	    empty->v_arg.type = INTGR;
	    empty->v_arg.v.int_val = 65535; /* should be MAXINT */
	    if (equals(c_token,"*"))
		c_token++;
	} else
	    parse_expression();
	if (!equals(c_token, "]"))
	    int_error(c_token, "']' expected");
	c_token++;
	(void) add_action(RANGE);
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
	/* I hate "else if" statements */
	if (equals(c_token, ">")) {
	    c_token++;
	    accept_additive_expression();
	    (void) add_action(GT);
	} else if (equals(c_token, "<")) {
	    c_token++;
	    accept_additive_expression();
	    (void) add_action(LT);
	} else if (equals(c_token, ">=")) {
	    c_token++;
	    accept_additive_expression();
	    (void) add_action(GE);
	} else if (equals(c_token, "<=")) {
	    c_token++;
	    accept_additive_expression();
	    (void) add_action(LE);
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
	} else if (string_result_only && parse_recursion_level == 1)
	    break;
	else if (equals(c_token, "+")) {
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
	c_token++;
	parse_unary_expression();
	(void) add_action(UMINUS);
    } else if (equals(c_token, "+")) {	/* unary + is no-op */
	c_token++;
	parse_unary_expression();
    } else
	parse_primary_expression();
}

/* find or add value and return pointer */
struct udvt_entry *
add_udv(int t_num)
{
    char varname[MAX_ID_LEN+1];
    copy_str(varname, t_num, MAX_ID_LEN);
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
static int
is_builtin_function(int t_num)
{
    int i;

    for (i = (int) SF_START; ft[i].f_name != NULL; i++) {
	if (equals(t_num, ft[i].f_name))
	    return (i);
    }
    return (0);
}

void
cleanup_udvlist()
{
    struct udvt_entry *udv_ptr = first_udv;
    struct udvt_entry *udv_dead;

    while (udv_ptr->next_udv) {
        if (udv_ptr->next_udv->udv_undef) {
	    udv_dead = udv_ptr->next_udv;
	    udv_ptr->next_udv = udv_dead->next_udv;
	    FPRINTF((stderr,"cleanup_udvlist: deleting %s\n",udv_dead->udv_name));
	    free(udv_dead->udv_name);
	    free(udv_dead);
	} else
	    udv_ptr = udv_ptr->next_udv;
    }
}

/* Look for an iterate-over-plot construct, of the form
 *    {s}plot  for [<var> = <start> : <end> { : <increment>}] ...
 */
void
check_for_iteration()
{
    iteration_udv = NULL;
    iteration_increment = 1;
    if (equals(c_token, "for")) {
	char *errormsg = "Expecting iterator of the form: for [<var> = <start> : <end>]";
	c_token++;
	if (!equals(c_token++, "["))
	    int_error(c_token, errormsg);
	iteration_udv = add_udv(c_token++);
	if (!equals(c_token++, "="))
	    int_error(c_token, errormsg);
	iteration_start = int_expression();
	if (!equals(c_token++, ":"))
	    int_error(c_token, errormsg);
	iteration_end = int_expression();
	if (equals(c_token,":")) {
	    c_token++;
	    iteration_increment = int_expression();
	    if (iteration_increment <= 0)
		iteration_increment = 1;
	}
	if (!equals(c_token++, "]"))
	    int_error(c_token, errormsg);
	if (iteration_udv->udv_undef == FALSE)
	    gpfree_string(&iteration_udv->udv_value);
	Ginteger(&(iteration_udv->udv_value), iteration_start);
	iteration_current = iteration_start;
	iteration_udv->udv_undef = FALSE;
    }
}

/* Set up next iteration.
 * Return TRUE if there is one, FALSE if we're done
 */
TBOOLEAN
next_iteration()
{
    if (!iteration_udv)
	return FALSE;
    iteration_current += iteration_increment;
    iteration_udv->udv_value.v.int_val = iteration_current;
    return (iteration_current <= iteration_end);
}

TBOOLEAN
empty_iteration()
{
    if (iteration_udv
        && (iteration_start > iteration_end))
        return TRUE;
    else
        return FALSE;
}
