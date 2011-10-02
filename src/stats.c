#ifndef lint
static char *RCSid() { return RCSid("$Id: stats.c,v 1.26 2009/11/15 18:34:44 v923z Exp $"); }
#endif

/* GNUPLOT - stats.c */

/*
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
 */

#include "gp_types.h"
#ifdef USE_STATS  /* Only compile this if configured with --enable-stats */

#include "alloc.h"
#include "axis.h"
#include "command.h"
#include "datafile.h"
#include "gadgets.h"  /* For polar and parametric flags */
#include "matrix.h"   /* For vector allocation */
#include "variable.h" /* For locale handling */

#include "stats.h"

#define INITIAL_DATA_SIZE (4096)   /* initial size of data arrays */
#define UNDEF (-1)                 /* needed to parse command line */

static int comparator __PROTO(( const void *a, const void *b ));
static struct file_stats analyze_file __PROTO(( long n, int outofrange, int invalid, int blank, int dblblank ));
static struct sgl_column_stats analyze_sgl_column __PROTO(( double *data, long n, long nr ));
static struct two_column_stats analyze_two_columns __PROTO(( double *x, double *y,
			      struct sgl_column_stats res_x,
			      struct sgl_column_stats res_y,
			      long n ));

static void ensure_output __PROTO((void));
static char* fmt __PROTO(( char *buf, double val ));
static void sgl_column_output_nonformat __PROTO(( struct sgl_column_stats s, char *x ));
static void file_output __PROTO(( struct file_stats s ));
static void sgl_column_output __PROTO(( struct sgl_column_stats s, long n ));
static void two_column_output __PROTO(( struct sgl_column_stats x,
					struct sgl_column_stats y,
					struct two_column_stats xy, long n));

static void create_and_set_var __PROTO(( double val, char *prefix, 
					 char *base, char *suffix ));

static void sgl_column_variables __PROTO(( struct sgl_column_stats res,
					   char *prefix, char *postfix ));

static TBOOLEAN validate_data __PROTO((double v, AXIS_INDEX ax));

void statsrequest __PROTO((void));

/* =================================================================
   Data Structures
   ================================================================= */

/* Keeps info on a value and its index in the file */
struct pair { 
    double val;
    long index;
};

/* Collect results from analysis */
struct file_stats {
    long records;
    long blanks;
    long invalid;
    long outofrange;
    long blocks;  /* blocks are separated by double blank lines */
};

struct sgl_column_stats {
    /* Matrix dimensions */
    int sx;
    int sy;

    double mean;
    double stddev;

    double sum;        /* sum x    */
    double sum_sq;       /* sum x**2 */

    struct pair min;
    struct pair max;
    
    double median;
    double lower_quartile;
    double upper_quartile;
    
    double cog_x;   /* centre of gravity */
    double cog_y; 
    
    /* info on data points out of bounds? */
};

struct two_column_stats {
    double sum_xy;

    double slope;        /* linear regression */
    double intercept;

    double slope_err;
    double intercept_err;

    double correlation; 

    double pos_min_y;	/* x coordinate of min y */
    double pos_max_y;	/* x coordinate of max y */
};

/* =================================================================
   Analysis and Output
   ================================================================= */

/* Needed by qsort() when we sort the data points to find the median.   */
/* FIXME: I am dubious that keeping the original index gains anything. */
/* It makes no sense at all for quartiles,  and even the min/max are not  */
/* guaranteed to be unique.                                               */
static int 
comparator( const void *a, const void *b )
{
    struct pair *x = (struct pair *)a;
    struct pair *y = (struct pair *)b;

    if ( x->val < y->val ) return -1;
    if ( x->val > y->val ) return 1;
    return 0;
}

static struct file_stats 
analyze_file( long n, int outofrange, int invalid, int blank, int dblblank )
{
    struct file_stats res;

    res.records = n;
    res.invalid = invalid;
    res.blanks  = blank;
    res.blocks  = dblblank + 1;  /* blocks are separated by dbl blank lines */
    res.outofrange = outofrange;

    return res;
}

static struct sgl_column_stats 
analyze_sgl_column( double *data, long n, long nr )
{
    struct sgl_column_stats res;

    long i;

    double s = 0.0;
    double s2 = 0.0;
    double cx = 0.0;
    double cy = 0.0;

    struct pair *tmp = (struct pair *)gp_alloc( n*sizeof(struct pair),
					      "analyze_sgl_column" );
     
    if ( nr > 0 ) {
	res.sx = nr;
	res.sy = n / nr;
    } else {
	res.sx = 0;
	res.sy = n;
    }
  
    /* Mean and Std Dev and centre of gravity */
    for( i=0; i<n; i++ ) {
	s  += data[i];
	s2 += data[i]*data[i];
	if ( nr > 0 ) {
	    cx += data[i]*(i % res.sx);
	    cy += data[i]*(i / res.sx);
	}
    }
    res.mean = s/(double)n;
    res.stddev = sqrt( s2/(double)n - res.mean*res.mean );

    res.sum  = s;
    res.sum_sq = s2;

    for( i=0; i<n; i++ ) {
	tmp[i].val = data[i];
	tmp[i].index = i;
    }
    qsort( tmp, n, sizeof(struct pair), comparator );
  
    res.min = tmp[0];
    res.max = tmp[n-1];

    /*
     * This uses the same quartile definitions as the boxplot code in graphics.c
     */
    if ((n & 0x1) == 0)
	res.median = 0.5 * (tmp[n/2 - 1].val + tmp[n/2].val);
    else
	res.median = tmp[(n-1)/2].val;
    if ((n & 0x3) == 0)
	res.lower_quartile = 0.5 * (tmp[n/4 - 1].val + tmp[n/4].val);
    else
	res.lower_quartile = tmp[(n+3)/4 - 1].val;
    if ((n & 0x3) == 0)
	res.upper_quartile = 0.5 * (tmp[n - n/4].val + tmp[n - n/4 - 1].val);
    else
	res.upper_quartile = tmp[n - (n+3)/4].val;

    /* Note: the centre of gravity makes sense for positive value matrices only */
    if ( cx == 0.0 && cy == 0.0) {
	res.cog_x = 0.0;
	res.cog_y = 0.0;
    } else {
	res.cog_x = cx / s;
	res.cog_y = cy / s;
    }

    free(tmp);

    return res;
}

static struct two_column_stats 
analyze_two_columns( double *x, double *y,
		     struct sgl_column_stats res_x,
		     struct sgl_column_stats res_y,
		     long n )
{
    struct two_column_stats res;

    long i;
    double s = 0;
  
    for( i=0; i<n; i++ ) {
	s += x[i]*y[i];
    }
    res.sum_xy = s;

    res.slope = res.sum_xy - res_x.sum*res_y.sum/n;
    res.slope /= res_x.sum_sq - (res_x.sum)*(res_x.sum)/n;

    res.intercept = res_y.mean - res.slope * res_x.mean;

    res.correlation = res.slope * res_x.stddev/res_y.stddev;

    res.pos_min_y = x[res_y.min.index];
    res.pos_max_y = x[res_y.max.index];

    return res;
}

/* =================================================================
   Output
   ================================================================= */

/* Output */
/* Note: print_out is a FILE ptr, set by the "set print" command */

static void 
ensure_output()
{
    if (!print_out)
	print_out = stderr;
}

static char* 
fmt( char *buf, double val )
{
    if ( fabs(val) < 1e-14 )
	sprintf( buf, "%11.4f", 0.0 );
    else if ( fabs(log10(fabs(val))) < 6 )
	sprintf( buf, "%11.4f", val );
    else
	sprintf( buf, "%11.5e", val );
    return buf;
}

static void 
file_output( struct file_stats s )
{
    int width = 3;

    /* Assuming that records is the largest number of the four... */
    if ( s.records > 0 )
	width = 1 + (int)( log10((double)s.records) );

    ensure_output();

    /* Non-formatted to disk */
    if ( print_out != stdout && print_out != stderr ) {
	fprintf( print_out, "%s\t%ld\n", "records", s.records );
	fprintf( print_out, "%s\t%ld\n", "invalid", s.invalid );
	fprintf( print_out, "%s\t%ld\n", "blanks", s.blanks );
	fprintf( print_out, "%s\t%ld\n", "blocks", s.blocks );
	fprintf( print_out, "%s\t%ld\n", "outofrange", s.outofrange );
	return;
    }

    /* Formatted to screen */
    fprintf( print_out, "\n" );
    fprintf( print_out, "* FILE: \n" );
    fprintf( print_out, "  Records:      %*ld\n", width, s.records );
    fprintf( print_out, "  Out of range: %*ld\n", width, s.outofrange );
    fprintf( print_out, "  Invalid:      %*ld\n", width, s.invalid );
    fprintf( print_out, "  Blank:        %*ld\n", width, s.blanks );
    fprintf( print_out, "  Data Blocks:  %*ld\n", width, s.blocks );
}

static void 
sgl_column_output_nonformat( struct sgl_column_stats s, char *x )
{
    fprintf( print_out, "%s%s\t%f\n", "mean",   x, s.mean );
    fprintf( print_out, "%s%s\t%f\n", "stddev", x, s.stddev );
    fprintf( print_out, "%s%s\t%f\n", "sum",   x, s.sum );
    fprintf( print_out, "%s%s\t%f\n", "sum_sq",  x, s.sum_sq );
  
    fprintf( print_out, "%s%s\t%f\n", "min",     x, s.min.val );
    if ( s.sx == 0 ) {
	fprintf( print_out, "%s%s\t%f\n", "lo_quartile", x, s.lower_quartile );
	fprintf( print_out, "%s%s\t%f\n", "median",      x, s.median );
	fprintf( print_out, "%s%s\t%f\n", "up_quartile", x, s.upper_quartile );
    }
    fprintf( print_out, "%s%s\t%f\n", "max",     x, s.max.val );

    /* If data set is matrix */
    if ( s.sx > 0 ) {
	fprintf( print_out, "%s%s\t%ld\n","index_min_x",  x, (s.min.index) / s.sx );
	fprintf( print_out, "%s%s\t%ld\n","index_min_y",  x, (s.min.index) % s.sx );
	fprintf( print_out, "%s%s\t%ld\n","index_max_x",  x, (s.max.index) / s.sx );
	fprintf( print_out, "%s%s\t%ld\n","index_max_y",  x, (s.max.index) % s.sx );
	fprintf( print_out, "%s%s\t%f\n","cog_x",  x, s.cog_x );
	fprintf( print_out, "%s%s\t%f\n","cog_y",  x, s.cog_y );
    } else {
	fprintf( print_out, "%s%s\t%ld\n","min_index",  x, s.min.index );
	fprintf( print_out, "%s%s\t%ld\n","max_index",  x, s.max.index );  
    }
}

static void 
sgl_column_output( struct sgl_column_stats s, long n )
{
    int width = 1;
    char buf[32];
    char buf2[32];

    if ( n > 0 )
	width = 1 + (int)( log10( (double)n ) );

    ensure_output();

    /* Non-formatted to disk */
    if ( print_out != stdout && print_out != stderr ) {
	sgl_column_output_nonformat( s, "_y" );
	return;
    }

    /* Formatted to screen */
    fprintf( print_out, "\n" );
  
    /* First, we check whether the data file was a matrix */
     if ( s.sx > 0) 
	 fprintf( print_out, "* MATRIX: [%d X %d] \n", s.sx, s.sy );
     else 
	 fprintf( print_out, "* COLUMN: \n" );

    fprintf( print_out, "  Mean:     %s\n", fmt( buf, s.mean ) );
    fprintf( print_out, "  Std Dev:  %s\n", fmt( buf, s.stddev ) );
    fprintf( print_out, "  Sum:      %s\n", fmt( buf, s.sum ) );
    fprintf( print_out, "  Sum Sq.:  %s\n", fmt( buf, s.sum_sq ) );
    fprintf( print_out, "\n" );

    /* For matrices, the quartiles and the median do not make too much sense */
    if ( s.sx > 0 ) {
	fprintf( print_out, "  Minimum:  %s [%*ld %ld ]\n", fmt(buf, s.min.val), width, 
	     (s.min.index) / s.sx, (s.min.index) % s.sx);
	fprintf( print_out, "  Maximum:  %s [%*ld %ld ]\n", fmt(buf, s.max.val), width, 
	     (s.max.index) / s.sx, (s.max.index) % s.sx);
	fprintf( print_out, "  COG:      %s %s\n", fmt(buf, s.cog_x), fmt(buf2, s.cog_y) );
    } else {
	/* FIXME:  The "position" are randomly selected from a non-unique set. Bad! */
	fprintf( print_out, "  Minimum:  %s [%*ld]\n", fmt(buf, s.min.val), width, s.min.index );
	fprintf( print_out, "  Maximum:  %s [%*ld]\n", fmt(buf, s.max.val), width, s.max.index );
	fprintf( print_out, "  Quartile: %s \n", fmt(buf, s.lower_quartile) );
	fprintf( print_out, "  Median:   %s \n", fmt(buf, s.median) );
	fprintf( print_out, "  Quartile: %s \n", fmt(buf, s.upper_quartile) );
	fprintf( print_out, "\n" );
    }
}

static void 
two_column_output( struct sgl_column_stats x,
			struct sgl_column_stats y,
			struct two_column_stats xy,
			long n )
{
    int width = 1;
    char bfx[32];
    char bfy[32];
    char blank[32];

    if ( n > 0 )
	width = 1 + (int)( log10( (double)n ) );

    /* Non-formatted to disk */
    if ( print_out != stdout && print_out != stderr ) {
	sgl_column_output_nonformat( x, "_x" );
	sgl_column_output_nonformat( y, "_y" );

	fprintf( print_out, "%s\t%f\n", "slope", xy.slope );
	fprintf( print_out, "%s\t%f\n", "intercept", xy.intercept );
	fprintf( print_out, "%s\t%f\n", "correlation", xy.correlation );
	fprintf( print_out, "%s\t%f\n", "sumxy", xy.sum_xy );
	return;
    }

    /* Create a string of blanks of the required length */
    strncpy( blank, "                 ", width+4 );
    blank[width+4] = '\0';

    ensure_output();

    fprintf( print_out, "\n" );
    fprintf( print_out, "* COLUMNS:\n" );
    fprintf( print_out, "  Mean:     %s %s %s\n", fmt(bfx, x.mean),   blank, fmt(bfy, y.mean) );
    fprintf( print_out, "  Std Dev:  %s %s %s\n", fmt(bfx, x.stddev), blank, fmt(bfy, y.stddev ) );
    fprintf( print_out, "  Sum:      %s %s %s\n", fmt(bfx, x.sum),  blank, fmt(bfy, y.sum) );
    fprintf( print_out, "  Sum Sq.:  %s %s %s\n", fmt(bfx, x.sum_sq), blank, fmt(bfy, y.sum_sq ) );
    fprintf( print_out, "\n" );

    /* FIXME:  The "positions" are randomly selected from a non-unique set.  Bad! */
    fprintf( print_out, "  Minimum:  %s [%*ld]   %s [%*ld]\n", fmt(bfx, x.min.val),
    	width, x.min.index, fmt(bfy, y.min.val), width, y.min.index );
    fprintf( print_out, "  Maximum:  %s [%*ld]   %s [%*ld]\n", fmt(bfx, x.max.val),
    	width, x.max.index, fmt(bfy, y.max.val), width, y.max.index );

    fprintf( print_out, "  Quartile: %s %s %s\n",
	fmt(bfx, x.lower_quartile), blank, fmt(bfy, y.lower_quartile) );
    fprintf( print_out, "  Median:   %s %s %s\n",
	fmt(bfx, x.median), blank, fmt(bfy, y.median) );
    fprintf( print_out, "  Quartile: %s %s %s\n",
	fmt(bfx, x.upper_quartile), blank, fmt(bfy, y.upper_quartile) );
    fprintf( print_out, "\n" );

    /* Simpler below - don't care about alignment */
    if ( xy.intercept < 0.0 ) 
	fprintf( print_out, "  Linear Model: y = %.4g x - %.4g\n", xy.slope, -xy.intercept );
    else
	fprintf( print_out, "  Linear Model: y = %.4g x + %.4g\n", xy.slope, xy.intercept );
  
    fprintf( print_out, "  Correlation:  r = %.4g\n", xy.correlation );
    fprintf( print_out, "  Sum xy:       %.4g\n", xy.sum_xy );
    fprintf( print_out, "\n" );
}

/* =================================================================
   Variable Handling
   ================================================================= */

static void 
create_and_set_var( double val, char *prefix, char *base, char *suffix )
{
    int len;
    char *varname;
    struct udvt_entry *udv_ptr;

    t_value data;
    Gcomplex( &data, val, 0.0 ); /* data is complex, real=val, imag=0.0 */

    /* In case prefix (or suffix) is NULL - make them empty strings */
    prefix = prefix ? prefix : "";
    suffix = suffix ? suffix : "";

    len = strlen(prefix) + strlen(base) + strlen(suffix) + 1;
    varname = (char *)gp_alloc( len, "create_and_set_var" );
    sprintf( varname, "%s%s%s", prefix, base, suffix );

    /* Note that add_udv_by_name() checks if the name already exists, and
     * returns the existing ptr if found. It also allocates memory for
     * its own copy of the varname.
     */
    udv_ptr = add_udv_by_name(varname);
    udv_ptr->udv_value = data;
    udv_ptr->udv_undef = FALSE;

    free( varname );
}

static void 
file_variables( struct file_stats s, char *prefix )
{
    /* Suffix does not make sense here! */
    create_and_set_var( s.records, prefix, "records", "" );
    create_and_set_var( s.invalid, prefix, "invalid", "" );
    create_and_set_var( s.blanks,  prefix, "blank",   "" );
    create_and_set_var( s.blocks,  prefix, "blocks",  "" );
    create_and_set_var( s.outofrange, prefix, "outofrange", "" );
}

static void 
sgl_column_variables( struct sgl_column_stats s, char *prefix, char *suffix )
{
    create_and_set_var( s.mean,   prefix, "mean",   suffix );
    create_and_set_var( s.stddev, prefix, "stddev", suffix );

    create_and_set_var( s.sum,  prefix, "sum",   suffix );
    create_and_set_var( s.sum_sq, prefix, "sumsq",  suffix );

    create_and_set_var( s.min.val, prefix, "min", suffix );
    create_and_set_var( s.max.val, prefix, "max", suffix );
  
    /* If data set is matrix */
    if ( s.sx > 0 ) {
	create_and_set_var( (s.min.index) / s.sx, prefix, "index_min_x", suffix );
	create_and_set_var( (s.min.index) % s.sx, prefix, "index_min_y", suffix );
	create_and_set_var( (s.max.index) / s.sx, prefix, "index_max_x", suffix );
	create_and_set_var( (s.max.index) % s.sx, prefix, "index_max_y", suffix );
    } else {
	create_and_set_var( s.median,         prefix, "median",      suffix );
	create_and_set_var( s.lower_quartile, prefix, "lo_quartile", suffix );
	create_and_set_var( s.upper_quartile, prefix, "up_quartile", suffix );
	create_and_set_var( s.min.index, prefix, "index_min", suffix );
	create_and_set_var( s.max.index, prefix, "index_max", suffix );
    }
}

static void 
two_column_variables( struct two_column_stats s, char *prefix)
{
    /* Suffix does not make sense here! */
    create_and_set_var( s.slope,       prefix, "slope",       "" );
    create_and_set_var( s.intercept,   prefix, "intercept",   "" );
    create_and_set_var( s.correlation, prefix, "correlation", "" );
    create_and_set_var( s.sum_xy,      prefix, "sumxy",  "" );

    create_and_set_var( s.pos_min_y,   prefix, "pos_min_y", "" );
    create_and_set_var( s.pos_max_y,   prefix, "pos_max_y", "" );
}

/* =================================================================
   Range Handling
   ================================================================= */

/* We validate our data here: discard everything that is outside 
 * the specified range. However, we have to be a bit careful here, 
 * because if no range is specified, we keep everything 
 */
static TBOOLEAN validate_data(double v, AXIS_INDEX ax)
{
    /* These are flag bits, not constants!!! */
    if ((axis_array[ax].autoscale & AUTOSCALE_BOTH) == AUTOSCALE_BOTH)
	return TRUE;
    if (((axis_array[ax].autoscale & AUTOSCALE_BOTH) == AUTOSCALE_MIN)
    &&  (v <= axis_array[ax].max))
	return TRUE;
    if (((axis_array[ax].autoscale & AUTOSCALE_BOTH) == AUTOSCALE_MAX)
    &&  (v >= axis_array[ax].min))
	return TRUE;
    if (((axis_array[ax].autoscale & AUTOSCALE_BOTH) == AUTOSCALE_NONE)
	 && ((v <= axis_array[ax].max) && (v >= axis_array[ax].min))) 
	return(TRUE);

    return(FALSE);
}
   
/* =================================================================
   Parse Command Line and Process
   ================================================================= */

void 
statsrequest(void)
{ 
    int i;
    int columns;
    int columnsread;
    double v[2];
    static char *file_name = NULL;

    /* Vars to hold data and results */
    long n;                /* number of records retained */
    long max_n;

    static double *data_x = NULL;
    static double *data_y = NULL;   /* values read from file */
    long invalid;          /* number of missing/invalid records */
    long blanks;           /* number of blank lines */
    long doubleblanks;     /* number of repeated blank lines */
    long out_of_range;     /* number pts rejected, because out of range */

    struct file_stats res_file;
    struct sgl_column_stats res_x, res_y;
    struct two_column_stats res_xy;
    
    float *matrix;            /* matrix data. This must be float. */
    int nc, nr;               /* matrix dimensions. */
    int index;

    /* Vars for variable handling */
    static char *prefix = NULL;       /* prefix for user-defined vars names */

    /* Vars that control output */
    int do_output;     /* Generate formatted output */ 
    
    c_token++;

    /* Parse ranges */
    AXIS_INIT2D(FIRST_X_AXIS, 0);
    AXIS_INIT2D(FIRST_Y_AXIS, 0);
    PARSE_RANGE(FIRST_X_AXIS);
    PARSE_RANGE(FIRST_Y_AXIS);

    /* Initialize */
    columnsread = 2;
    invalid = 0;          /* number of missing/invalid records */
    blanks = 0;           /* number of blank lines */
    doubleblanks = 0;     /* number of repeated blank lines */
    out_of_range = 0;     /* number pts rejected, because out of range */
    n = 0;                /* number of records retained */
    nr = 0;               /* Matrix dimensions */
    nc = 0;
    max_n = INITIAL_DATA_SIZE;
    
    do_output = UNDEF;
    
    free(data_x);
    free(data_y);
    data_x = vec(max_n);       /* start with max. value */
    data_y = vec(max_n);

    if ( !data_x || !data_y )
      int_error( NO_CARET, "Internal error: out of memory in stats" );

    n = invalid = blanks = doubleblanks = out_of_range = nr = 0;

    /* Get filename */
    free ( file_name );
    file_name = try_to_get_string();

    if ( !file_name )
	int_error(c_token, "missing filename");

    /* ===========================================================
    v923z: insertion for treating matrices 
      EAM: only handles ascii matrix with uniform grid,
           and fails to apply any input data transforms
      =========================================================== */
    if ( almost_equals(c_token, "mat$rix") ) {
	df_open(file_name, 3, NULL);
	index = df_num_bin_records - 1;
	
	/* We take these values as set by df_determine_matrix_info
	See line 1996 in datafile.c */
	nc = df_bin_record[index].scan_dim[0];
	nr = df_bin_record[index].scan_dim[1];
	n = nc * nr;
	
	matrix = (float *)df_bin_record[index].memory_data;
	
	/* Fill up a vector, so that we can use the existing code. */
	if ( !redim_vec(&data_x, n ) ) {
	    int_error( NO_CARET, 
		   "Out of memory in stats: too many datapoints (%d)?", n );
	}
	for( i=0; i < n; i++ ) {
	    data_y[i] = (double)matrix[i];  
	}
	/* We can close the file here, there is nothing else to do */
	df_close();
	/* We will invoke single column statistics for the matrix */
	columns = 1;

    } else { /* Not a matrix */
	columns = df_open(file_name, 2, NULL); /* up to 2 using specs allowed */

	if (columns < 0)
	    int_error(NO_CARET, "Can't read data file"); 

	if (columns > 2 )
	    int_error(c_token, "Need 0 to 2 using specs for stats command");

	/* If the user has set an explicit locale for numeric input, apply it
	   here so that it affects data fields read from the input file. */
	/* v923z: where exactly should this be? here or before the matrix case? 
	 * I think, we should move everything here to before trying to open the file. 
	 * There is no point in trying to read anything, if the axis is logarithmic, e.g.
	 */
	set_numeric_locale();   

	/* For all these below: we could save the state, switch off, then restore */
	if ( axis_array[FIRST_X_AXIS].log || axis_array[FIRST_Y_AXIS].log )
	    int_error( NO_CARET, "Stats command not available with logscale");

	if ( axis_array[FIRST_X_AXIS].is_timedata || axis_array[FIRST_Y_AXIS].is_timedata )
	    int_error( NO_CARET, "Stats command not available with timedata mode");

	if ( polar )
	    int_error( NO_CARET, "Stats command not availble in polar mode" );

	if ( parametric )
	    int_error( NO_CARET, "Stats command not availble in parametric mode" );

	/* The way readline and friends work is as follows:
	 - df_open will return the number of columns requested in the using spec
	   so that "columns" will be 0, 1, or 2 (no using, using 1, using 1:2)
	 - readline always returns the same number of columns (for us: 1 or 2)
	 - using 1:2 = return two columns, skipping lines w/ bad data
	 - using 1   = return sgl column (supply zeros (0) for the second col)
	 - no using  = return two columns (first two), fail on bad data 

	 We need to know how many columns to process. If columns==1 or ==2
	 (that is, if there was a using spec), all is clear and we use the
	 value of columns. 
	 But: if columns is 0, then we need to figure out the number of cols
	 read from the return value of readline. If readline ever returns
	 1, we take that; only if it always returns 2 do we assume two cols.
	 */
  
	while( (i = df_readline(v, 2)) != DF_EOF ) {
	    columnsread = ( i > columnsread ? i : columnsread );

	    if ( n >= max_n ) {
		max_n = (max_n * 3) / 2; /* increase max_n by factor of 1.5 */
	  
		/* Some of the reallocations went bad: */
		if ( 0 || !redim_vec(&data_x, max_n) || !redim_vec(&data_y, max_n) ) {
		    df_close();
		    int_error( NO_CARET, 
		       "Out of memory in stats: too many datapoints (%d)?",
		       max_n );
		} 
	    } /* if (need to extend storage space) */

	    switch (i) {
	    case DF_MISSING:
	    case DF_UNDEFINED:
	      /* Invalids are only recognized if the syntax is like this:
		     stats "file" using ($1):($2)
		 If the syntax is simply:
		     stats "file" using 1:2
		 then df_readline simply skips invalid records (does not
		 return anything!) Status: 2009-11-02 */
	      invalid += 1;
	      continue;
	      
	    case DF_FIRST_BLANK: 
	      blanks += 1;
	      continue;

	    case DF_SECOND_BLANK:      
	      blanks += 1;
	      doubleblanks += 1;
	      continue;
	      
	    case 0:
	      int_error( NO_CARET,
			 "bad data on line %d of datafile", df_line_number);
	      break;

	    case 1: /* Read single column successfully  */
	      if ( validate_data(v[0], FIRST_Y_AXIS) )  {
		data_y[n] = v[0];
		n++;
	      } else {
		out_of_range++;
	      }
	      break;

	    case 2: /* Read two columns successfully  */
	      if ( validate_data(v[0], FIRST_X_AXIS) &&
		  validate_data(v[1], FIRST_Y_AXIS) ) {
		data_x[n] = v[0];
		data_y[n] = v[1];
		n++;
	      } else {
		out_of_range++;
	      }
	      break;
	    }
	} /* end-while : done reading file */
	df_close();

	/* now resize fields to actual length: */
	redim_vec(&data_x, n);
	redim_vec(&data_y, n);

	/* figure out how many columns where really read... */
	if ( columns == 0 )
	    columns = columnsread;

    }  /* end of case when the data file is not a matrix */

    /* Now finished reading user input; return to C locale for internal use*/
    reset_numeric_locale();

    /* PKJ - TODO: similar for logscale, polar/parametric, timedata */

    /* No data! Try to explain why. */
    if ( n == 0 ) {
	if ( out_of_range > 0 )
	    int_error( NO_CARET, "All points out of range" );
	else 
	    int_error( NO_CARET, "No valid data points found in file" );
    }

    /* Parse the remainder of the command line: 0 to 2 tokens possible */
    while( !(END_OF_COMMAND) ) {
	if ( do_output != UNDEF ) {
	    int_error( c_token, "';' expected" );

	} else if ( almost_equals( c_token, "out$put" ) ) {
	    if ( do_output == UNDEF )
		do_output = 1;
	    else
		int_error( c_token, "Only one output allowed" );

	} else if ( almost_equals( c_token, "noout$put" ) ) {
	    if ( do_output == UNDEF )
		do_output = 0;
	    else
		int_error( c_token, "Only one [no]output allowed" );

	} else if ( almost_equals(c_token, "pre$fix") 
	       ||   equals(c_token, "name")) {
	    c_token++;
	    free ( prefix );
	    prefix = try_to_get_string();
	    if ( !strcmp ( "GPVAL_", prefix ) )
		int_error( c_token, "GPVAL_ is forbidden as a prefix" );

	}  else {
	    int_error( c_token, "Expecting [no]output or prefix");
	}

	c_token++;
    }

    /* Set defaults if not explicitly set by user */
    do_output = ( do_output == UNDEF ? 1 : do_output ); /* YES by default */
    if (!prefix)
	prefix = gp_strdup("STATS_");
    i = strlen(prefix);
    if (prefix[i-1] != '_') {
	prefix = gp_realloc(prefix, i+2, "prefix");
	strcat(prefix,"_");
    }

    /* Do the actual analysis */
    res_file = analyze_file( n, out_of_range, invalid, blanks, doubleblanks );
    if ( columns == 1 ) {
	res_y = analyze_sgl_column( data_y, n, nr );
    }

    if ( columns == 2 ) {
	/* If there are two columns, then the data file is not a matrix */
	res_x = analyze_sgl_column( data_x, n, 0 );
	res_y = analyze_sgl_column( data_y, n, 0 );
	res_xy = analyze_two_columns( data_x, data_y, res_x, res_y, n );
    }

    /* Store results in user-accessible variables */
    /* Clear out any previous use of these variables */
    del_udv_by_name( prefix, TRUE );

    file_variables( res_file, prefix );

    if ( columns == 1 ) {
	sgl_column_variables( res_y, prefix, "" );
    }
    
    if ( columns == 2 ) {
	sgl_column_variables( res_x, prefix, "_x" );
	sgl_column_variables( res_y, prefix, "_y" );
	two_column_variables( res_xy, prefix );
    }

    /* Output */
    if ( do_output ) {
	file_output( res_file );
	if ( columns == 1 )
	    sgl_column_output( res_y, res_file.records );
	else
	    two_column_output( res_x, res_y, res_xy, res_file.records );
    }

    /* Cleanup */
      
    free(data_x);
    free(data_y);

    data_x = NULL;
    data_y = NULL;
    
    free( file_name );
    file_name = NULL;
    
    free( prefix );
    prefix = NULL;
}
#endif /* The whole file is conditional on USE_STATS */
