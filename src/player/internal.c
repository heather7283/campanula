#include <mpv/stream_cb.h>
#include <mpv/client.h>

#include "player/internal.h"
#include "player/init.h"
#include "player/events.h"
#include "player/playlist.h"
#include "eventloop.h"
#include "log.h"

struct player player = {0};

static enum log_level convert_loglevel(enum mpv_log_level l) {
    switch (l) {
    case MPV_LOG_LEVEL_NONE:  return LOG_QUIET;
    case MPV_LOG_LEVEL_FATAL: return LOG_ERROR;
    case MPV_LOG_LEVEL_ERROR: return LOG_ERROR;
    case MPV_LOG_LEVEL_WARN:  return LOG_WARN;
    case MPV_LOG_LEVEL_INFO:  return LOG_DEBUG;
    case MPV_LOG_LEVEL_V:     return LOG_TRACE;
    case MPV_LOG_LEVEL_DEBUG: return LOG_TRACE;
    case MPV_LOG_LEVEL_TRACE: return LOG_TRACE;
    default: return LOG_QUIET;
    }
}

static const char *property_to_str(enum mpv_format format, const void *data) {
    static char str[64];

    switch (format) {
    case MPV_FORMAT_NONE:
        snprintf(str, sizeof(str), "none");
        break;
    case MPV_FORMAT_INT64:
        snprintf(str, sizeof(str), "i64 %ld", *(int64_t *)data);
        break;
    case MPV_FORMAT_DOUBLE:
        snprintf(str, sizeof(str), "double %f", *(double *)data);
        break;
    case MPV_FORMAT_FLAG:
        snprintf(str, sizeof(str), "flag %d", *(int *)data);
        break;
    case MPV_FORMAT_STRING:
    case MPV_FORMAT_OSD_STRING:
        snprintf(str, sizeof(str), "str %s", (char *)data);
        break;
    case MPV_FORMAT_NODE:
    case MPV_FORMAT_NODE_MAP:
    case MPV_FORMAT_NODE_ARRAY:
    case MPV_FORMAT_BYTE_ARRAY:
        snprintf(str, sizeof(str), "ptr %p", data);
        break;
    default:
        snprintf(str, sizeof(str), "unknown format %d", format);
        break;
    }

    return str;
}

static void process_property_change(const struct mpv_event_property *prop, uint64_t event) {
    if (prop->data == NULL) {
        WARN("mpv event: property %s: data is NULL!", prop->name);
        return;
    } else {
        TRACE("mpv event: property %s: %s", prop->name, property_to_str(prop->format, prop->data));
    }

    switch ((enum player_event)event) {
    case PLAYER_EVENT_PAUSE:
        if (prop->format != MPV_FORMAT_FLAG) {
            WARN("mpv event: property %s: unexpected format!", prop->name);
            break;
        }
        bool pause = *(int *)prop->data;
        signal_emit_bool(&player.emitter, event, pause);
        break;
    case PLAYER_EVENT_PERCENT_POSITION:
        if (prop->format != MPV_FORMAT_INT64) {
            WARN("mpv event: property %s: unexpected format!", prop->name);
            break;
        }
        int64_t percent_pos = *(int64_t *)prop->data;
        signal_emit_i64(&player.emitter, event, percent_pos);
        break;
    case PLAYER_EVENT_PLAYLIST_POSITION:
        if (prop->format != MPV_FORMAT_INT64) {
            WARN("mpv event: property %s: unexpected format!", prop->name);
            break;
        }
        int64_t playlist_pos = *(int64_t *)prop->data;
        signal_emit_i64(&player.emitter, event, playlist_pos);
        break;
    case PLAYER_EVENT_VOLUME:
        if (prop->format != MPV_FORMAT_INT64) {
            WARN("mpv event: property %s: unexpected format!", prop->name);
            break;
        }
        int64_t volume = *(int64_t *)prop->data;
        signal_emit_i64(&player.emitter, event, volume);
        break;
    default:
        WARN("mpv event: property %s not asked for?", prop->name);
        break;
    }
}

static int on_mpv_events(struct pollen_callback *, uint64_t, void *) {
    const struct mpv_event *ev = NULL;
    while ((ev = mpv_wait_event(player.mpv_handle, 0))->event_id != MPV_EVENT_NONE) {
        switch (ev->event_id) {
        case MPV_EVENT_PROPERTY_CHANGE:
            const struct mpv_event_property *p = ev->data;
            process_property_change(p, ev->reply_userdata);
            break;
        case MPV_EVENT_QUEUE_OVERFLOW:
            WARN("mpv event: queue overflow!");
            break;
        case MPV_EVENT_LOG_MESSAGE:
            const struct mpv_event_log_message *m = ev->data;
            log_print(convert_loglevel(m->log_level), "mpv: %s: %s", m->prefix, m->text);
            break;
        default:
            DEBUG("mpv event: %s", mpv_event_name(ev->event_id));
            break;
        }
    }

    return 0;
}

static void mpv_wakeup_callback(void *) {
    pollen_efd_trigger(player.mpv_events_callback, 1);
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
    SET_PROPERTY_STRING_OR_FAIL("audio-client-name", "campanula");
    SET_PROPERTY_STRING_OR_FAIL("title", "${media-title}");
    SET_PROPERTY_STRING_OR_FAIL("keep-open", "yes");

    SET_PROPERTY_STRING_OR_FAIL("input-default-bindings", "yes");
    SET_PROPERTY_STRING_OR_FAIL("input-terminal", "yes");
    SET_PROPERTY_STRING_OR_FAIL("terminal", "yes");
    SET_PROPERTY_STRING_OR_FAIL("really-quiet", "yes");

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
        int ret = mpv_command(player.mpv_handle, (const char *[]){ "quit", NULL });
        if (ret != MPV_ERROR_SUCCESS) {
            ERROR("failed to send quit command to mpv: %s", mpv_error_string(ret));
        }

        mpv_terminate_destroy(player.mpv_handle);
    }

    ARRAY_FOREACH(&player.playlist.songs, i) {
        struct song *s = ARRAY_AT(&player.playlist.songs, i);
        song_free_contents(s);
    }
    ARRAY_FREE(&player.playlist.songs);
}

