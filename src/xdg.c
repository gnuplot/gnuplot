#include "xdg.h"
#ifdef USE_XDG_BASEDIR

#include "plot.h"
#include "util.h"
#include "alloc.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Names of the environment variables, mapped to XDGVarType values */
static const char *xdg_env_vars[] = {
    [kXDGConfigHome] = "XDG_CONFIG_HOME",
    [kXDGDataHome] = "XDG_DATA_HOME",
    [kXDGStateHome] = "XDG_STATE_HOME",
    [kXDGCacheHome] = "XDG_CACHE_HOME",
    [kXDGRuntimeDir] = "XDG_RUNTIME_DIR",
    [kXDGConfigDirs] = "XDG_CONFIG_DIRS",
    [kXDGDataDirs] = "XDG_DATA_DIRS",
};

/* Defaults for XDGVarType values
 * Used in case environment variables contain nothing. Need to be expanded.
 */
static const char *const xdg_defaults[] = {
    [kXDGConfigHome] = "~/.config",
    [kXDGDataHome] = "~/.local/share",
    [kXDGStateHome] = "~/.local/state",
    [kXDGCacheHome] = "~/.cache",
    [kXDGRuntimeDir] = "",
    [kXDGConfigDirs] = "/etc/xdg/",
    [kXDGDataDirs] = "/usr/local/share/:/usr/share/",
};

/* helper function: return TRUE if dirname exists or can be created */

static TBOOLEAN check_dir(const char *dirname) {
#ifdef HAVE_SYS_STAT_H
     return existdir(dirname) || !mkdir(dirname, 00700);
#else	/* I believe this does not happen */
     return FALSE;
#endif
}

/* name used for subdirectory */
static const char *appname = "gnuplot";

/* Return pathname of XDG base directory or a file in it
 *
 * XDG base directory specification:
 * https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
 *
 * @param[in]  idx     XDG variable to use.
 * @param[in]  fname   if not NULL, name of a file in the base directory
 * @param[in]  subdir  if TRUE, append "/gnuplot" to the base directory
 * @param[in]  create  if TRUE, try to create the directory
 *
 * @return [allocated] pathname of the base directory if fname is NULL,
 *                     else of the file in the directory. Returns NULL if
 *                     create is TURE but the directory can't be created.
 */
char *xdg_get_path(XDGVarType idx, const char* fname,
		    TBOOLEAN subdir, TBOOLEAN create) {
    char *pathname;
    if ((pathname = getenv(xdg_env_vars[idx]))) {
        pathname = gp_strdup(pathname);	/* use the environment variable */
    }
    else {
	pathname = gp_strdup(xdg_defaults[idx]);    /* use the default */
	gp_expand_tilde(&pathname);
    }
    if (create && !check_dir(pathname)) {
	free(pathname);
	return NULL;
    }
    if (subdir) {
	pathname = gp_realloc(pathname,
			    strlen(pathname) + strlen(appname) + 2, "XDG");
	PATH_CONCAT(pathname, appname);
	if (create && !check_dir(pathname)) {
	    free(pathname);
	    return NULL;
	}
    }
    if (fname) {
	pathname = gp_realloc(pathname,
			    strlen(pathname) + strlen(fname) + 2, "XDG");
	PATH_CONCAT(pathname, fname);
    }
    return pathname;
}

#endif /* USE_XDG_BASEDIR */
