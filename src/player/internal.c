#include <mpv/stream_cb.h>
#include <mpv/client.h>

#include "player/internal.h"
#include "player/init.h"
#include "player/events.h"
#include "types/song.h"
#include "eventloop.h"
#include "config.h"
#include "log.h"

struct player player = {0};

static int on_mpv_events(struct pollen_callback *, uint64_t, void *) {
    const struct mpv_event *ev = NULL;
    while ((ev = mpv_wait_event(player.mpv_handle, 0))->event_id != MPV_EVENT_NONE) {
        player_process_event(ev);
    }

    return 0;
}

static void mpv_wakeup_callback(void *) {
    pollen_efd_trigger(player.mpv_events_callback);
}

bool player_init(void) {
    int ret;

    player.mpv_handle = mpv_create();
    if (player.mpv_handle == NULL) {
        ERROR("failed to create mpv handle");
        return false;
    }

    player.mpv_events_callback = pollen_loop_add_efd(event_loop, on_mpv_events, NULL);
    if (player.mpv_events_callback == NULL) {
        ERROR("failed to set up mpv events callback: %m");
        return false;
    }
    mpv_set_wakeup_callback(player.mpv_handle, mpv_wakeup_callback, NULL);

    #define SET_PROPERTY_STRING_OR_FAIL(key, val) \
        do { \
            ret = mpv_set_property_string(player.mpv_handle, (key), (val)); \
            if (ret != MPV_ERROR_SUCCESS) { \
                ERROR("failed to set option %s to %s: %s", (key), (val), mpv_error_string(ret)); \
                return false; \
            } \
        } while (0)

    SET_PROPERTY_STRING_OR_FAIL("vid", "no");
    SET_PROPERTY_STRING_OR_FAIL("video", "no");
    SET_PROPERTY_STRING_OR_FAIL("audio-display", "no");
    SET_PROPERTY_STRING_OR_FAIL("audio-client-name", config.application_name);
    SET_PROPERTY_STRING_OR_FAIL("title", "${media-title}");
    SET_PROPERTY_STRING_OR_FAIL("keep-open", "yes");

    #undef SET_PROPERTY_STRING_OR_FAIL

    ret = mpv_initialize(player.mpv_handle);
    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to initialise player: %s", mpv_error_string(ret));
        return false;
    }

    mpv_request_log_messages(player.mpv_handle, "v");

    #define OBSERVE_PROPERTY_OR_FAIL(event, property, format) \
        do { \
            ret = mpv_observe_property(player.mpv_handle, (event), (property), (format)); \
            if (ret != MPV_ERROR_SUCCESS) { \
                ERROR("failed to observe property %s: %s", (property), mpv_error_string(ret)); \
                return false; \
            } \
        } while (0)

    signal_emitter_init(&player.emitter);
    OBSERVE_PROPERTY_OR_FAIL(PLAYER_EVENT_PLAYLIST_POSITION, "playlist-pos", MPV_FORMAT_INT64);
    OBSERVE_PROPERTY_OR_FAIL(PLAYER_EVENT_PERCENT_POSITION, "percent-pos", MPV_FORMAT_INT64);
    OBSERVE_PROPERTY_OR_FAIL(PLAYER_EVENT_PAUSE, "pause", MPV_FORMAT_FLAG);
    OBSERVE_PROPERTY_OR_FAIL(PLAYER_EVENT_VOLUME, "volume", MPV_FORMAT_INT64);
    OBSERVE_PROPERTY_OR_FAIL(PLAYER_EVENT_MUTE, "mute", MPV_FORMAT_FLAG);
    OBSERVE_PROPERTY_OR_FAIL(PLAYER_EVENT_DURATION, "duration", MPV_FORMAT_INT64);
    OBSERVE_PROPERTY_OR_FAIL(PLAYER_EVENT_TIME_POSITION, "time-pos", MPV_FORMAT_INT64);
    OBSERVE_PROPERTY_OR_FAIL(PLAYER_EVENT_TIME_REMAINING, "time-remaining", MPV_FORMAT_INT64);

    #undef OBSERVE_PROPERTY_OR_FAIL

    ret = mpv_stream_cb_add_ro(player.mpv_handle, MPV_PROTOCOL, NULL, player_stream_open);
    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to register player protocol: %s", mpv_error_string(ret));
        return false;
    }

    return true;
}

void player_cleanup(void) {
    if (player.mpv_handle != NULL) {
        mpv_terminate_destroy(player.mpv_handle);
    }

    VEC_FOREACH(&player.playlist.songs, i) {
        struct song *s = VEC_AT(&player.playlist.songs, i);
        song_free_contents(s);
    }
    VEC_FREE(&player.playlist.songs);
}

