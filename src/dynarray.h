#ifndef DYNARRAY__H
#define DYNARRAY__H

#include <stdlib.h>
#include "plot.h"

typedef struct dynarray {
  long size;		/* alloced size of the array */
  long end;		/* index of first unused entry */
  long increment;	/* amount to increment size by, on realloc */
  size_t entry_size;		/* size of the entries in this array */
  void GPHUGE *v;		/* the vector itself */
} dynarray;

/* Prototypes */
void init_dynarray __PROTO((dynarray *array,
				     size_t element,
				     long size,
				     long increment));
void free_dynarray __PROTO((dynarray *array));
void extend_dynarray __PROTO((dynarray *array,
			       long increment));
void resize_dynarray __PROTO((dynarray *array,
				long newsize));
GPHUGE void *nextfrom_dynarray __PROTO((dynarray *array));
void droplast_dynarray __PROTO((dynarray *array));

#endif /* DYNARRAY_H */
