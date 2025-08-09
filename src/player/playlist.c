#include "player/playlist.h"
#include "player/internal.h"

void playlist_append_song(const struct song *song) {
    struct player_playlist *pl = &player.playlist;

    if (player_loadfile(song)) {
        struct song *new_song = ARRAY_EMPLACE_BACK(&pl->songs);
        song_deep_copy(new_song, song);
    }
}

void playlist_clear(void) {
    struct player_playlist *pl = &player.playlist;

    if (player_stop()) {
        ARRAY_CLEAR(&pl->songs);
    }
}

size_t playlist_get_songs(const struct song **songs) {
    struct player_playlist *pl = &player.playlist;

    if (songs != NULL) {
        *songs = ARRAY_DATA(&pl->songs);
    }
    return ARRAY_SIZE(&pl->songs);
}

size_t playlist_get_current_song(const struct song **song) {
    struct player_playlist *pl = &player.playlist;

    if (song != NULL) {
        if (ARRAY_SIZE(&pl->songs) > 0) {
            *song = ARRAY_AT(&pl->songs, pl->current_song);
        } else {
            *song = NULL;
        }
    }

    return pl->current_song;
}

