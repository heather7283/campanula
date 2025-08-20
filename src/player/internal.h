#ifndef SRC_PLAYER_INTERNAL_H
#define SRC_PLAYER_INTERNAL_H

#include <mpv/stream_cb.h>

#include "collections/vec.h"
#include "signals.h"

/*
 * Basically everything that should not be exposed to users goes here,
 * idk how else to do that honestly, a bit of a mess but eh whatever
 */

#define MPV_PROTOCOL "campanula"

struct player {
    struct mpv_handle *mpv_handle;
    struct pollen_callback *mpv_events_callback;

    struct player_playlist {
        VEC(struct song) songs;
        size_t current_song;

        bool loop;
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

