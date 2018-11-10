/*
 * DATA_TYPES and struct value must match definitions in
 * #include <gp_types.h>
 */
#include <inttypes.h>		/* C99 type definitions */
enum DATA_TYPES {
	INTGR=1,
	CMPLX,
	STRING,
	DATABLOCK,
	ARRAY,
	NOTDEFINED,	/* exists, but value is currently undefined */
	INVALID_VALUE,	/* used only for error return by external functions */
	INVALID_NAME	/* used only to trap errors in linked axis function definition */
};

struct gp_cmplx {
	double real, imag;
};

typedef struct value {
    enum DATA_TYPES type;
    union {
	int64_t int_val;
	struct gp_cmplx cmplx_val;
	char *string_val;
	char **data_array;
	struct value *value_array;
    } v;
} t_value;

/*
 * End of definitions from gp_types.h
 */



#ifdef _WIN32
# define DLLEXPORT __declspec(dllexport)
# define __inline__ __inline
#else
# define DLLEXPORT
#endif

/* prototype for a plugin function */
DLLEXPORT void *gnuplot_init(struct value(*)(int, struct value *, void *));
DLLEXPORT void gnuplot_fini(void *);

/* Check that the number of parameters declared in the gnuplot import
 * statement matches the actual number of parameters in the exported
 * function
 */
#define RETURN_ERROR_IF_WRONG_NARGS(r, nargs, ACTUAL_NARGS) \
    if (nargs != ACTUAL_NARGS) { \
	r.type = INVALID_VALUE;  \
	return r; \
    }

#define RETURN_ERROR_IF_NONNUMERIC(r, arg) \
    if (arg.type != CMPLX && arg.type != INTGR) { \
	r.type = INVALID_VALUE; \
	return r; \
    }

__inline__ static double RVAL(struct value v)
{
  if (v.type == CMPLX)
    return v.v.cmplx_val.real;
  else if (v.type == INTGR)
    return (double) v.v.int_val;
  else
    return 0;	/* precluded by sanity check above */
}

__inline__ static int IVAL(struct value v)
{
  if (v.type == CMPLX)
    return (int)v.v.cmplx_val.real;
  else if (v.type == INTGR)
    return v.v.int_val;
  else
    return 0;	/* precluded by sanity check above */
}

#undef NaN
