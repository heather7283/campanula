#include "config.h"
#include "log.h"
#include "eventloop.h"
#include "network/init.h"
#include "player/init.h"
#include "player/playlist.h"
#include "player/control.h"
#include "db/init.h"
#include "db/populate.h"
#include "db/query.h"
#include "tui/init.h"
#include "mpris/init.h"

static int sigint_handler(struct pollen_callback *callback, int signum, void *data) {
    player_quit();
    return 0;
}

int main(int argc, char **argv) {
    log_init(stderr, LOG_TRACE, false);

    if (!load_config()) {
        return 1;
    }

    log_init(fopen("campanula.log", "w"), LOG_TRACE, true);

    if (!db_init()) {
        return 1;
    }
    /* retrieve server id right after connecting to the db to avoid surprises later */
    int64_t server_id = db_get_server_id(config.server_address);
    if (server_id < 0) {
        server_id = db_add_server(config.server_address);
    }
    if (server_id < 0) {
        return 1;
    }
    config.server_id = server_id;

    event_loop = pollen_loop_create();
    pollen_loop_add_signal(event_loop, SIGINT, sigint_handler, &event_loop);

    if (!network_init()) {
        return 1;
    }
    if (!player_init()) {
        return 1;
    }
    if (!tui_init()) {
        return 1;
    }
    mpris_init();

    pollen_loop_run(event_loop);

    mpris_cleanup();
    tui_cleanup();
    player_cleanup();
    network_cleanup();
    db_cleanup();
    pollen_loop_cleanup(event_loop);

    return 0;
}

