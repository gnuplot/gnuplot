/*
 * $Id: alloc.h,v 1.5 1997/04/10 02:32:40 drd Exp $
 */

/* prototypes from "alloc.c". This file figures out if the free hack is needed
 * and redefines free if necessary.
 */

char *gp_alloc __PROTO((unsigned long size, char *message));
generic *gp_realloc __PROTO((generic *p, unsigned long size, char *message));

/* dont define CHECK_HEAP_USE on a FARALLOC machine ! */

#ifdef CHECK_HEAP_USE

/* all allocated blocks have guards at front and back.
 * CHECK_POINTER checks guards on block, and checks that p is in range
 * START_LEAK_CHECK and END_LEAK_CHECK allow assert that no net memory
 * is allocated within enclosed block
 */
 
void checked_free(void *p);
void check_pointer_in_block(void *block, void *p, int size, char *file, int line);
void start_leak_check(char *file,int line);
void end_leak_check(char *file,int line);
# define free(x) checked_free(x)
# define CHECK_POINTER(block, p) check_pointer_in_block(block, p, sizeof(*p), __FILE__, __LINE__)
# define START_LEAK_CHECK() start_leak_check(__FILE__, __LINE__)
# define END_LEAK_CHECK() end_leak_check(__FILE__, __LINE__)
#else
# define CHECK_POINTER(block, p) /*nowt*/
# define START_LEAK_CHECK() /*nowt*/
# define END_LEAK_CHECK() /*nowt*/
#endif

#if defined(MSDOS) && defined(__TURBOC__) && !defined(DOSX286) || defined(_Windows) && !defined(WIN32)
#define FARALLOC
void gpfree __PROTO((generic *p));
#define free gpfree
#endif
