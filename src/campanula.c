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

struct playback_data {
    int64_t pos, volume;
    bool pause, mute;
};

static void print_status_bar(const struct playback_data *d) {
    static struct string str = {0};
    string_clear(&str);

    string_append(&str, "\r");
    string_append(&str, d->pause ? "|| " : "|> ");
    if (d->mute) {
        string_append(&str, "VOL MUTE ");
    } else {
        string_appendf(&str, "VOL %3li%% ", d->volume);
    }
    string_appendf(&str, "POS %3li%%", d->pos);

    fputs(str.str, stdout);
    fflush(stdout);
}

static void on_playlist_position(uint64_t, const struct signal_data *data, void *userdata) {
    struct playback_data *d = userdata;
    d->pos = data->as.i64;

    printf("\nPlaylist pos: %li\n", d->pos);

    const struct song *songs;
    const size_t song_count = playlist_get_songs(&songs);
    for (int64_t i = 0; (size_t)i < song_count; i++) {
        const bool current = (i == d->pos);
        printf("%s%s%li. %s - %s%s\n",
               current ? "\033[1m" : "",
               current ? "> " : "  ",
               i, songs[i].artist, songs[i].title,
               current ? "\033[m" : "");
    }

    print_status_bar(d);
}

static void on_percent_position(uint64_t, const struct signal_data *data, void *userdata) {
    struct playback_data *d = userdata;
    d->pos = data->as.i64;

    print_status_bar(d);
}

static void on_pause(uint64_t, const struct signal_data *data, void *userdata) {
    struct playback_data *d = userdata;
    d->pause = data->as.boolean;

    print_status_bar(d);
}

static void on_volume(uint64_t, const struct signal_data *data, void *userdata) {
    struct playback_data *d = userdata;
    d->volume = data->as.i64;

    print_status_bar(d);
}

static void on_mute(uint64_t, const struct signal_data *data, void *userdata) {
    struct playback_data *d = userdata;
    d->mute = data->as.boolean;

    print_status_bar(d);
}

static void print_album(const struct album *a, enum log_level lvl) {
    log_println(lvl, "Album %s (%s) {", a->name, a->id);
    log_println(lvl, "    artist: %s (%s)", a->artist, a->artist_id);
    log_println(lvl, "    song_count: %d", a->song_count);
    log_println(lvl, "    duration: %d", a->duration);
    log_println(lvl, "}");
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

    if (!db_init()) {
        return 1;
    }
    if (!network_init()) {
        return 1;
    }
    if (!player_init()) {
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

    struct playback_data d = {0};
    struct signal_listener l1, l2, l3, l4, l5;
    player_event_subscribe(&l1, PLAYER_EVENT_PLAYLIST_POSITION, on_playlist_position, &d);
    player_event_subscribe(&l2, PLAYER_EVENT_PERCENT_POSITION, on_percent_position, &d);
    player_event_subscribe(&l3, PLAYER_EVENT_PAUSE, on_pause, &d);
    player_event_subscribe(&l4, PLAYER_EVENT_VOLUME, on_volume, &d);
    player_event_subscribe(&l5, PLAYER_EVENT_MUTE, on_mute, &d);

    pollen_loop_run(event_loop);

    player_cleanup();
    network_cleanup();
    db_cleanup();
    pollen_loop_cleanup(event_loop);

    return 0;
}

