#ifndef GNUPLOT_XDG_H
#define GNUPLOT_XDG_H

#if defined(__unix__) || (defined(__APPLE__) && defined(__MACH__))

#define USE_XDG_BASEDIR

#include "syscfg.h" /* for TBOOLEAN */

/* List of possible XDG variables */
typedef enum {
  kXDGNone = -1,
  kXDGConfigHome,  /* XDG_CONFIG_HOME */
  kXDGDataHome,    /* XDG_DATA_HOME */
  kXDGStateHome,   /* XDG_STATE_HOME */
  kXDGCacheHome,   /* XDG_CACHE_HOME */
  kXDGRuntimeDir,  /* XDG_RUNTIME_DIR */
  kXDGConfigDirs,  /* XDG_CONFIG_DIRS */
  kXDGDataDirs,    /* XDG_DATA_DIRS */
} XDGVarType;

char *xdg_get_path(XDGVarType idx, const char *fname,
		    TBOOLEAN subdir, TBOOLEAN create);

#endif /* USE_XDG_BASEDIR */

#endif /* GNUPLOT_XDG_H */

