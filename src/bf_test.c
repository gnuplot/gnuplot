#ifndef lint
static char *RCSid() { return RCSid("$Id: bf_test.c,v 1.9 2004/07/01 17:10:03 broeker Exp $"); }
#endif


/*
 * Test routines for binary files
 * cc bf_test.c -o bf_test binary_files.o -lm
 *
 * Copyright (c) 1992 Robert K. Cunningham, MIT Lincoln Laboratory
 *
 */

/* Note that this file is not compiled into gnuplot, and so
 * its more-restrictive copyright need not apply to gnuplot
 * as a whole. (I think.)
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define GPFAR /**/

#include "syscfg.h"
#include "stdfn.h"
#include "binary.h"

/* we declare these here instead of including more header files */
void int_error __PROTO((int, const char *));
void FreeHelp __PROTO((void));

/* static functions */
static float function __PROTO((int p, double x, double y));


typedef struct {
  float xmin, xmax;
  float ymin, ymax;
} range;

#define NUM_PLOTS 2
static range TheRange[] = {{-3,3,-2,2},
			   {-3,3,-3,3},
			   {-3,3,-3,3}}; /* Sampling rate causes this to go from -3:6*/

/*---- Stubs to make this work without including huge libraries ----*/
void
int_error(int dummy, const char *error_text)
{
    (void) dummy;		/* avoid -Wunused warning */
    fprintf(stderr, "Fatal error..\n%s\n...now exiting to system ...\n",
	    error_text);
    exit(EXIT_FAILURE);
}


void
FreeHelp()
{
}

/*---- End of stubs ----*/


static float
function(int p, double x, double y)
{
    float t = 0;			/* HBB 990828: initialize */

    switch (p) {
    case 0:
	t = 1.0 / (x * x + y * y + 1.0);
	break;
    case 1:
	t = sin(x * x + y * y) / (x * x + y * y);
	if (t > 1.0)
	    t = 1.0;
	break;
    case 2:
	t = sin(x * x + y * y) / (x * x + y * y);
	/* sinc modulated sinc */
	t *= sin(4. * (x * x + y * y)) / (4. * (x * x + y * y));
	if (t > 1.0)
	    t = 1.0;
	break;
    default:
	fprintf(stderr, "Unknown function\n");
	break;
    }
    return t;
}

#define ISOSAMPLES 5.0

int
main(void)
{
    int plot;
    int i, j;
    float x, y;
    float *rt, *ct;
    float **m;
    int xsize, ysize;
    char buf[256];
    FILE *fout;
/*  Create a few standard test interfaces */

    for (plot = 0; plot < NUM_PLOTS; plot++) {
	xsize = (TheRange[plot].xmax - TheRange[plot].xmin) * ISOSAMPLES + 1;
	ysize = (TheRange[plot].ymax - TheRange[plot].ymin) * ISOSAMPLES + 1;

	rt = alloc_vector(0, xsize - 1);
	ct = alloc_vector(0, ysize - 1);
	m = matrix(0, xsize - 1, 0, ysize - 1);

	for (y = TheRange[plot].ymin, j = 0; j < ysize; j++, y += 1.0 / (double) ISOSAMPLES) {
	    ct[j] = y;
	}

	for (x = TheRange[plot].xmin, i = 0; i < xsize; i++, x += 1.0 / (double) ISOSAMPLES) {
	    rt[i] = x;
	    for (y = TheRange[plot].ymin, j = 0; j < ysize; j++, y += 1.0 / (double) ISOSAMPLES) {
		m[i][j] = function(plot, x, y);
	    }
	}

	sprintf(buf, "binary%d", plot + 1);
	if (!(fout = fopen(buf, "wb")))
	    int_error(0, "Could not open file");
	else {
	    fwrite_matrix(fout, m, 0, xsize - 1, 0, ysize - 1, rt, ct);
	}
	free_vector(rt, 0);
	free_vector(ct, 0);
	free_matrix(m, 0, xsize - 1, 0);
    }

    /* Show that it's ok to vary sampling rate, as long as x1<x2, y1<y2... */
    xsize = (TheRange[plot].xmax - TheRange[plot].xmin) * ISOSAMPLES + 1;
    ysize = (TheRange[plot].ymax - TheRange[plot].ymin) * ISOSAMPLES + 1;

    rt = alloc_vector(0, xsize - 1);
    ct = alloc_vector(0, ysize - 1);
    m = matrix(0, xsize - 1, 0, ysize - 1);

    for (y = TheRange[plot].ymin, j = 0; j < ysize; j++, y += 1.0 / (double) ISOSAMPLES) {
	ct[j] = y > 0 ? 2 * y : y;
    }
    for (x = TheRange[plot].xmin, i = 0; i < xsize; i++, x += 1.0 / (double) ISOSAMPLES) {
	rt[i] = x > 0 ? 2 * x : x;
	for (y = TheRange[plot].ymin, j = 0; j < ysize; j++, y += 1.0 / (double) ISOSAMPLES) {
	    m[i][j] = function(plot, x, y);
	}
    }

    sprintf(buf, "binary%d", plot + 1);
    if (!(fout = fopen(buf, "wb")))
	int_error(0, "Could not open file");
    else {
	fwrite_matrix(fout, m, 0, xsize - 1, 0, ysize - 1, rt, ct);
    }
    free_vector(rt, 0);
    free_vector(ct, 0);
    free_matrix(m, 0, xsize - 1, 0);

    return EXIT_SUCCESS;
}
