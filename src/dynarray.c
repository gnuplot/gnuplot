#ifndef lint
static char *RCSid() { return RCSid("$Id: dynarray.c,v 1.4 2000/05/02 18:30:04 lhecking Exp $"); }
#endif

/*[
 * Copyright 1986 - 1993, 1998, 1999   Thomas Williams, Colin Kelley
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

/* This module implements a dynamically growing array of arbitrary
 * elements parametrized by their sizeof(). There is no 'access
 * function', i.e. you'll have to access the elements of the
 * dynarray->v memory block by hand. It's implemented in OO-style,
 * even though this is C, not C++.  In particular, every function
 * takes a pointer to a data structure type 'dynarray', which mimics
 * the 'this' pointer of an object. */

#include "dynarray.h"

#include "alloc.h"
#include "util.h"		/* for graph_error() */

/* The 'constructor' of a dynarray object: initializes all the
 * variables to well-defined startup values */
void 
init_dynarray(array, entry_size, size, increment)
     dynarray *array;		/* the 'this' pointer */
     size_t entry_size;		/* size of the array's elements */
     long size, increment;	/* original size, and incrementation
				 * step of the dynamical array */
{
  array->v = 0;			/* preset value, in case gp_alloc fails */
  if (size)
    array->v = gp_alloc(entry_size*size, "init dynarray");
  array->size = size;
  array->end = 0;
  array->increment = increment;
  array->entry_size = entry_size;
}

/* The 'destructor'; sets all crucial elements of the structure to
 * well-defined values to avoid problems by use of bad pointers... */
void 
free_dynarray(array)
     dynarray *array;		/* the 'this' pointer */
{
    free(array->v);		/* should work, even if gp_alloc failed */
    array->v = 0;
    array->end = array->size = 0;
}

/* Set the size of the dynamical array to a new, fixed value */
void 
resize_dynarray(array, newsize)
     dynarray *array;		/* the 'this' pointer */
     long newsize;		/* the new size to set it to */
{
    if (!array->v)
	graph_error("resize_dynarray: dynarray wasn't initialized!");

    if (newsize == 0)
	free_dynarray(array);
    else {
	array->v = gp_realloc(array->v, array->entry_size * newsize, "extend dynarray");
	array->size = newsize;
    }
}

/* Increase the size of the dynarray by a given amount */
void 
extend_dynarray(array, increment)
     dynarray *array;		/* the 'this' pointer */
     long increment;		/* the amount to increment by */
{
    resize_dynarray(array, array->size + increment);
}

/* Get pointer to the element one past the current end of the dynamic
 * array. Resize it if necessary. Returns a pointer-to-void to that
 * element. */
GPHUGE void *
nextfrom_dynarray(array)
     dynarray *array;		/* the 'this' pointer */
{
    if (!array->v)
	graph_error("nextfrom_dynarray: dynarray wan't initialized!");

    if (array->end >= array->size)
	extend_dynarray(array, array->increment);
    return (void *)((char *)(array->v) + array->entry_size * (array->end++));
}

/* Release the element at the current end of the dynamic array, by
 * moving the 'end' index one element backwards */
void 
droplast_dynarray(array)
     dynarray *array;
{
    if (!array->v)
	graph_error("droplast_dynarray: dynarray wasn't initialized!");

    if (array->end)
	array->end--;
}
