#include <mpv/client.h>

#include "player/control.h"
#include "player/common.h"
#include "collections/string.h"
#include "log.h"

bool player_play(const char *id) {
    struct string url = {0};
    string_appendf(&url, "%s://%s", PLAYER_PROTOCOL, id);

    const char *args[] = { "loadfile", url.str, NULL };
    int ret = mpv_command(player_state.mpv_handle, args);
    string_free(&url);

    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to start playback: %s", mpv_error_string(ret));
        return false;
    }

    return true;
}

