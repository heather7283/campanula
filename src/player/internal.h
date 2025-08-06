#ifndef SRC_PLAYER_INTERNAL_H
#define SRC_PLAYER_INTERNAL_H

#include <mpv/stream_cb.h>

#include "collections/array.h"
#include "signals.h"

#define MPV_PROTOCOL "campanula"

struct player {
    struct mpv_handle *mpv_handle;
    struct pollen_callback *mpv_events_callback;

    struct player_playlist {
        ARRAY(struct song) songs;
        size_t current_song;

        bool loop;
    } playlist;

    struct signal_emitter emitter;
};

extern struct player player;

int player_stream_open(void *userdata, char *uri, struct mpv_stream_cb_info *info);

void player_process_event(const struct mpv_event *event);

#endif /* #ifndef SRC_PLAYER_INTERNAL_H */

