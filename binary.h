/*
 * $Id: binary.h,v 1.3 1996/12/08 13:07:54 drd Exp $
 *
 */

/* Copied from command.c -- this should be put in a shared macro file */
#ifndef inrange
#define inrange(z,min,max) ((min<max) ? ((z>=min)&&(z<=max)) : ((z>=max)&&(z<=min)) )
#endif

/* Routines for interfacing with command.c */
float GPFAR *vector __PROTO(( int nl, int nh));
float GPFAR *extend_vector __PROTO((float GPFAR *vec, int old_nl, int old_nh, int new_nh));
float GPFAR *retract_vector __PROTO((float GPFAR *v, int old_nl, int old_nh, int new_nh));
float GPFAR * GPFAR *matrix __PROTO(( int nrl, int nrh, int ncl, int nch));
float GPFAR * GPFAR *extend_matrix __PROTO(( float GPFAR * GPFAR *a, int nrl, int nrh, int ncl, int nch, int srh, int sch));
float GPFAR * GPFAR *retract_matrix __PROTO(( float GPFAR * GPFAR *a, int nrl, int nrh, int ncl, int nch, int srh, int sch));
void free_matrix __PROTO((float GPFAR * GPFAR *m, unsigned nrl, unsigned nrh, unsigned ncl, unsigned nch));
void free_vector __PROTO((float GPFAR *vec, int nl, int nh));
int is_binary_file __PROTO(( FILE *fp));
int fread_matrix __PROTO((FILE *fin, float GPFAR * GPFAR * GPFAR *ret_matrix, int *nr, int *nc, float GPFAR * GPFAR *row_title, float GPFAR * GPFAR *column_title));
int fwrite_matrix __PROTO(( FILE *fout, float GPFAR * GPFAR *m, int nrl, int nrh, int ncl, int nch, float GPFAR *row_title, float GPFAR *column_title));
float GPFAR * GPFAR *convert_matrix __PROTO((float GPFAR *a, int nrl, int nrh, int ncl, int nch));
void free_convert_matrix __PROTO((float GPFAR* GPFAR *b, int nrl, int nrh, int ncl, int nch));
