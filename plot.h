/*
 *
 *    G N U P L O T  --  plot.h
 *
 *  Copyright (C) 1986, 1987  Thomas Williams, Colin Kelley
 *
 *  You may use this code as you wish if credit is given and this message
 *  is retained.
 *
 *  Please e-mail any useful additions to vu-vlsi!plot so they may be
 *  included in later releases.
 *
 *  This file should be edited with 4-column tabs!  (:set ts=4 sw=4 in vi)
 */

#define PROGRAM "G N U P L O T"
#define PROMPT "gnuplot> "
#define SHELL "/bin/csh"		/* used if SHELL env variable not set */

#ifdef vms
#define HELP  "gnuplot "
#else /* vms */
#define HELP  "/usr/local/bin/gnuphelp gnuplot"
#endif /* vms */

#define SAMPLES 160			/* default number of samples for a plot */
#define ZERO	1e-8		/* default for 'zero' set option */

#ifdef PC
#define TERM "egalib"
#else /* PC */
#define TERM "tek"		/* put your most common term type here! */
#endif /* PC */

#define TRUE 1
#define FALSE 0

#define Pi 3.141592653589793

#define MAX_POINTS 1
#define MAX_LINE_LEN 255	/* maximum number of chars allowed on line */
#define MAX_TOKENS 200
#define MAX_ID_LEN 20		/* max length of an identifier */

#define MAX_AT_LEN 150		/* max number of entries in action table */
#define STACK_DEPTH 100
#define NO_CARET (-1)


#ifdef vms
#define OS "VMS "
#endif

#ifdef unix
#define OS "unix "
#endif

#ifdef MSDOS
#define OS "MS-DOS "
#endif

#ifndef OS
#define OS ""
#endif

/*
 * note about HUGE:  this number is just used as a flag for really
 *   big numbers, so it doesn't have to be the absolutely biggest number
 *   on the machine.
 *
 * define HUGE here if your compiler doesn't define it in <math.h>
 * example:
#define HUGE 1e38
 */

#define END_OF_COMMAND (c_token >= num_tokens || equals(c_token,";"))


#ifdef vms

#define is_comment(c) ((c) == '#' || (c) == '!')
#define is_system(c) ((c) == '$')

#else /* vms */

#define is_comment(c) ((c) == '#')
#define is_system(c) ((c) == '!')

#endif /* vms */

/*
 * If you're memcpy() has another name, define it below as bcopy() is.
 * If you don't have a memcpy():
#define NOCOPY
 */

#ifdef BCOPY
#define memcpy(dest,src,len) bcopy(src,dest,len)
#endif /* BCOPY */

#ifdef vms
#define memcpy(dest,src,len) lib$movc3(&len,src,dest)
#endif /* vms */

#define top_of_stack stack[s_p]

typedef int BOOLEAN;
typedef int (*FUNC_PTR)();

enum operators {
	PUSH, PUSHC, PUSHD, CALL, LNOT, BNOT, UMINUS, LOR, LAND, BOR, XOR,
	BAND, EQ, NE, GT, LT, GE, LE, PLUS, MINUS, MULT, DIV, MOD, POWER,
	FACTORIAL, BOOL, JUMP, JUMPZ, JUMPNZ, JTERN, SF_START
};

#define is_jump(operator) ((operator) >=(int)JUMP && (operator) <(int)SF_START)

enum DATA_TYPES {
	INT, CMPLX
};

enum PLOT_TYPE {
	FUNC, DATA
};

enum PLOT_STYLE {
	LINES, POINTS, IMPULSES
};

struct cmplx {
	double real, imag;
};

struct value {
	enum DATA_TYPES type;
	union {
		int int_val;
		struct cmplx cmplx_val;
	} v;
};

struct lexical_unit {	/* produced by scanner */
	BOOLEAN is_token;	/* true if token, false if a value */ 
	struct value l_val;
	int start_index;	/* index of first char in token */
	int length;			/* length of token in chars */
};

struct ft_entry {		/* standard/internal function table entry */
	char *f_name;		/* pointer to name of this function */
	FUNC_PTR func;		/* address of function to call */
};

struct udft_entry {				/* user-defined function table entry */
	struct udft_entry *next_udf; /* pointer to next udf in linked list */
	char udf_name[MAX_ID_LEN+1]; /* name of this function entry */
	struct at_type *at;			/* pointer to action table to execute */
	char *definition; 			/* definition of function as typed */
	struct value dummy_value;	/* current value of dummy variable */
};

struct udvt_entry {			/* user-defined value table entry */
	struct udvt_entry *next_udv; /* pointer to next value in linked list */
	char udv_name[MAX_ID_LEN+1]; /* name of this value entry */
	BOOLEAN udv_undef;		/* true if not defined yet */
	struct value udv_value;	/* value it has */
};

union argument {			/* p-code argument */
	int j_arg;				/* offset for jump */
	struct value v_arg;		/* constant value */
	struct udvt_entry *udv_arg;	/* pointer to dummy variable */
	struct udft_entry *udf_arg; /* pointer to udf to execute */
};

struct at_entry {			/* action table entry */
	enum operators index;	/* index of p-code function */
	union argument arg;
};

struct at_type {
	int a_count;				/* count of entries in .actions[] */
	struct at_entry actions[MAX_AT_LEN];
		/* will usually be less than MAX_AT_LEN is malloc()'d copy */
};

struct coordinate {
	BOOLEAN undefined;	/* TRUE if value off screen */
#ifdef PC
	float x, y;			/* memory is tight on PCs! */
#else
	double x, y;
#endif /* PC */
};

struct curve_points {
	struct curve_points *next_cp;	/* pointer to next plot in linked list */
	enum PLOT_TYPE plot_type;
	enum PLOT_STYLE plot_style;
	char *title;
	int p_count;					/* count of points in .points[] */
	struct coordinate points[MAX_POINTS];
		/* will usually be less in malloc()'d copy */
};

struct termentry {
	char *name;
	unsigned int xmax,ymax,v_char,h_char,v_tic,h_tic;
	FUNC_PTR init,reset,text,graphics,move,vector,linetype,lrput_text,
		ulput_text,point;
};

/*
 * SS$_NORMAL is "normal completion", STS$M_INHIB_MSG supresses
 * printing a status message.
 * SS$_ABORT is the general abort status code.
 from:	Martin Minow
	decvax!minow
 */
#ifdef	vms
#include		<ssdef.h>
#include		<stsdef.h>
#define	IO_SUCCESS	(SS$_NORMAL | STS$M_INHIB_MSG)
#define	IO_ERROR	SS$_ABORT
#endif /* vms */

#ifndef	IO_SUCCESS	/* DECUS or VMS C will have defined these already */
#define	IO_SUCCESS	0
#endif
#ifndef	IO_ERROR
#define	IO_ERROR	1
#endif
