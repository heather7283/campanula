#include "config.h"
#include "log.h"
#include "eventloop.h"
#include "xmalloc.h"
#include "api/network.h"
#include "api/requests.h"
#include "player/init.h"
#include "player/control.h"
#include "player/playlist.h"
#include "player/events.h"
#include "collections/string.h"
#include "db/init.h"
#include "db/populate.h"
#include "db/query.h"
#include "tui/init.h"

static void api_callback(const char *errmsg, const struct subsonic_response *response, void *data) {
    if (response == NULL) {
        ERROR("api request failed: %s", errmsg);
        return;
    }

    switch (response->inner_object_type) {
    case API_TYPE_SONGS: {
        const struct api_type_songs *songs = &response->inner_object.songs;

        const struct api_type_child *c = NULL;
        ARRAY_FOREACH(&songs->song, i) {
            c = ARRAY_AT(&songs->song, i);

            playlist_append_song(&(struct song){
                .id = c->id,
                .title = c->title,
                .artist = c->artist,
            });
        }

        player_set_pause(false);

        break;
    }
    default:
        return;
    }
}

static int sigint_handler(struct pollen_callback *callback, int signum, void *data) {
    pollen_loop_quit(pollen_callback_get_loop(callback), 0);
    return 0;
}

int main(int argc, char **argv) {
    log_init(fopen("campanula.log", "w"), LOG_DEBUG, true);

    char *password = getenv("CAMPANULA_PASSWORD");
    if (password == NULL) {
        ERROR("CAMPANULA_PASSWORD is unset");
        return 1;
    }
    config.password = password;

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

    api_get_random_songs(5, NULL, 0, 0, NULL, api_callback, NULL);

    //db_populate();

    struct album *albums;
    size_t nalbums = db_get_albums(&albums, 2, 10);
    for (size_t i = 0; i < nalbums; i++) {
        struct album *a = &albums[i];
        INFO("%2zu. album \"%s\" (%s)", i, a->name, a->id);

        struct song *songs;
        size_t nsongs = db_get_songs_in_album(&songs, a);
        for (size_t j = 0; j < nsongs; j++) {
            struct song *s = &songs[j];
            INFO("    %2zu. song \"%s\" (%s)", j, s->title, s->id);
            song_free_contents(s);
        }
        free(songs);

        album_free_contents(a);
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

