
/* The Death of global variables - part one. */

/* #define DEBUG */

#include "plot.h"
#include "variable.h"

#define PATHSEP_TO_NUL(arg) \
{ char *s = arg; \
  while ((s=strchr(s,PATHSEP)) != NULL) \
    *s++ = NUL; }

#define PRINT_LOADPATH(limit) \
  while (p < limit) { \
    fprintf(stderr, "\"%s\" ",p); \
    p += strlen(p) + 1; \
  } \
  fputc('\n',stderr);

/*
 * char *loadpath_handler (int, char *)
 *
 */
char *loadpath_handler(action, path)
int action;
char *path;
{
    /* loadpath variable
     * the path elements are '\0' separated (!)
     * this way, reading out loadpath is very
     * easy to implement */
    static char *loadpath;
    /* index pointer, end of loadpath,
     * env section of loadpath, current limit, in that order */
    static char *p, *last, *envptr, *limit;
    static int beenhere;

    switch (action) {
    case ACTION_CLEAR:
	/* Clear loadpath, fall through to init */
	FPRINTF((stderr, "Clear loadpath\n"));
	free(loadpath);
	loadpath = p = last = NULL;
    case ACTION_INIT:
	/* Init loadpath from environment */
	FPRINTF((stderr, "Init loadpath from environment\n"));
	assert(loadpath==NULL);
	if (loadpath == NULL);
	{
	    char *envlib = getenv("GNUPLOT_LIB");
	    if (envlib != NULL) {
		int len = strlen(envlib);
		loadpath = gp_alloc(len+1, "init loadpath");
		safe_strncpy(loadpath,envlib,len+1);
		/* point to end of loadpath */
		last = loadpath + len;
		/* convert all PATHSEPs to \0 */
		PATHSEP_TO_NUL(loadpath);
	    } /* else: NULL = empty */
	} /* else: already initialised; int_warn (?) */
	/* point to env portion of loadpath */
	envptr = loadpath;
	break;
    case ACTION_SET:
	/* set the loadpath */
	FPRINTF((stderr, "Set loadpath\n"));
	if (path != NULL && *path != NUL) {
	    /* length of env portion */
	    int elen = last - envptr;
	    int plen = strlen(path);
	    if (loadpath != NULL && envptr != NULL) {
		/* we are prepending a path name; because
		 * realloc() preserves only the contents up
		 * to the minimum of old and new size, we move
		 * the part to be preserved to the beginning
		 * of the string; use memmove() because strings
		 * may overlap */
		memmove(loadpath,envptr,elen+1);
	    }
	    loadpath = gp_realloc(loadpath,elen+1+plen+1, "expand loadpath");
	    /* now move env part back to the end to make space for
	     * the new path */
	    memmove(loadpath+plen+1,loadpath,elen+1);
	    strcpy(loadpath,path);
	    /* separate new path(s) and env path(s) */
	    loadpath[plen] = PATHSEP;
	    /* adjust pointer to env part and last */
	    envptr = &loadpath[plen+1];
	    last = envptr + elen;
	    PATHSEP_TO_NUL(loadpath);
	} /* else: NULL = empty */
	break;
    case ACTION_SHOW:
	/* print the current, full loadpath */
	FPRINTF((stderr, "Show loadpath\n"));
	if (loadpath != NULL) {
	    p = loadpath;
	    fputs("\tloadpath is ",stderr);
	    PRINT_LOADPATH(envptr);
	    if (envptr != NULL) {
		/* env part */
		p = envptr;
		fputs("\tsystem loadpath is ",stderr);
		PRINT_LOADPATH(last);
	    }
	} else
	    fputs("\tno loadpath\n",stderr);
	break;
    case ACTION_SAVE:
	/* we don't save the load path taken from the
	 * environment, so don't go beyond envptr when
	 * extracting the path elements
	 */
	limit = envptr;
    case ACTION_GET:
	/* subsequent calls to get_loadpath() return all
	 * elements of the loadpath until exhausted
	 */
	FPRINTF((stderr, "Get loadpath\n"));
	if (!loadpath)
	    return NULL;
	if (!beenhere) {
	    /* init section */
	    beenhere = 1;
	    p = loadpath;
	    if (limit == NULL)
		limit = last;
	    return p;
	} else {
	    p += strlen(p);
	    /* skip over '\0' */
	    p++;
	    if (p < limit)
		return p;
	    else {
		beenhere = 0;
		return NULL;
	    }
	}
	break;
    case ACTION_NULL:
	/* just return */
    default:
	break;
    }

    /* should always be ignored - points to the
     * first path in the list */
    return loadpath;

}
