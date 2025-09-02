#include "player/events.h"
#include "player/internal.h"
#include "player/playlist.h"
#include "api/requests.h"
#include "eventloop.h"
#include "log.h"

void player_event_subscribe(struct signal_listener *listener, enum player_event events,
                            signal_callback_func_t callback, void *callback_data) {
    signal_subscribe(&player.emitter, listener, events, callback, callback_data);
}

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
    }

    #define CHECK_FORMAT(fmt) \
        if (prop->format != MPV_FORMAT_##fmt) { \
            WARN("mpv event: property %s: unexpected format!", prop->name); \
            break; \
        }

    switch ((enum player_event)event) {
    case PLAYER_EVENT_PAUSE:
        CHECK_FORMAT(FLAG);
        const bool pause = *(int *)prop->data;
        player.is_paused = pause;
        signal_emit_bool(&player.emitter, event, pause);
        break;
    case PLAYER_EVENT_PERCENT_POSITION:
        CHECK_FORMAT(INT64);
        const int64_t percent_pos = *(int64_t *)prop->data;
        signal_emit_i64(&player.emitter, event, percent_pos);
        break;
    case PLAYER_EVENT_PLAYLIST_POSITION:
        CHECK_FORMAT(INT64);
        const int64_t playlist_pos = *(int64_t *)prop->data;
        player.playlist.current_song = playlist_pos;
        signal_emit_i64(&player.emitter, event, playlist_pos);
        break;
    case PLAYER_EVENT_VOLUME:
        CHECK_FORMAT(INT64);
        const int64_t volume = *(int64_t *)prop->data;
        signal_emit_i64(&player.emitter, event, volume);
        break;
    case PLAYER_EVENT_MUTE:
        CHECK_FORMAT(FLAG);
        const bool mute = *(int *)prop->data;
        signal_emit_bool(&player.emitter, event, mute);
        break;
    case PLAYER_EVENT_DURATION:
        CHECK_FORMAT(DOUBLE);
        const double duration = *(double *)prop->data;
        const uint64_t duration_ms = duration * 1'000;
        signal_emit_u64(&player.emitter, event, duration_ms);
        break;
    case PLAYER_EVENT_TIME_POSITION:
        CHECK_FORMAT(INT64);
        const uint64_t time_position = *(int64_t *)prop->data;
        const uint64_t time_position_ms = time_position * 1'000;
        signal_emit_u64(&player.emitter, event, time_position_ms);
        break;
    case PLAYER_EVENT_IDLE:
        CHECK_FORMAT(FLAG);
        const bool is_idle = *(int *)prop->data;
        player.is_idle = is_idle;
        signal_emit_bool(&player.emitter, event, is_idle);
        break;
    default:
        WARN("mpv event: property %s not asked for?", prop->name);
        break;
    }

    #undef CHECK_FORMAT
}

void player_process_event(const struct mpv_event *ev) {
    if (ev->event_id != MPV_EVENT_PROPERTY_CHANGE && ev->event_id != MPV_EVENT_LOG_MESSAGE) {
        TRACE("mpv event: %s", mpv_event_name(ev->event_id));
    }

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
    case MPV_EVENT_FILE_LOADED:
        const struct song *s = NULL;
        playlist_get_current_song(&s);
        if (s != NULL) {
            api_scrobble(s->id);
        }
        break;
    case MPV_EVENT_SEEK:
        int64_t pos;
        mpv_get_property(player.mpv_handle, "time-pos/full", MPV_FORMAT_INT64, &pos);
        signal_emit_i64(&player.emitter, PLAYER_EVENT_SEEK, pos);
        break;
    case MPV_EVENT_SHUTDOWN:
        DEBUG("got MPV_EVENT_SHUTDOWN, quitting application");
        pollen_loop_quit(event_loop, 0);
        break;
    default:
    }
}

