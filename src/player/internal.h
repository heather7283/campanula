#ifndef SRC_PLAYER_INTERNAL_H
#define SRC_PLAYER_INTERNAL_H

#include <sys/types.h>

#include <mpv/stream_cb.h>

#include "collections/vec.h"
#include "signals.h"

#define MPV_PROTOCOL "campanula"

struct player {
    struct mpv_handle *mpv_handle;
    struct pollen_callback *mpv_events_callback;

    bool is_paused, is_idle;

    struct player_playlist {
        VEC(struct song) songs;
        int current_song;
    } playlist;

    struct signal_emitter emitter;
};

extern struct player player;

int player_stream_open(void *userdata, char *uri, struct mpv_stream_cb_info *info);

void player_process_event(const struct mpv_event *event);

/* Append song to playlist (TODO: ability to specify index) */
bool player_loadfile(const struct song *song);
/* Stop playback and clear playlist. */
bool player_stop(void);

#endif /* #ifndef SRC_PLAYER_INTERNAL_H */

