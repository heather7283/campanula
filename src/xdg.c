#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stddef.h>
#include <pwd.h>

#include "xdg.h"
#include "log.h"
#include "xmalloc.h"

static const char *get_home(void) {
    static char *home = NULL;
    if (home != NULL) {
        return home;
    }

    if ((home = xstrdup(getenv("HOME"))) == NULL) {
        WARN("HOME is unset, falling back to /etc/passwd");
        const struct passwd *pass = getpwuid(getuid());
        if (pass == NULL) {
            ERROR("failed to retrieve user info from /etc/passwd: %m");
        } else {
            home = xstrdup(pass->pw_dir);
        }
    }

    return home;
}

const char *get_xdg_config_dir(void) {
    static char *dir = NULL;
    if (dir != NULL) {
        return dir;
    }

    const char *xdg_config_home = getenv("XDG_CONFIG_HOME");
    if (xdg_config_home != NULL) {
        xasprintf(&dir, "%s/campanula/", xdg_config_home);
    } else {
        WARN("XDG_CONFIG_HOME is unset");
        const char *home = get_home();
        if (home != NULL) {
            xasprintf(&dir, "%s/.config/campanula/", home);
        }
    }

    return dir;
}

const char *get_xdg_cache_dir(void) {
    static char *dir = NULL;
    if (dir != NULL) {
        return dir;
    }

    const char *xdg_cache_home = getenv("XDG_CACHE_HOME");
    if (xdg_cache_home != NULL) {
        xasprintf(&dir, "%s/campanula/", xdg_cache_home);
    } else {
        WARN("XDG_CACHE_HOME is unset");
        const char *home = get_home();
        if (home != NULL) {
            xasprintf(&dir, "%s/.cache/campanula/", home);
        }
    }

    return dir;
}

const char *get_xdg_data_dir(void) {
    static char *dir = NULL;
    if (dir != NULL) {
        return dir;
    }

    const char *xdg_cache_home = getenv("XDG_DATA_HOME");
    if (xdg_cache_home != NULL) {
        xasprintf(&dir, "%s/campanula/", xdg_cache_home);
    } else {
        WARN("XDG_DATA_HOME is unset");
        const char *home = get_home();
        if (home != NULL) {
            xasprintf(&dir, "%s/.local/share/campanula/", home);
        }
    }

    return dir;
}

bool mkdir_with_parents(const char *path) {
    const pid_t pid = fork();
    switch (pid) {
    case -1: /* failure */
        ERROR("failed to fork child: %m");
        return false;
    case 0: /* child */
        execlp("mkdir", "mkdir", "-p", path, NULL);
        ERROR("failed to exec mkdir: %m");
        exit(1);
    default: /* parent */
    }

    int status;
    if (waitpid(pid, &status, 0) < 0 || !(WIFEXITED(status) && (WEXITSTATUS(status) == 0))) {
        return false;
    }

    return true;
}

