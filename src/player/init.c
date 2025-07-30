#include <mpv/client.h>
#include <pollen.h>

#include "player/init.h"
#include "player/common.h"
#include "campanula.h"
#include "log.h"

static int player_process_events(struct pollen_callback *, uint64_t, void *) {
    struct mpv_event *ev = NULL;
    while ((ev = mpv_wait_event(player_state.mpv_handle, 0)), ev->event_id != MPV_EVENT_NONE) {
        TRACE("got mpv event: %s", mpv_event_name(ev->event_id));
    }

    return 0;
}

static void mpv_wakeup_callback(void *data) {
    pollen_efd_trigger(player_state.events_callback, 1);
}

bool player_init(void) {
    int ret;

    player_state.mpv_handle = mpv_create();
    if (player_state.mpv_handle == NULL) {
        ERROR("failed to create mpv handle");
        return false;
    }

    player_state.events_callback = pollen_loop_add_efd(event_loop, player_process_events, NULL);
    if (player_state.events_callback == NULL) {
        ERROR("failed to set up efd");
        return false;
    }
    mpv_set_wakeup_callback(player_state.mpv_handle, mpv_wakeup_callback, NULL);

    #define SET_PROPERTY_STRING_OR_FAIL(handle, key, val) \
        do { \
            if ((ret = mpv_set_property_string((handle), (key), (val))) != MPV_ERROR_SUCCESS) { \
                ERROR("failed to set option %s to %s: %s", (key), (val), mpv_error_string(ret)); \
                return false; \
            } \
        } while (0)

    SET_PROPERTY_STRING_OR_FAIL(player_state.mpv_handle, "video", "no");
    SET_PROPERTY_STRING_OR_FAIL(player_state.mpv_handle, "audio-display", "no");
    SET_PROPERTY_STRING_OR_FAIL(player_state.mpv_handle, "ao", "pipewire,");
    SET_PROPERTY_STRING_OR_FAIL(player_state.mpv_handle, "audio-client-name", "campanula");

    #undef SET_PROPERTY_STRING_OR_FAIL

    ret = mpv_initialize(player_state.mpv_handle);
    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to initialise player: %s", mpv_error_string(ret));
        return false;
    }

    return true;
}

void player_cleanup(void) {
    if (player_state.mpv_handle != NULL) {
        mpv_terminate_destroy(player_state.mpv_handle);
    }
    pollen_loop_remove_callback(player_state.events_callback);
}

