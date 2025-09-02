#include "player/playlist.h"
#include "player/events.h"
#include "player/internal.h"
#include "player/control.h"

void playlist_append_song(const struct song *song) {
    struct player_playlist *pl = &player.playlist;

    if (player_loadfile(song)) {
        struct song *new_song = VEC_EMPLACE_BACK(&pl->songs);
        song_deep_copy(new_song, song);

        //if (VEC_SIZE(&pl->songs) == 1) {
        //    player_play_nth(0);
        //}

        signal_emit_i64(&player.emitter, PLAYER_EVENT_PLAYLIST_SONG_ADDED, VEC_SIZE(&pl->songs) - 1);
    }
}

void playlist_clear(void) {
    struct player_playlist *pl = &player.playlist;

    if (player_stop()) {
        VEC_CLEAR(&pl->songs);
        signal_emit_ptr(&player.emitter, PLAYER_EVENT_PLAYLIST_CLEARED, NULL);
    }
}

int playlist_get_songs(const struct song **songs) {
    struct player_playlist *pl = &player.playlist;

    if (songs != NULL) {
        *songs = VEC_DATA(&pl->songs);
    }
    return VEC_SIZE(&pl->songs);
}

int playlist_get_current_song(const struct song **song) {
    struct player_playlist *pl = &player.playlist;

    if (song != NULL) {
        if (VEC_SIZE(&pl->songs) > 0) {
            *song = VEC_AT(&pl->songs, pl->current_song);
        } else {
            *song = NULL;
        }
    }

    return pl->current_song;
}

