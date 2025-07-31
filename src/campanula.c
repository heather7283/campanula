#include <fcntl.h>
#include <sys/stat.h>

#include <pollen.h>

#include "config.h"
#include "log.h"
#include "eventloop.h"
#include "xmalloc.h"
#include "api/network.h"
#include "api/requests.h"
#include "player/init.h"
#include "player/control.h"

static void api_callback(const char *errmsg, const struct subsonic_response *response, void *data) {
    if (response == NULL) {
        ERROR("api request failed: %s", errmsg);
        return;
    }

    INFO("Got api response, status %d, version %s", response->status, response->version);
    switch (response->inner_object_type) {
    case API_TYPE_SONGS: {
        const struct api_type_songs *songs = &response->inner_object.random_songs;
        DEBUG("Got %zu songs:", ARRAY_SIZE(&songs->song));

        const struct api_type_child *c = NULL;
        ARRAY_FOREACH(&songs->song, i) {
            c = &ARRAY_AT(&songs->song, i);
            DEBUG("%zu. %s (%s) / %s (%s) / %d. %s (%s)",
                  i, c->artist, c->artist_id, c->album, c->album_id, c->track, c->title, c->id);
        }

        player_play(c->id);

        break;
    }
    default:
        return;
    }
}

int sigint_handler(struct pollen_callback *callback, int signum, void *data) {
    pollen_loop_quit(pollen_callback_get_loop(callback), 0);
    return 0;
}

int main(int argc, char **argv) {
    log_init(fopen("campanula.log", "w"), LOG_TRACE, true);

    char *password = getenv("CAMPANULA_PASSWORD");
    if (password == NULL) {
        ERROR("CAMPANULA_PASSWORD is unset");
        return 1;
    }
    config.password = password;

    event_loop = pollen_loop_create();
    pollen_loop_add_signal(event_loop, SIGINT, sigint_handler, &event_loop);

    if (!network_init()) {
        return 1;
    }
    if (!player_init()) {
        return 1;
    }

    api_get_random_songs(5, NULL, 0, 0, NULL, api_callback, NULL);

    pollen_loop_run(event_loop);

    player_cleanup();
    network_cleanup();
    pollen_loop_cleanup(event_loop);

    return 0;
}

