#ifndef lint
static char *RCSid() { return RCSid("$Id: dynarray.c,v 1.3.2.1 2000/05/03 21:26:11 joze Exp $"); }
#endif

/* GNUPLOT - dynarray.c */

/* HBB: new code fragment for an 'OO-style' resizeable-array
 * container.  used in hidden3d, for now, but should be useful,
 * elsewhere (after extracting the header and removing the 'static'
 * declarations, that is */

#include "dynarray.h"

#include "alloc.h"
#include "graphics.h"

/*********** Implementation ************/

void
init_dynarray(array, entry_size, size, increment)
dynarray *array;
size_t entry_size;
long size, increment;
{
    array->v = 0;		/* preset value, in case gp_alloc fails */
    if (size)
	array->v = gp_alloc(entry_size * size, "init dynarray");
    array->size = size;
    array->end = 0;
    array->increment = increment;
    array->entry_size = entry_size;
}

void
free_dynarray(array)
dynarray *array;
{
    free(array->v);		/* should work, even if gp_alloc failed */
    array->v = 0;
    array->end = array->size = 0;
}

void
resize_dynarray(array, newsize)
dynarray *array;
long newsize;
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

void
extend_dynarray(array, increment)
dynarray *array;
long increment;
{
    resize_dynarray(array, array->size + increment);
}

GPHUGE void *
nextfrom_dynarray(array)
dynarray *array;
{
    if (!array->v)
	graph_error("nextfrom_dynarray: dynarray wan't initialized!");

    if (array->end >= array->size)
	extend_dynarray(array, array->increment);
    return (void *)((char *)(array->v) + array->entry_size * (array->end++));
}

void
droplast_dynarray(array)
dynarray *array;
{
    if (!array->v)
	graph_error("droplast_dynarray: dynarray wasn't initialized!");

    if (array->end)
	array->end--;
}
