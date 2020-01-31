#include "xdg.h"
#include "plot.h"
#include "util.h"

#ifdef __unix__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Names of the environment variables, mapped to XDGVarType values */
static const char *xdg_env_vars[] = {
    [kXDGConfigHome] = "XDG_CONFIG_HOME",
    [kXDGDataHome] = "XDG_DATA_HOME",
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
    [kXDGCacheHome] = "~/.cache",
    [kXDGRuntimeDir] = "",
    [kXDGConfigDirs] = "/etc/xdg/",
    [kXDGDataDirs] = "/usr/local/share/:/usr/share/",
};


/* Return XDG variable value
 *
 * First query the environement variable and if that does not exist fall back
 * to the defaults. Further reading:
 * https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
 *
 * @param[in]  idx  XDG variable to use.
 *
 * @return [allocated] variable value.
 */
char *xdg_get_var(const XDGVarType idx) {
    /* Check the environment variable. If it is there, we are done. */
    char *XDGVar;
    if ((XDGVar = getenv(xdg_env_vars[idx])) != NULL) {
        return gp_strdup(XDGVar);
    }

    /* Looks like the environment variable is not there.
     * Load the default and run word expansion on it.
     */
    XDGVar = gp_strdup(xdg_defaults[idx]);
    gp_expand_tilde(&XDGVar);
    return XDGVar;
}

#endif /* __unix__ */
