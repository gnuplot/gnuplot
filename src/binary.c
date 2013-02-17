#ifndef lint
static char *RCSid() { return RCSid("$Id: binary.c,v 1.13 2007/10/02 18:18:57 sfeam Exp $"); }
#endif

/*
 * Last update:  3/29/92 memory allocation bugs fixed. jvdwoude@hut.nl
 *               3/09/92 spelling errors, general cleanup, use alloc with no
 *                       nasty fatal errors
 *               3/03/92 for Gnuplot 3.24.
 * Created from code for written by RKC for gnuplot 2.0b.
 *
 * Copyright (c) 1991,1992 Robert K. Cunningham, MIT Lincoln Laboratory
 *
 */

/* NOTE: These routines are not called from anywhere in gnuplot.
 * They are provided as a utility library for people wanting to 
 * write binary files usable by gnuplot as in the bf_test.c demo.
 */

#include "binary.h"

#include "alloc.h"
#include "util.h"

/* This routine scans the first block of the file to see if the file
 * is a binary file.  A file is considered binary if 10% of the
 * characters in it are not in the ascii character set. (values <
 * 128), or if a NUL is found.  I hope this doesn't break when used on
 * the bizzare PC's. */
int
is_binary_file(FILE *fp)
{
    int i, len;
    int odd;		/* Contains a count of the odd characters */
    long where;
    unsigned char *c;
    unsigned char buffer[512];

    if ((where = ftell(fp)) == -1) {	/* Find out where we start */
	fprintf(stderr, "Notice: Assuming unseekable data is not binary\n");
	return (FALSE);
    } else {
	rewind(fp);

	len = fread(buffer, sizeof(char), 512, fp);
	if (len <= 0)		/* Empty file is declared ascii */
	    return (FALSE);

	c = buffer;

	/* now scan buffer to look for odd characters */
	odd = 0;
	for (i = 0; i < len; i++, c++) {
	    if (!*c) {		/* NUL _never_ allowed in text */
		odd += len;
		break;
	    } else if ((*c & 128) ||	/* Meta-characters--we hope it's not formatting */
		       (*c == 127) ||	/* DEL */
		       (*c < 32 &&
			*c != '\n' && *c != '\r' && *c != '\b' &&
			*c != '\t' && *c != '\f' && *c != 27 /*ESC */ ))
		odd++;
	}

	fseek(fp, where, 0);	/* Go back to where we started */

	if (odd * 10 > len)	/* allow 10% of the characters to be odd */
	    return (TRUE);
	else
	    return (FALSE);
    }
}


/*========================= I/O Routines ================================
  These may be useful for situations other than just gnuplot.  Note that I
  have included the reading _and_ the writing routines, so others can create
  the file as well as read the file.
*/

#define START_ROWS 100		/* Each of these must be at least 1 */
#define ADD_ROWS 50


/* This function reads a matrix from a stream
 *
 * This routine never returns anything other than vectors and arrays
 * that range from 0 to some number. */
int
fread_matrix(
    FILE *fin,
    float GPFAR * GPFAR * GPFAR * ret_matrix,
    int *nr, int *nc,
    float GPFAR * GPFAR * row_title,
    float GPFAR * GPFAR * column_title)
{
    float GPFAR * GPFAR * m, GPFAR * rt, GPFAR * ct;
    int num_rows = START_ROWS;
    size_t num_cols;
    int current_row = 0;
    float GPFAR * GPFAR * temp_array;
    float fdummy;

    if (fread(&fdummy, sizeof(fdummy), 1, fin) != 1)
	return FALSE;

    num_cols = (size_t) fdummy;

    /* Choose a reasonable number of rows, allocate space for it and
     * continue until this space runs out, then extend the matrix as
     * necessary. */
    ct = alloc_vector(0, num_cols - 1);
    fread(ct, sizeof(*ct), num_cols, fin);

    rt = alloc_vector(0, num_rows - 1);
    m = matrix(0, num_rows - 1, 0, num_cols - 1);

    while (fread(&rt[current_row], sizeof(rt[current_row]), 1, fin) == 1) {
	/* We've got another row */
	if (fread(m[current_row], sizeof(*(m[current_row])), num_cols, fin) != num_cols)
	    return (FALSE);	/* Not a True matrix */

	current_row++;
	if (current_row >= num_rows) {	/* We've got to make a bigger rowsize */
	    temp_array = extend_matrix(m, 0, num_rows - 1, 0, num_cols - 1,
				       num_rows + ADD_ROWS - 1, num_cols - 1);
	    rt = extend_vector(rt, 0, num_rows + ADD_ROWS - 1);

	    num_rows += ADD_ROWS;
	    m = temp_array;
	}
    }
    /*  finally we force the matrix to be the correct row size */
    /*  bug fixed. procedure called with incorrect 6th argument.
     *   jvdwoude@hut.nl */
    temp_array = retract_matrix(m, 0, num_rows - 1, 0, num_cols - 1, current_row - 1, num_cols - 1);
    /* Now save the things that change */
    *ret_matrix = temp_array;
    *row_title = retract_vector(rt, 0, current_row - 1);
    *column_title = ct;
    *nr = current_row;		/* Really the total number of rows */
    *nc = num_cols;
    return (TRUE);
}

/* This writes a matrix to a stream
 *
 * Note that our ranges are inclusive ranges--and we can specify
 * subsets.  This behaves similarly to the xrange and yrange operators
 * in gnuplot that we all are familiar with.
 */
int
fwrite_matrix(
    FILE *fout,
    float GPFAR * GPFAR *m,
    int nrl, int nrh, int ncl, int nch,
    float GPFAR *row_title,
    float GPFAR *column_title)
{
    int j;
    float length;
    int col_length;
    int status;
    float GPFAR *title = NULL;

    length = (float) (col_length = nch - ncl + 1);

    if ((status = fwrite((char *) &length, sizeof(float), 1, fout)) != 1) {
	fprintf(stderr, "fwrite 1 returned %d\n", status);
	return (FALSE);
    }
    if (!column_title) {
	column_title = title = alloc_vector(ncl, nch);
	for (j = ncl; j <= nch; j++)
	    title[j] = (float) j;
    }
    fwrite((char *) column_title, sizeof(float), col_length, fout);
    if (title) {
	free_vector(title, ncl);
	title = NULL;
    }
    if (!row_title) {
	row_title = title = alloc_vector(nrl, nrh);
	for (j = nrl; j <= nrh; j++)
	    title[j] = (float) j;
    }
    for (j = nrl; j <= nrh; j++) {
	fwrite((char *) &row_title[j], sizeof(float), 1, fout);
	fwrite((char *) (m[j] + ncl), sizeof(float), col_length, fout);
    }
    if (title)
	free_vector(title, nrl);

    return (TRUE);
}

/*===================== Support routines ==============================*/

/* ******************************* VECTOR *******************************
 *       The following routines interact with vectors.
 *
 *   If there is an error we don't really return - int_error breaks us out.
 *
 *   This subroutine based on a subroutine listed in "Numerical Recipies in C",
 *   by Press, Flannery, Teukoilsky and Vetterling (1988).
 *
 */
float GPFAR *
alloc_vector(int nl, int nh)
{
    float GPFAR *vec;

    if (! (vec = gp_alloc((nh - nl + 1) * sizeof(float), NULL))) {
	int_error(NO_CARET, "not enough memory to create vector");
	return NULL;		/* Not reached */
    }
    return (vec - nl);
}


/*
 *  Free a vector allocated above
 *
 *   This subroutine based on a subroutine listed in "Numerical Recipies in C",
 *   by Press, Flannery, Teukoilsky and Vetterling (1988).
 *
 */
void
free_vector(float GPFAR *vec, int nl)
{
    free(vec + nl);
}

/************ Routines to modify the length of a vector ****************/
float GPFAR *
extend_vector(float GPFAR *vec, int old_nl, int new_nh)
{
    float GPFAR *new_v = gp_realloc((vec + old_nl),
				    (new_nh - old_nl + 1) * sizeof(new_v[0]),
				    "extend/retract vector");
    return new_v - old_nl;
}

float GPFAR *
retract_vector(float GPFAR *v, int old_nl, int new_nh)
{
    return extend_vector(v, old_nl, new_nh);
}


/* **************************** MATRIX ************************
 *
 * 	  The following routines work with matricies
 *
 * 	 I always get confused with this, so here I write it down:
 * 			  for nrl<= nri <=nrh and
 * 			  for ncl<= ncj <=nch
 *
 *   This matrix is accessed as:
 *
 *     matrix[nri][ncj];
 *     where nri is the offset to the pointer to a vector where the
 *     ncjth element lies.
 *
 *   If there is an error we don't really return - int_error breaks us out.
 *
 *   This subroutine based on a subroutine listed in "Numerical Recipies in C",
 *   by Press, Flannery, Teukoilsky and Vetterling (1988).
 *
 */
float GPFAR * GPFAR *
matrix(int nrl, int nrh, int ncl, int nch)
{
    int i;
    float GPFAR * GPFAR * m;

    m = gp_alloc((nrh - nrl + 1) * sizeof(m[0]), "matrix rows");
    m -= nrl;

    for (i = nrl; i <= nrh; i++) {
	if (!(m[i] = gp_alloc((nch - ncl + 1) * sizeof(m[i][0]), NULL))) {
	    free_matrix(m, nrl, i - 1, ncl);
	    int_error(NO_CARET, "not enough memory to create matrix");
	    return NULL;
	}
	m[i] -= ncl;
    }
    return m;
}

/*
 * Free a matrix allocated above
 *
 *
 *   This subroutine based on a subroutine listed in "Numerical Recipies in C",
 *   by Press, Flannery, Teukoilsky and Vetterling (1988).
 *
 */
void
free_matrix(
    float GPFAR * GPFAR * m,
    int nrl, int nrh, int ncl)
{
    int i;

    for (i = nrl; i <= nrh; i++)
	free(m[i] + ncl);
    free(m + nrl);
}

/* This routine takes a sub matrix and extends the number of rows and
 * columns for a new matrix */
float GPFAR * GPFAR *
extend_matrix(
    float GPFAR * GPFAR * a,
    int nrl, int nrh, int ncl, int nch,
    int srh, int sch)
{
    int i;
    float GPFAR * GPFAR * m;

    m = gp_realloc(a + nrl, (srh - nrl + 1) * sizeof(m[0]),
		    "extend matrix");
    m -= nrl;

    if (sch != nch) {
	for (i = nrl; i <= nrh; i++) {	/* Copy and extend rows */
	    if (!(m[i] = extend_vector(m[i], ncl, sch))) {
		free_matrix(m, nrl, nrh, ncl);
		int_error(NO_CARET, "not enough memory to extend matrix");
		return NULL;
	    }
	}
    }
    for (i = nrh + 1; i <= srh; i++) {
	if (!(m[i] = gp_alloc((nch - ncl + 1) * sizeof(m[i][0]), NULL))) {
	    free_matrix(m, nrl, i - 1, nrl);
	    int_error(NO_CARET, "not enough memory to extend matrix");
	    return NULL;
	}
	m[i] -= ncl;
    }
    return m;
}
/* this routine carves a large matrix down to size */
float GPFAR * GPFAR *
retract_matrix(
    float GPFAR * GPFAR * a,
    int nrl, int nrh, int ncl, int nch,
    int srh, int sch)
{
    int i;
    float GPFAR * GPFAR * m;

    for (i = srh + 1; i <= nrh; i++) {
	free_vector(a[i], ncl);
    }

    m = gp_realloc(a + nrl, (srh - nrl + 1) * sizeof(m[0]),
		   "retract matrix");
    m -= nrl;

    if (sch != nch) {
	for (i = nrl; i <= srh; i++)
	    if (!(m[i] = retract_vector(m[i], ncl, sch))) {
		/* Shrink rows */
		free_matrix(m, nrl, srh, ncl);
		int_error(NO_CARET, "not enough memory to retract matrix");
		return NULL;
	    }
    }
    return m;
}


/* allocate a float matrix m[nrl...nrh][ncl...nch] that points to the
   matrix declared in the standard C manner as a[nrow][ncol], where
   nrow=nrh-nrl+1, ncol=nch-ncl+1.  The routine should be called with
   the address &a[0][0] as the first argument.  This routine does
   not free the memory used by the original array a but merely assigns
   pointers to the rows. */

float GPFAR * GPFAR *
convert_matrix(
    float GPFAR *a,
    int nrl, int nrh, int ncl, int nch)
{
    int i, j, ncol, nrow;
    float GPFAR * GPFAR * m;

    nrow = nrh - nrl + 1;
    ncol = nch - ncl + 1;
    m = gp_alloc((nrh - nrl + 1) * sizeof(m[0]),
		 "convert_matrix");
    m -= nrl;

    m[nrl] = a - ncl;
    for (i = 1, j = nrl + 1; i <= nrow - 1; i++, j++)
	m[j] = m[j - 1] + ncol;
    return m;
}


void
free_convert_matrix(float GPFAR * GPFAR * b, int nrl)
{
    free(b + nrl);
}
