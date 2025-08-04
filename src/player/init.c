#include <mpv/client.h>
#include <mpv/stream_cb.h>

#include "player/init.h"
#include "player/common.h"
#include "player/stream.h"
#include "eventloop.h"
#include "log.h"

static enum log_level mpv_log_level_to_log_loglevel(enum mpv_log_level l) {
    switch (l) {
    case MPV_LOG_LEVEL_NONE : return LOG_QUIET;
    case MPV_LOG_LEVEL_FATAL: return LOG_ERROR;
    case MPV_LOG_LEVEL_ERROR: return LOG_ERROR;
    case MPV_LOG_LEVEL_WARN : return LOG_WARN;
    case MPV_LOG_LEVEL_INFO : return LOG_DEBUG;
    case MPV_LOG_LEVEL_V    : return LOG_DEBUG;
    case MPV_LOG_LEVEL_DEBUG: return LOG_TRACE;
    case MPV_LOG_LEVEL_TRACE: return LOG_TRACE;
    default: return LOG_QUIET;
    }
}

static void log_mpv_event(const struct mpv_event *ev) {
    switch (ev->event_id) {
    case MPV_EVENT_LOG_MESSAGE:
        const struct mpv_event_log_message *m = ev->data;
        log_print(mpv_log_level_to_log_loglevel(m->log_level),
                  "mpv: %s: %s", m->prefix, m->text);
        break;
    case MPV_EVENT_END_FILE:
        const struct mpv_event_end_file *ef = ev->data;
        if (ef->reason == MPV_END_FILE_REASON_ERROR) {
            ERROR("mpv: EOF: %s", mpv_error_string(ef->error));
        } else {
            TRACE("mpv: EOF (%d)", ef->reason);
        }
        break;
    default:
        TRACE("mpv: %s", mpv_event_name(ev->event_id));
        break;
    }
}

static int player_process_events(struct pollen_callback *, uint64_t, void *) {
    struct mpv_event *ev = NULL;
    while ((ev = mpv_wait_event(player_state.mpv_handle, 0)), ev->event_id != MPV_EVENT_NONE) {
        log_mpv_event(ev);
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

    SET_PROPERTY_STRING_OR_FAIL(player_state.mpv_handle, "vid", "no");
    SET_PROPERTY_STRING_OR_FAIL(player_state.mpv_handle, "video", "no");
    SET_PROPERTY_STRING_OR_FAIL(player_state.mpv_handle, "audio-display", "no");
    SET_PROPERTY_STRING_OR_FAIL(player_state.mpv_handle, "audio-client-name", "campanula");
    SET_PROPERTY_STRING_OR_FAIL(player_state.mpv_handle, "title",
                                /* "${metadata/by-key/Track:}${?metadata/by-key/Track:. }" */
                                "${metadata/by-key/Artist:}${?metadata/by-key/Artist: - }"
                                "${media-title}");

    SET_PROPERTY_STRING_OR_FAIL(player_state.mpv_handle, "input-default-bindings", "yes");
    SET_PROPERTY_STRING_OR_FAIL(player_state.mpv_handle, "input-terminal", "yes");
    SET_PROPERTY_STRING_OR_FAIL(player_state.mpv_handle, "terminal", "yes");
    SET_PROPERTY_STRING_OR_FAIL(player_state.mpv_handle, "keep-open", "yes");

    #undef SET_PROPERTY_STRING_OR_FAIL

    ret = mpv_initialize(player_state.mpv_handle);
    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to initialise player: %s", mpv_error_string(ret));
        return false;
    }

    mpv_request_log_messages(player_state.mpv_handle, "v");

    ret = mpv_stream_cb_add_ro(player_state.mpv_handle, PLAYER_PROTOCOL, NULL, player_stream_open);
    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to register player protocol: %s", mpv_error_string(ret));
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

