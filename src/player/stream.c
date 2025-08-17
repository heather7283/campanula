#include <pthread.h>
#include <unistd.h>
#include <string.h>

#include "player/internal.h"
#include "stream/open.h"
#include "config.h"

int player_stream_open(void *userdata, char *uri, struct mpv_stream_cb_info *info) {
    const char *id = uri + strlen(MPV_PROTOCOL) + strlen("://");

    struct stream_functions funcs = {0};
    void *cookie = NULL;
    if (!stream_open(id, config.preferred_audio_bitrate, config.preferred_audio_format,
                     &funcs, &cookie)) {
        goto err;
    }

    *info = (struct mpv_stream_cb_info){
        .read_fn = funcs.read,
        .seek_fn = funcs.seek,
        .size_fn = funcs.size,
        .close_fn = funcs.close,
        .cancel_fn = NULL,

        .cookie = cookie,
    };

    return 0;

err:
    return MPV_ERROR_LOADING_FAILED;
}


