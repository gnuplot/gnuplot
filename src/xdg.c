#include "xdg.h"

#ifdef __unix__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wordexp.h>

/// Names of the environment variables, mapped to XDGVarType values
static const char *xdg_env_vars[] = {
    [kXDGConfigHome] = "XDG_CONFIG_HOME",
    [kXDGDataHome] = "XDG_DATA_HOME",
    [kXDGCacheHome] = "XDG_CACHE_HOME",
    [kXDGRuntimeDir] = "XDG_RUNTIME_DIR",
    [kXDGConfigDirs] = "XDG_CONFIG_DIRS",
    [kXDGDataDirs] = "XDG_DATA_DIRS",
};

/// Defaults for XDGVarType values
///
/// Used in case environment variables contain nothing. Need to be expanded.
static const char *const xdg_defaults[] = {
    [kXDGConfigHome] = "~/.config",
    [kXDGDataHome] = "~/.local/share",
    [kXDGCacheHome] = "~/.cache",
    [kXDGRuntimeDir] = "",
    [kXDGConfigDirs] = "/etc/xdg/",
    [kXDGDataDirs] = "/usr/local/share/:/usr/share/",
};


/// Perform word expansion for a given string
///
/// Try to perform word expansion on the input like a shell, but don't perform
/// command substitution (dangerous) and treat undefined variables like an error.
/// failures will be reported on stderr.  If word expansion fails, NULL is
/// returned.
///
/// @param[in]  str  input to perform word expansion on
///
/// @return [allocated] expanded string
static char *xdg_wordexp_unix(const char *const str) {
    char *word = NULL;

    wordexp_t p;
    switch (wordexp(str, &p, WRDE_SHOWERR | WRDE_NOCMD | WRDE_UNDEF)) {
    case WRDE_BADCHAR:
        fprintf(stderr,
                "%s: Illegal occurrence of newline or one of |, &, ;, "
                "<, >, (, ), {, }.",
                __FUNCTION__);
        goto fail;
    case WRDE_BADVAL:
        fprintf(stderr,
                "%s: An undefined shell variable was referenced, and "
                "the WRDE_UNDEF flag told us to consider this an "
                "error.",
                __FUNCTION__);
        goto fail;
    case WRDE_CMDSUB:
        fprintf(stderr,
                "%s: Command substitution requested, but the "
                "WRDE_NOCMD flag told us to consider this an error.",
                __FUNCTION__);
        goto fail;
    case WRDE_NOSPACE:
        fprintf(stderr, "%s: Out of memory.", __FUNCTION__);
        goto fail;
    case WRDE_SYNTAX:
        fprintf(stderr,
                "%s: Shell syntax error, such as unbalanced "
                "parentheses or unmatched quotes.",
                __FUNCTION__);
        goto fail;
    }

    assert(p.we_wordc == 1); // Expand to only a single word!
    word = strdup(p.we_wordv[0]);
    wordfree(&p);

fail:
    return word;
}

/// Return XDG variable value
///
/// First query the environement variable and if that does not exist fall back
/// to the defaults. Further reading:
/// https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
///
/// @param[in]  idx  XDG variable to use.
///
/// @return [allocated] variable value.
char *xdg_get_var(const XDGVarType idx) {
    // Check the environment variable, if it is there, we are done.
    char *XDGVar;
    if ((XDGVar = getenv(xdg_env_vars[idx])) != NULL) {
        return strdup(XDGVar);
    }

    // Looks like the environment variable is not there.
    // Load the default and run word expansion on it.
    return xdg_wordexp_unix(xdg_defaults[idx]);
}

#endif // __unix__
