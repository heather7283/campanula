#include <stdlib.h>
#include <stddef.h>

#include "config.h"
#include "xdg.h"
#include "xmalloc.h"
#include "log.h"

struct config config = {
    .server_address = "10.200.200.10:4533/music",
    .application_name = "campanula",
    .preferred_audio_format = "opus",
    .preferred_audio_bitrate = 128,
    .username = "heather",
};

bool load_config(void) {
    config.config_dir = get_xdg_config_dir();
    if (config.config_dir == NULL) {
        ERROR("Could not find config directory");
        return false;
    } else if (!mkdir_with_parents(config.config_dir)) {
        ERROR("Could not create config directory");
        return false;
    }

    config.cache_dir = get_xdg_cache_dir();
    if (config.cache_dir == NULL) {
        ERROR("Could not find cache directory");
        return false;
    } else if (!mkdir_with_parents(config.cache_dir)) {
        ERROR("Could not create cache directory");
        return false;
    }

    config.data_dir = get_xdg_data_dir();
    if (config.data_dir == NULL) {
        return false;
    } else if (!mkdir_with_parents(config.data_dir)) {
        ERROR("Could not create data directory");
        return false;
    }

    config.password = xstrdup(getenv("CAMPANULA_PASSWORD"));
    if (config.password == NULL) {
        ERROR("CAMPANULA_PASSWORD is not set");
        return false;
    }

    return true;
}

