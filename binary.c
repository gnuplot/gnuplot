#ifndef lint
static char *RCSid = "$Id: binary.c,v 1.9 1997/04/10 02:32:40 drd Exp $";
#endif

/*
 * The addition of gnubin and binary, along with a small patch
 * to command.c, will permit gnuplot to plot binary files.
 * gnubin  - contains the code that relies on gnuplot include files
 *                     and other definitions
 * binary      - contains those things that are independent of those 
 *                     definitions and files
 *
 * With these routines, hidden line removal of your binary data is possible!
 *
 * Last update:  3/29/92 memory allocation bugs fixed. jvdwoude@hut.nl
 *               3/09/92 spelling errors, general cleanup, use alloc with no
 *                       nasty fatal errors
 *               3/03/92 for Gnuplot 3.24.
 * Created from code for written by RKC for gnuplot 2.0b.
 *
 * Copyright (c) 1991,1992 Robert K. Cunningham, MIT Lincoln Laboratory
 *
 */

#include "plot.h"   /* We have to get TRUE and FALSE */
#include "stdfn.h"
#include "binary.h"

/* 
 * This routine scans the first block of the file to see if the file is a 
 * binary file.  A file is considered binary if 10% of the characters in it 
 * are not in the ascii character set. (values < 128), or if a NUL is found.
 * I hope this doesn't break when used on the bizzare PC's.
 */
int
  is_binary_file(fp)
register FILE *fp;
{
  register int i,len;
  register int odd;                /* Contains a count of the odd characters */
  long where;
  register unsigned char *c;
  unsigned char buffer[512];

  if((where = ftell(fp)) == -1){ /* Find out where we start */
    fprintf(stderr,"Notice: Assuming unseekable data is not binary\n");
    return(FALSE);
  }
  else {
    rewind(fp);

    len = fread(buffer,sizeof(char),512,fp);
    if (len <= 0)	                  /* Empty file is declared ascii */
      return(FALSE);

    c = buffer;

    /* now scan buffer to look for odd characters */
    odd = 0;
    for (i=0; i<len; i++,c++) {
      if (!*c) {			  /* NUL _never_ allowed in text */
	odd += len;
	break;
      }
      else if ((*c & 128) ||/* Meta-characters--we hope it's not formatting */
	       (*c == 127)|| /* DEL */
	       (*c < 32 && 
		*c != '\n' && *c != '\r' && *c != '\b' &&
		*c != '\t' && *c != '\f' && *c != 27 /*ESC*/))
	odd++;
    }
  
    fseek(fp,where,0); /* Go back to where we started */

    if (odd * 10 > len)             /* allow 10% of the characters to be odd */
      return(TRUE);
    else
      return(FALSE);
  }
}
/*========================= I/O Routines ================================
  These may be useful for situations other than just gnuplot.  Note that I 
  have included the reading _and_ the writing routines, so others can create 
  the file as well as read the file.
*/

/*
  This function reads a matrix from a stream

  This routine never returns anything other than vectors and arrays
  that range from 0 to some number.  

*/
#define START_ROWS 100/* Each of these must be at least 1 */
#define ADD_ROWS 50
int
  fread_matrix(fin,ret_matrix,nr,nc,row_title,column_title)
FILE *fin;
float GPFAR * GPFAR * GPFAR *ret_matrix,GPFAR * GPFAR * row_title, GPFAR * GPFAR *column_title;
int *nr,*nc;
{
  float  GPFAR * GPFAR *m, GPFAR *rt, GPFAR *ct;
  register int num_rows = START_ROWS;
  register int num_cols;
  register int current_row = 0;
  register float  GPFAR * GPFAR *temp_array;
  float fdummy;
  
  if (fread(&fdummy,sizeof(fdummy),1,fin) != 1)
	return FALSE;

  num_cols = (int)fdummy;
  
  /* 
    Choose a reasonable number of rows,
    allocate space for it and continue until this space
    runs out, then extend the matrix as necessary.
    */
  ct = vector(0,num_cols-1);
  fread(ct,sizeof(*ct),num_cols,fin);

  rt = vector(0,num_rows-1);
  m = matrix(0,num_rows-1,0,num_cols-1);

  while(fread(&rt[current_row], sizeof(rt[current_row]), 1, fin)==1){ 
    /* We've got another row */
    if(fread(m[current_row],sizeof(*(m[current_row])),num_cols,fin)!=num_cols)
      return(FALSE);      /* Not a True matrix */

    current_row++;
    if(current_row>=num_rows){ /* We've got to make a bigger rowsize */
      temp_array = extend_matrix(m,0,num_rows-1,0,num_cols-1,
				 num_rows+ADD_ROWS-1,num_cols-1);
      rt = extend_vector(rt,0,num_rows-1,num_rows+ADD_ROWS-1);
      
      num_rows+= ADD_ROWS;
      m = temp_array;
    }
  }
  /*  finally we force the matrix to be the correct row size */
  /*  bug fixed. procedure called with incorrect 6th argument. jvdwoude@hut.nl */
  temp_array = retract_matrix(m,0,num_rows-1,0,num_cols-1,current_row-1,num_cols-1);
  /* Now save the things that change */
  *ret_matrix = temp_array;
  *row_title = retract_vector(rt, 0, num_rows-1, current_row-1);
  *column_title = ct;
  *nr = current_row;/* Really the total number of rows */
  *nc = num_cols;
  return(TRUE);
}

/* This writes a matrix to a stream 
   Note that our ranges are inclusive ranges--and we can specify subsets.
   This behaves similarly to the xrange and yrange operators in gnuplot
   that we all are familiar with.
*/
int
  fwrite_matrix(fout,m,nrl,nrh,ncl,nch,row_title,column_title)
register FILE *fout;
register float  GPFAR * GPFAR *m, GPFAR *row_title, GPFAR *column_title;
register int nrl,nrh,ncl,nch;
{
  register int j;
  float length;
  register int col_length;
  register int status;
  float  GPFAR *title = NULL;

  length = (float)(col_length = nch-ncl+1);

  if((status = fwrite((char*)&length,sizeof(float),1,fout))!=1){
    fprintf(stderr,"fwrite 1 returned %d\n",status);
    return(FALSE);
  }
  
  if(!column_title){
    column_title = title = vector(ncl,nch);
    for(j=ncl; j<=nch; j++)
      title[j] = (float)j;
  }
  fwrite((char*)column_title,sizeof(float),col_length,fout);
  if(title){
    free_vector(title,ncl,nch);
    title = NULL;
  }

  if(!row_title){
    row_title = title = vector(nrl,nrh);
    for(j=nrl; j<=nrh; j++)
      title[j] = (float)j;
  }
    
  for(j=nrl; j<=nrh; j++){
    fwrite((char*)&row_title[j],sizeof(float),1,fout);
    fwrite((char*)(m[j]+ncl),sizeof(float),col_length,fout);
  }
  if(title)
    free_vector(title,nrl,nrh);

  return(TRUE);
}

/*===================== Support routines ==============================*/

/******************************** VECTOR *******************************
 *       The following routines interact with vectors.
 *
 *   If there is an error we don't really return - int_error breaks us out.
 *
 *   This subroutine based on a subroutine listed in "Numerical Recipies in C",
 *   by Press, Flannery, Teukoilsky and Vetterling (1988).
 *
 */
float GPFAR *vector(nl,nh)
     register int nl,nh;
{
  register float GPFAR *vec;

  if (!(vec = (float GPFAR *)gp_alloc((unsigned long) (nh-nl+1)*sizeof(float), NULL))){
    int_error("not enough memory to create vector",NO_CARET);
    return NULL;/* Not reached */
  }
  return (vec-nl);
}
/* 
 *  Free a vector allocated above
 *
 *   This subroutine based on a subroutine listed in "Numerical Recipies in C",
 *   by Press, Flannery, Teukoilsky and Vetterling (1988).
 *
 */
void 
  free_vector(vec,nl,nh)
float  GPFAR *vec;
int nl,nh;
{
  free(vec+nl);
}
/************ Routines to modify the length of a vector ****************/  
float  GPFAR *
  extend_vector(vec,old_nl,old_nh,new_nh)
float  GPFAR *vec;
register int old_nl,old_nh,new_nh;
{
  register float  GPFAR *new_v;
  new_v = (float GPFAR *)gp_realloc((void*)(vec+old_nl),
                       (unsigned long)(new_nh-old_nl+1)*sizeof(float),
		       "extend vector");
  return new_v-old_nl;
}

float  GPFAR *
  retract_vector(v,old_nl,old_nh,new_nh)
float  GPFAR *v;
register int old_nl,old_nh,new_nh;
{
  register float GPFAR *new_v;
  new_v = (float GPFAR *)gp_realloc((void*)(v+old_nl),
   (unsigned long)(new_nh-old_nl+1)*sizeof(float), "retract vector");
  return new_v-old_nl;
}
/***************************** MATRIX ************************
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
float 
   GPFAR * GPFAR *matrix(nrl,nrh,ncl,nch)
register int nrl,nrh,ncl,nch;
{
  register int i;
  register float GPFAR * GPFAR *m;

  m = (float GPFAR * GPFAR *)gp_alloc((unsigned long)(nrh-nrl+1)*sizeof(float GPFAR *), "matrix");
  m -= nrl;

  for (i=nrl; i<=nrh; i++)
    {
      if(!(m[i] = (float GPFAR *) gp_alloc((unsigned long)(nch-ncl+1)*sizeof(float), NULL))){
	free_matrix(m,nrl,i-1,ncl,nch);
	int_error("not enough memory to create matrix",NO_CARET);
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
  free_matrix(m,nrl,nrh,ncl,nch)
float  GPFAR * GPFAR *m;
unsigned nrl,nrh,ncl,nch;
{
  register unsigned int i;

  for (i=nrl; i<=nrh; i++) 
    free((char GPFAR *) (m[i]+ncl));
  free((char GPFAR *) (m+nrl));
}
/*
  This routine takes a sub matrix and extends the number of rows and 
  columns for a new matrix
*/
float GPFAR * GPFAR *extend_matrix(a,nrl,nrh,ncl,nch,srh,sch)
     register float  GPFAR * GPFAR *a;
     register int nrl,nrh,ncl,nch;
     register int srh,sch;
{
  register int i;
  register float GPFAR * GPFAR *m;

  m = (float GPFAR * GPFAR *)gp_realloc((void*)(a+nrl),(unsigned long)(srh-nrl+1)*sizeof(float GPFAR *), "extend matrix");

  m -= nrl;

  if(sch != nch){
    for(i=nrl; i<=nrh; i++)
      {/* Copy and extend rows */
	if(!(m[i] = extend_vector(m[i],ncl,nch,sch))){
	  free_matrix(m,nrl,nrh,ncl,sch);
	  int_error("not enough memory to extend matrix",NO_CARET);
	  return NULL;
	}
      }
  }
  for(i=nrh+1; i<=srh; i++)
    {
      if(!(m[i] = (float GPFAR *) gp_alloc((unsigned long) (nch-ncl+1)*sizeof(float), NULL))){
	free_matrix(m,nrl,i-1,nrl,sch);
	int_error("not enough memory to extend matrix",NO_CARET);
	return NULL;
      }
      m[i] -= ncl;
    }
  return m;
}
/*
  this routine carves a large matrix down to size
*/
float GPFAR * GPFAR *retract_matrix(a,nrl,nrh,ncl,nch,srh,sch)
     register float  GPFAR * GPFAR *a;
     register int nrl,nrh,ncl,nch;
     register int srh,sch;
{
  register int i;
  register float  GPFAR * GPFAR *m;

  for(i=srh+1; i<=nrh; i++) {
    free_vector(a[i],ncl,nch);
  }

  m = (float GPFAR * GPFAR *)gp_realloc((void*)(a+nrl), (unsigned long)(srh-nrl+1)*sizeof(float GPFAR *), "retract matrix");

  m -= nrl;

  if(sch != nch){
    for(i=nrl; i<=srh; i++)       
	if(!(m[i] = retract_vector(m[i],ncl,nch,sch))){ {/* Shrink rows */
	  free_matrix(m,nrl,srh,ncl,sch);
	  int_error("not enough memory to retract matrix",NO_CARET);
	  return NULL;
	}
      }
  }

  return m;
}

float 
   GPFAR * GPFAR *convert_matrix(a,nrl,nrh,ncl,nch)
float GPFAR *a;
register int nrl,nrh,ncl,nch;

/* allocate a float matrix m[nrl...nrh][ncl...nch] that points to the
matrix declared in the standard C manner as a[nrow][ncol], where 
nrow=nrh-nrl+1, ncol=nch-ncl+1.  The routine should be called with
the address &a[0][0] as the first argument.  This routine does
not free the memory used by the original array a but merely assigns
pointers to the rows. */

{
  register int i,j,ncol,nrow;
  register float GPFAR * GPFAR *m;

  nrow=nrh-nrl+1;
  ncol=nch-ncl+1;
  m = (float GPFAR * GPFAR *)gp_alloc((unsigned long)(nrh-nrl+1)*sizeof(float GPFAR *), "convert_matrix");
  m -= nrl;

  m[nrl]=a-ncl;
  for(i=1,j=nrl+1;i<=nrow-1;i++,j++) m[j]=m[j-1]+ncol;
  return m;
}

void free_convert_matrix(b,nrl,nrh,ncl,nch)
float GPFAR* GPFAR *b;
register int nrl,nrh,ncl,nch;
{
	free((char*) (b+nrl));
}
