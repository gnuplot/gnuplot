#ifndef lint
static char *RCSid() { return RCSid("$Id: alloc.c,v 1.14 2010/10/04 19:06:49 sfeam Exp $"); }
#endif

/* GNUPLOT - alloc.c */

/*[
 * Copyright 1986 - 1993, 1998, 2004   Thomas Williams, Colin Kelley
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

/*
 * AUTHORS
 *
 * Alexander Lehmann (collected functions from misc.c and binary.c)
 *
 */

#include "alloc.h"

#ifndef NO_GIH
# include "help.h"
#endif
#include "util.h"

#if defined(MSDOS) && defined(__TURBOC__)
# include <alloc.h>		/* for farmalloc, farrealloc */
#endif

#if defined(_Windows) && !defined(WIN32)
# include <windows.h>
# include <windowsx.h>
# define farmalloc(s) GlobalAllocPtr(GHND,s)
# define farrealloc(p,s) GlobalReAllocPtr(p,s,GHND)
#endif



#ifndef GP_FARMALLOC
# ifdef FARALLOC
#  define GP_FARMALLOC(size) farmalloc ((size))
#  define GP_FARREALLOC(p,size) farrealloc ((p), (size))
# else
#  ifdef MALLOC_ZERO_RETURNS_ZERO
#   define GP_FARMALLOC(size) malloc ((size_t)((size==0)?1:size))
#  else
#   define GP_FARMALLOC(size) malloc ((size_t)(size))
#  endif
#  define GP_FARREALLOC(p,size) realloc ((p), (size_t)(size))
# endif
#endif

/* uncomment if you want to trace all allocs */
#define TRACE_ALLOC(x)		/*printf x */


#ifdef CHECK_HEAP_USE

/* This is in no way supported, and in particular it breaks the
 * online help. But it is useful to leave it in in case any
 * heap-corruption bugs turn up. Wont work with FARALLOC
 */

struct frame_struct {
    char *use;
    int requested_size;
    int pad;			/* preserve 8-byte alignment */
    int checksum;
};

struct leak_struct {
    char *file;
    int line;
    int allocated;
};

static struct leak_struct leak_stack[40];	/* up to 40 nested leak checks */
static struct leak_struct *leak_frame = leak_stack;

static long bytes_allocated = 0;

#define RESERVED_SIZE sizeof(struct frame_struct)
#define CHECKSUM_INT 0xcaac5e1f
#define CHECKSUM_FREE 0xf3eed222
#define CHECKSUM_CHAR 0xc5

static void
mark(struct frame_struct *p, unsigned long size, char *usage)
{
    p->use = usage;
    p->requested_size = size;
    p->checksum = (CHECKSUM_INT ^ (int) (p->use) ^ size);
    ((unsigned char *) (p + 1))[size] = CHECKSUM_CHAR;
}

#define mark_free(p) ( ((struct frame_struct *)p)[-1].checksum = CHECKSUM_FREE)

static void
validate(generic *x)
{
    struct frame_struct *p = (struct frame_struct *) x - 1;
    if (p->checksum != (CHECKSUM_INT ^ (int) (p->use) ^ p->requested_size)) {
	fprintf(stderr, "Heap corruption at start of block for %s\n", p->use);
	if (p->checksum == CHECKSUM_FREE)
	    fprintf(stderr, "Looks like it has already been freed ?\n");
	abort();
    }
    if (((unsigned char *) (p + 1))[p->requested_size] != CHECKSUM_CHAR) {
	fprintf(stderr, "Heap corruption at end of block for %-60s\n", p->use);
	int_error(NO_CARET, "Argh !");
    }
}

/* used to confirm that a pointer is inside an allocated region via
 * macro CHECK_POINTER. Nowhere near as good as using a bounds-checking
 * compiler (such as gcc-with-bounds-checking), but when we do
 * come across problems, we can add these guards to the code until
 * we find the problem, and then leave the guards in (as CHECK_POINTER
 * macros which expand to nothing, until we need to re-enable them)
 */

void
check_pointer_in_block(generic *block, generic *p, int size, char *file, int line)
{
    struct frame_struct *f = (struct frame_struct *) block - 1;
    validate(block);
    if (p < block || p >= (block + f->requested_size)) {
	fprintf(stderr, "argh - pointer %p outside block %p->%p for %s at %s:%d\n",
		p, block, (char *) block + f->requested_size, f->use, file, line);
	int_error(NO_CARET, "argh - pointer misuse !");
    }
}

generic *
gp_alloc(size_t size, const char *usage)
{
    struct frame_struct *p;
    size_t total_size = size + RESERVED_SIZE + 1;

    TRACE_ALLOC(("gp_alloc %d for %s\n", (int) size,
		 usage ? usage : "<unknown>"));

    p = malloc(total_size);
    if (!p)
	int_error(NO_CARET, "Out of memory");

    bytes_allocated += size;

    mark(p, size, usage);

    /* Cast possibly needed for K&R compilers */
    return (generic *) (p + 1);
}

generic *
gp_realloc(generic *old, size_t size, const char *usage)
{
    if (!old)
	return gp_alloc(size, usage);
    validate(old);
    /* if block gets moved, old block is marked free.  If not, we'll
     * remark it later */
    mark_free(old);		

    {
	struct frame_struct *p = (struct frame_struct *) old - 1;
	size_t total = size + RESERVED_SIZE + 1;

	p = realloc(p, total);

	if (!p)
	    int_error(NO_CARET, "Out of memory");

	TRACE_ALLOC(("gp_realloc %d for %s (was %d)\n", (int) size,
		     usage ? usage : "<unknown>", p->requested_size));

	bytes_allocated += size - p->requested_size;

	mark(p, size, usage);

	return (generic *) (p + 1);
    }
}

#undef free

void
checked_free(generic *p)
{
    struct frame_struct *frame;

    validate(p);
    mark_free(p);		/* trap attempts to free twice */
    frame = (struct frame_struct *) p - 1;
    TRACE_ALLOC(("free %d for %s\n",
		 frame->requested_size,
		 (frame->use ? frame->use : "(NULL)")));
    bytes_allocated -= frame->requested_size;
    free(frame);
}


/* this leak checking stuff will be broken by first int_error or interrupt */

void
start_leak_check(char *file, int line)
{
    if (leak_frame >= leak_stack + 40) {
	fprintf(stderr, "too many nested memory-leak checks - %s:%d\n",
		file, line);
	return;
    }
    leak_frame->file = file;
    leak_frame->line = line;
    leak_frame->allocated = bytes_allocated;

    ++leak_frame;
}

void
end_leak_check(char *file, int line)
{
    if (--leak_frame < leak_stack) {
	fprintf(stderr, "memory-leak stack underflow at %s:%d\n", file, line);
	return;
    }
    if (leak_frame->allocated != bytes_allocated) {
	fprintf(stderr,
		"net change of %+d heap bytes between %s:%d and %s:%d\n",
		(int) (bytes_allocated - leak_frame->allocated),
		leak_frame->file, leak_frame->line, file, line);
    }
}

#else /* CHECK_HEAP_USE */

/* gp_alloc:
 * allocate memory
 * This is a protected version of malloc. It causes an int_error
 * if there is not enough memory, but first it tries FreeHelp()
 * to make some room, and tries again. If message is NULL, we
 * allow NULL return. Otherwise, we handle the error, using the
 * message to create the int_error string. Note cp/sp_extend uses realloc,
 * so it depends on this using malloc().
 */

generic *
gp_alloc(size_t size, const char *message)
{
    char *p;			/* the new allocation */

#ifndef NO_GIH
    p = GP_FARMALLOC(size);
    if (p == (char *) NULL) {
	FreeHelp();		/* out of memory, try to make some room */
#endif /* NO_GIH */
	p = GP_FARMALLOC(size);	/* try again */
	if (p == NULL) {
	    /* really out of memory */
	    if (message != NULL) {
		int_error(NO_CARET, "out of memory for %s", message);
		/* NOTREACHED */
	    }
	    /* else we return NULL */
	}
#ifndef NO_GIH
    }
#endif
    return (p);
}

/*
 * note gp_realloc assumes that failed realloc calls leave the original mem
 * block allocated. If this is not the case with any C compiler, a substitue
 * realloc function has to be used.
 */

generic *
gp_realloc(generic *p, size_t size, const char *message)
{
    char *res;			/* the new allocation */

    /* realloc(NULL,x) is meant to do malloc(x), but doesn't always */
    if (!p)
	return gp_alloc(size, message);

#ifndef NO_GIH
    res = GP_FARREALLOC(p, size);
    if (res == (char *) NULL) {
	FreeHelp();		/* out of memory, try to make some room */
#endif /* NO_GIH */
	res = GP_FARREALLOC(p, size);	/* try again */
	if (res == (char *) NULL) {
	    /* really out of memory */
	    if (message != NULL) {
		int_error(NO_CARET, "out of memory for %s", message);
		/* NOTREACHED */
	    }
	    /* else we return NULL */
	}
#ifndef NO_GIH
    }
#endif
    return (res);
}

#endif /* CHECK_HEAP_USE */

#ifdef FARALLOC
void
gpfree(generic *p)
{
#ifdef _Windows
    HGLOBAL hGlobal = GlobalHandle(SELECTOROF(p));
    GlobalUnlock(hGlobal);
    GlobalFree(hGlobal);
#else
    farfree(p);
#endif
}

#endif
