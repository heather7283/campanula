#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "xdg.h"
#include "log.h"

int main(void) {
    log_init(stderr, LOG_TRACE, false);

    setenv("XDG_CONFIG_HOME", "/home/abobus/.myconfig", true);
    setenv("XDG_CACHE_HOME", "/home/abobus/.mycache", true);

    setenv("HOME", "/home/abobus", true);
    unsetenv("XDG_DATA_HOME");

    const char *config = get_xdg_config_dir();
    const char *cache = get_xdg_cache_dir();
    const char *data = get_xdg_data_dir();

    assert(config != NULL);
    assert(cache != NULL);
    assert(data != NULL);

    assert(strcmp(config, "/home/abobus/.myconfig/campanula/") == 0);
    assert(strcmp(cache, "/home/abobus/.mycache/campanula/") == 0);
    assert(strcmp(data, "/home/abobus/.local/share/campanula/") == 0);
}

