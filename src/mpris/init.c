#include <errno.h>
#include <poll.h>
#include <unistd.h>

#include "mpris/init.h"
#include "mpris/dbus.h"
#include "mpris/interfaces.h"
#include "collections/string.h"
#include "player/control.h"
#include "player/events.h"
#include "player/playlist.h"
#include "eventloop.h"
#include "log.h"

struct dbus_state dbus = {
    .bus_fd = -1,
};

static struct signal_listener listener = {0};

static void process_player_events(uint64_t event, const struct signal_data *data, void *) {
    switch ((enum player_event)event) {
    case PLAYER_EVENT_PLAYLIST_POSITION: {
        const struct song *song;
        playlist_get_current_song(&song);
        mpris_update_metadata(song);
    }
    case PLAYER_EVENT_PLAYLIST_SONG_ADDED:
    case PLAYER_EVENT_PLAYLIST_SONG_REMOVED:
    case PLAYER_EVENT_PLAYLIST_CLEARED: {
        const struct song *songs;
        const size_t nsongs = playlist_get_songs(&songs);
        const ssize_t current_song = playlist_get_current_song(NULL);
        mpris_update_playlist_stuff(songs, nsongs, current_song);
        break;
    }
    case PLAYER_EVENT_IDLE:
        const bool idle = data->as.boolean;
        if (idle) {
            mpris_update_playback_status(PLAYBACK_STATUS_STOPPED);
        } else {
            mpris_update_playback_status(player_is_paused()
                                         ? PLAYBACK_STATUS_PAUSED
                                         : PLAYBACK_STATUS_PLAYING);
        }
        break;
    case PLAYER_EVENT_PAUSE:
        const bool paused = data->as.boolean;
        if (!player_is_idle()) {
            mpris_update_playback_status(paused
                                         ? PLAYBACK_STATUS_PAUSED
                                         : PLAYBACK_STATUS_PLAYING);
        }
        break;
    case PLAYER_EVENT_TIME_POSITION:
        const uint64_t pos_milliseconds = data->as.u64;
        mpris_update_position(pos_milliseconds * 1'000);
        break;
    case PLAYER_EVENT_SEEK:
        const uint64_t new_pos_milliseconds = data->as.u64;
        mpris_emit_seek(new_pos_milliseconds * 1'000);
        break;
    default:
    }
}

static uint32_t poll_events_to_epoll_events(int poll_events) {
    if (poll_events == 0) {
        return POLLHUP | POLLERR | POLLNVAL;
    }

    uint32_t epoll_events = 0;

    #define CONVERT(event) \
        do { if (poll_events & event) { epoll_events |= E##event; } } while (0)

    CONVERT(POLLIN);
    CONVERT(POLLPRI);
    CONVERT(POLLOUT);
    CONVERT(POLLRDHUP);
    CONVERT(POLLERR);

    #undef CONVERT

    return epoll_events;
}

static int dbus_fd_handler(struct pollen_callback *, int, uint32_t, void *);
static int dbus_timer_handler(struct pollen_callback *, void *);

static int dbus_process_events(void) {
    int ret;
    do {
        ret = sd_bus_process(dbus.bus, NULL);
    } while (ret > 0);
    if (ret < 0) {
        if (ret == -EBUSY) {
            WARN("sd_bus_process returned EBUSY (why does this happen?)");
        } else {
            ERROR("failed to process dbus events: %s", strerror(-ret));
            return ret;
        }
    }

    int new_fd = sd_bus_get_fd(dbus.bus);
    uint32_t new_events = poll_events_to_epoll_events(sd_bus_get_events(dbus.bus));
    uint64_t new_timeout_us; sd_bus_get_timeout(dbus.bus, &new_timeout_us);

    if (new_fd != dbus.bus_fd) {
        pollen_loop_remove_callback(dbus.bus_fd_callback);
        pollen_loop_add_fd(event_loop, new_fd, new_events, false, dbus_fd_handler, NULL);
    } else if (new_events != dbus.events) {
        pollen_fd_modify_events(dbus.bus_fd_callback, new_events);
    }
    dbus.bus_fd = new_fd;
    dbus.events = new_events;

    if (new_timeout_us == UINT64_MAX) {
        if (dbus.timer_armed) {
            pollen_timer_disarm(dbus.bus_timer_callback);
            dbus.timer_armed = false;
        }
    } else if (new_timeout_us == 0) {
        /* wake up immediately */
        pollen_timer_arm_ns(dbus.bus_timer_callback, false, 1, 0);
        dbus.timer_armed = true;
    } else {
        pollen_timer_arm_us(dbus.bus_timer_callback, true, new_timeout_us, 0);
        dbus.timer_armed = true;
    }

    return 0;
}

static int dbus_fd_handler(struct pollen_callback *, int, uint32_t, void *) {
    return dbus_process_events();
}

static int dbus_timer_handler(struct pollen_callback *, void *) {
    return dbus_process_events();
}

bool mpris_init(void) {
    [[gnu::cleanup(string_free)]] struct string bus_name = {0};
    int ret;

    if ((ret = sd_bus_open_user(&dbus.bus)) < 0) {
        ERROR("dbus: failed to connect to user bus: %s", strerror(-ret));
        goto err;
    }
    INFO("connected to dbus");

    string_appendf(&bus_name, "org.mpris.MediaPlayer2.campanula.instance%d", getpid());
    ret = sd_bus_request_name(dbus.bus, bus_name.str, 0);
    if (ret < 0) {
        ERROR("dbus: failed to acquire name %s: %s", bus_name.str, strerror(-ret));
        goto err;
    }
    INFO("acquired dbus name %s", bus_name.str);

    if (!mpris_init_interfaces(&dbus)) {
        goto err;
    }

    dbus.bus_fd = sd_bus_get_fd(dbus.bus);
    dbus.bus_fd_callback = pollen_loop_add_fd(event_loop, dbus.bus_fd, 0, false,
                                              dbus_fd_handler, NULL);

    dbus.bus_timer_callback = pollen_loop_add_timer(event_loop, CLOCK_MONOTONIC,
                                                    dbus_timer_handler, NULL);

    dbus_process_events();

    player_event_subscribe(&listener, (uint64_t)-1, process_player_events, NULL);

    return true;

err:
    mpris_cleanup();
    return false;
}

void mpris_cleanup(void) {
    if (dbus.bus_timer_callback != NULL) {
        pollen_loop_remove_callback(dbus.bus_timer_callback);
        dbus.bus_timer_callback = NULL;
    }
    if (dbus.bus_fd_callback != NULL) {
        pollen_loop_remove_callback(dbus.bus_fd_callback);
        dbus.bus_fd_callback = NULL;
    }

    if (dbus.mpris_vtable_slot != NULL) {
        sd_bus_slot_unref(dbus.mpris_vtable_slot);
        dbus.mpris_vtable_slot = NULL;
    }
    if (dbus.player_vtable_slot != NULL) {
        sd_bus_slot_unref(dbus.player_vtable_slot);
        dbus.player_vtable_slot = NULL;
    }

    if (dbus.bus != NULL) {
        sd_bus_flush(dbus.bus);
        sd_bus_close(dbus.bus);
        sd_bus_unref(dbus.bus);
        dbus.bus = NULL;
        dbus.bus_fd = -1;
    }
}

