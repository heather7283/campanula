#include <fcntl.h>
#include <sys/stat.h>

#include <pollen.h>

#include "network.h"
#include "config.h"
#include "log.h"
#include "api/requests.h"

struct pollen_loop *event_loop;

static void api_callback(const char *errmsg, const struct subsonic_response *response, void *data) {
    if (response == NULL) {
        ERROR("api request failed: %s", errmsg);
        return;
    }

    INFO("got api response, status %d, version %s", response->status, response->version);
    switch (response->inner_object_type) {
    case API_TYPE_SONGS:
        const struct api_type_songs *songs = &response->inner_object.random_songs;
        ARRAY_FOREACH(&songs->song, i) {
            const struct api_type_child *c = &ARRAY_AT(&songs->song, i);
            DEBUG("[%zu] %s/%s/%d. %s", i, c->artist, c->album, c->track, c->title);
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
    log_init(stderr, LOG_TRACE, false);

    char *password = getenv("CAMPANULA_PASSWORD");
    if (password == NULL) {
        ERROR("CAMPANULA_PASSWORD is unset");
        return 1;
    }
    config.password = password;

    event_loop = pollen_loop_create();
    pollen_loop_add_signal(event_loop, SIGINT, sigint_handler, &event_loop);

    if (!curl_init()) {
        return 1;
    }

    api_get_random_songs(5, NULL, 0, 0, NULL, api_callback, NULL);

    pollen_loop_run(event_loop);

    curl_cleanup();
    pollen_loop_cleanup(event_loop);

    return 0;
}

