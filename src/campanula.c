#include "config.h"
#include "log.h"
#include "eventloop.h"
#include "xmalloc.h"
#include "api/network.h"
#include "player/init.h"
#include "player/playlist.h"
#include "db/init.h"
#include "db/populate.h"
#include "db/query.h"
#include "tui/init.h"

static int sigint_handler(struct pollen_callback *callback, int signum, void *data) {
    pollen_loop_quit(pollen_callback_get_loop(callback), 0);
    return 0;
}

int main(int argc, char **argv) {
    log_init(stderr, LOG_TRACE, false);

    if (!load_config()) {
        return 1;
    }

    log_init(fopen("campanula.log", "w"), LOG_DEBUG, true);

    event_loop = pollen_loop_create();
    pollen_loop_add_signal(event_loop, SIGINT, sigint_handler, &event_loop);

    if (!db_init()) {
        return 1;
    }
    if (!network_init()) {
        return 1;
    }
    if (!player_init()) {
        return 1;
    }
    if (!tui_init()) {
        return 1;
    }

    //db_populate();

    struct album *albums;
    size_t nalbums = db_search_albums(&albums, "stratocaster", 0, 10);
    for (size_t i = 0; i < nalbums; i++) {
        struct album *a = &albums[i];
        INFO("%2zu. album \"%s\" (%s)", i, a->name, a->id);

        struct song *songs;
        size_t nsongs = db_get_songs_in_album(&songs, a);
        for (size_t j = 0; j < nsongs; j++) {
            struct song *s = &songs[j];
            INFO("    %2zu. song \"%s\" (%s)", j, s->title, s->id);

            playlist_append_song(s);

            song_free_contents(s);
        }
        free(songs);

        album_free_contents(a);

        break;
    }
    free(albums);

    pollen_loop_run(event_loop);

    tui_cleanup();
    player_cleanup();
    network_cleanup();
    db_cleanup();
    pollen_loop_cleanup(event_loop);

    return 0;
}

