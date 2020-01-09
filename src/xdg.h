#ifndef GNUPLOT_XDG_H
#define GNUPLOT_XDG_H

#ifdef __unix__

/// List of possible XDG variables
typedef enum {
  kXDGNone = -1,
  kXDGConfigHome,  ///< XDG_CONFIG_HOME
  kXDGDataHome,    ///< XDG_DATA_HOME
  kXDGCacheHome,   ///< XDG_CACHE_HOME
  kXDGRuntimeDir,  ///< XDG_RUNTIME_DIR
  kXDGConfigDirs,  ///< XDG_CONFIG_DIRS
  kXDGDataDirs,    ///< XDG_DATA_DIRS
} XDGVarType;

char *xdg_get_var(const XDGVarType idx);

#endif // __unix__

#endif // GNUPLOT_XDG_H

