#ifndef lint
static char *RCSid() { return RCSid("$Id: parse.c,v 1.26 2004/09/11 17:46:02 sfeam Exp $"); }
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

/* Exported globals: the current 'dummy' variable names */
char c_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1];
char set_dummy_var[MAX_NUM_VAR][MAX_ID_LEN+1] = { "x", "y" };

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

/* isvar - When this variable is true PUSH operations become PUSHV */
static TBOOLEAN push_vars = TRUE;

static void
convert(struct value *val_ptr, int t_num)
{
    *val_ptr = token[t_num].l_val;
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


/* if dummy_dunc == NULL on entry, do not attempt to compile dummy variables
 * - div
 */
struct at_type *
temp_at()
{
    /* build a static action table and return its
     * pointer */

    if (at != NULL) {
	free(at);
	at = NULL;
    }
    at = (struct at_type *) gp_alloc(sizeof(struct at_type), "action table");

    at->a_count = 0;		/* reset action table !!! */
    at_size = MAX_AT_LEN;
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


static void
parse_expression()
{				/* full expressions */
    accept_logical_OR_expression();
    parse_conditional_expression();
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
#ifdef GP_ISVAR
		/* Check to see if it is isvar */
		/* Is so then turn off normal variable pushing */
                /* Push variable definition state instead */
		if (strcmp(ft[whichfunc].f_name,"defined")==0) {
			push_vars=FALSE;
		}
#endif  /*GP_ISVAR*/

		/* it's a standard function */
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

#ifdef GP_STRING_VARS
		/* So far sprintf is the only built-in function */
		/* with a variable number of arguments.         */
		if (!strcmp(ft[whichfunc].f_name,"sprintf"))
		    add_action(PUSHC)->v_arg = num_params;
#endif
		(void) add_action(whichfunc);

		 /* Turn normal variable pushing back on */
		push_vars=TRUE;
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
            if (push_vars==FALSE)
		add_action(PUSHV)->udv_arg = add_udv(c_token);
	    else
		add_action(PUSH)->udv_arg = add_udv(c_token);

	    c_token++;
	}
    }
    /* end if letter */

#ifdef GP_STRING_VARS
    /* Maybe it's a string constant */
    else if (isstring(c_token)) {
	union argument *foo = add_action(PUSHC);
	foo->v_arg.type = STRING;
	foo->v_arg.v.string_val = NULL;
	m_quote_capture(&(foo->v_arg.v.string_val), c_token, c_token);
	c_token++;
    }
#endif

    else
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
#ifdef GP_STRING_VARS
	} else if (equals(c_token, "eq")) {
	    c_token++;
	    accept_relational_expression();
	    (void) add_action(EQS);
	} else if (equals(c_token, "ne")) {
	    c_token++;
	    accept_relational_expression();
	    (void) add_action(NES);
#endif
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
    /* create action codes for + and - operators */

    while (TRUE) {
	if (equals(c_token, "+")) {
	    c_token++;
	    accept_multiplicative_expression();
	    (void) add_action(PLUS);
	} else if (equals(c_token, "-")) {
	    c_token++;
	    accept_multiplicative_expression();
	    (void) add_action(MINUS);
#ifdef GP_STRING_VARS
	} else if (equals(c_token, ".")) {
	    c_token++;
	    accept_multiplicative_expression();
	    (void) add_action(CONCATENATE);
#endif
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
    struct udvt_entry **udv_ptr = &first_udv;

    /* check if it's already in the table... */

    while (*udv_ptr) {
	if (equals(t_num, (*udv_ptr)->udv_name))
	    return (*udv_ptr);
	udv_ptr = &((*udv_ptr)->next_udv);
    }

    *udv_ptr = (struct udvt_entry *)
	gp_alloc(sizeof(struct udvt_entry), "value");
    (*udv_ptr)->next_udv = NULL;
    (*udv_ptr)->udv_name = gp_alloc (token_len(t_num)+1, "user var");
    copy_str((*udv_ptr)->udv_name, t_num, token_len(t_num)+1);
    (*udv_ptr)->udv_value.type = INTGR;		/* not necessary, but safe! */
    (*udv_ptr)->udv_undef = TRUE;
    return (*udv_ptr);
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

/* EAM July 2003 - add_udv_by_name() is like add_udv except that it does not
 * require that the udv key be a token in the current command line. */
struct udvt_entry *
add_udv_by_name(char *key)
{
    struct udvt_entry **udv_ptr = &first_udv;

    /* check if it's already in the table... */

    while (*udv_ptr) {
	if (!strcmp(key, (*udv_ptr)->udv_name))
	    return (*udv_ptr);
	udv_ptr = &((*udv_ptr)->next_udv);
    }

    *udv_ptr = (struct udvt_entry *)
	gp_alloc(sizeof(struct udvt_entry), "value");
    (*udv_ptr)->next_udv = NULL;
    (*udv_ptr)->udv_name = gp_strdup(key);
    (*udv_ptr)->udv_undef = TRUE;
    return (*udv_ptr);
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
