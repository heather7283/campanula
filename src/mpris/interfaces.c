#include <stddef.h>

#include "mpris/interfaces.h"
#include "player/control.h"
#include "log.h"

typedef uint32_t dbus_bool;

enum playback_status {
    PLAYBACK_STATUS_PLAYING,
    PLAYBACK_STATUS_PAUSED,
    PLAYBACK_STATUS_STOPPED,
};

static const char *playback_status_values[] = {
    [PLAYBACK_STATUS_PLAYING] = "Playing",
    [PLAYBACK_STATUS_PAUSED] = "Paused",
    [PLAYBACK_STATUS_STOPPED] = "Stopped",
};

enum loop_status {
    LOOP_STATUS_NONE,
    LOOP_STATUS_TRACK,
    LOOP_STATUS_PLAYLIST,
};

static const char *loop_status_values[] = {
    [LOOP_STATUS_NONE] = "None",
    [LOOP_STATUS_TRACK] = "Track",
    [LOOP_STATUS_PLAYLIST] = "Playlist",
};

static struct player_interface {
    enum playback_status playback_status;
    enum loop_status loop_status; /* rw */
    double rate; /* rw */
    dbus_bool shuffle; /* rw */
    /* a{sv} metadata */
    double volume; /* rw */
    int64_t position;

    double minimum_rate;
    double maximum_rate;

    dbus_bool can_go_next;
    dbus_bool can_go_previous;
    dbus_bool can_play;
    dbus_bool can_pause;
    dbus_bool can_seek;
    dbus_bool can_control;
} player_interface = {
    .playback_status = PLAYBACK_STATUS_STOPPED,
    .loop_status = LOOP_STATUS_NONE,
    .rate = 1,
    .volume = 1,
    .minimum_rate = 1,
    .maximum_rate = 1,
    .can_control = true,
};

static int player_property_playback_status_get(sd_bus *bus, const char *path, const char *interface,
                                               const char *property, sd_bus_message *reply,
                                               void *userdata, sd_bus_error *ret_error) {
    const char *v = playback_status_values[player_interface.playback_status];
    return sd_bus_message_append_basic(reply, 's', v);
}

static int player_property_loop_status_get(sd_bus *bus, const char *path, const char *interface,
                                           const char *property, sd_bus_message *reply,
                                           void *userdata, sd_bus_error *ret_error) {
    const char *v = loop_status_values[player_interface.loop_status];
    return sd_bus_message_append_basic(reply, 's', v);
}

static int player_property_loop_status_set(sd_bus *bus, const char *path, const char *interface,
                                           const char *property, sd_bus_message *reply,
                                           void *userdata, sd_bus_error *ret_error) {
    return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_NOT_SUPPORTED, "TODO");
}

static int player_property_rate_set(sd_bus *bus, const char *path, const char *interface,
                                    const char *property, sd_bus_message *reply,
                                    void *userdata, sd_bus_error *ret_error) {
    return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_NOT_SUPPORTED, "Not supported");
}

static int player_property_shuffle_set(sd_bus *bus, const char *path, const char *interface,
                                       const char *property, sd_bus_message *reply,
                                       void *userdata, sd_bus_error *ret_error) {
    return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_NOT_SUPPORTED, "TODO");
}

static int player_property_metadata_get(sd_bus *bus, const char *path, const char *interface,
                                        const char *property, sd_bus_message *reply,
                                        void *userdata, sd_bus_error *ret_error) {
    return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_NOT_SUPPORTED, "TODO");
}

static int player_property_volume_set(sd_bus *bus, const char *path, const char *interface,
                                        const char *property, sd_bus_message *reply,
                                        void *userdata, sd_bus_error *ret_error) {
    return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_NOT_SUPPORTED, "TODO");
}

static const struct sd_bus_vtable player_vtable[] = {
    SD_BUS_VTABLE_START(SD_BUS_VTABLE_UNPRIVILEGED),

    SD_BUS_PROPERTY("PlaybackStatus", "s",
                    player_property_playback_status_get,
                    0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_WRITABLE_PROPERTY("LoopStatus", "s",
                             player_property_loop_status_get, player_property_loop_status_set,
                             0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_WRITABLE_PROPERTY("Rate", "d",
                             NULL, player_property_rate_set,
                             offsetof(struct player_interface, rate),
                             SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_WRITABLE_PROPERTY("Shuffle", "d",
                             NULL, player_property_shuffle_set,
                             offsetof(struct player_interface, shuffle),
                             SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("Metadata", "a{sv}", player_property_metadata_get,
                    0, SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_WRITABLE_PROPERTY("Volume", "d",
                             NULL, player_property_volume_set,
                             offsetof(struct player_interface, volume),
                             SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("Position", "x", NULL,
                    offsetof(struct player_interface, position),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("MinimumRate", "d", NULL,
                    offsetof(struct player_interface, minimum_rate),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("MaximumRate", "d", NULL,
                    offsetof(struct player_interface, maximum_rate),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("CanGoNext", "b", NULL,
                    offsetof(struct player_interface, can_go_next),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("CanGoPrevious", "b", NULL,
                    offsetof(struct player_interface, can_go_previous),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("CanPlay", "b", NULL,
                    offsetof(struct player_interface, can_play),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("CanPause", "b", NULL,
                    offsetof(struct player_interface, can_pause),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("CanSeek", "b", NULL,
                    offsetof(struct player_interface, can_seek),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("CanControl", "b", NULL,
                    offsetof(struct player_interface, can_control),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),

    SD_BUS_VTABLE_END
};

static struct mpris_interface {
    dbus_bool can_quit;
    dbus_bool fullscreen; /* rw */
    dbus_bool can_set_fullscreen;
    dbus_bool can_raise;
    dbus_bool has_track_list;

    char *identity;
    char *desktop_entry;

    char *supported_uri_schemes[1];
    char *supported_mime_types[1];
} mpris_interface = {
    .can_quit = false,
    .fullscreen = false,
    .can_set_fullscreen = false,
    .can_raise = false,
    .has_track_list = false,

    .identity = "campanula",
    .desktop_entry = "campanula",

    .supported_uri_schemes = { NULL },
    .supported_mime_types = { NULL },
};

static int mpris_method_raise(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    return sd_bus_reply_method_errorf(m, SD_BUS_ERROR_NOT_SUPPORTED,
                                      "You cannot raise a terminal application");
}

static int mpris_method_quit(sd_bus_message *m, void *userdata, sd_bus_error *ret_error) {
    player_quit();
    return sd_bus_reply_method_return(m, NULL);
}

static int mpris_property_fullscreen_set(sd_bus *bus, const char *path, const char *interface,
                                         const char *property, sd_bus_message *reply,
                                         void *userdata, sd_bus_error *ret_error) {
    return sd_bus_reply_method_errorf(reply, SD_BUS_ERROR_NOT_SUPPORTED,
                                      "You cannot fullscreen a terminal application");
}

static const struct sd_bus_vtable mpris_vtable[] = {
    SD_BUS_VTABLE_START(SD_BUS_VTABLE_UNPRIVILEGED),

    SD_BUS_METHOD("Raise", "", "", mpris_method_raise, 0),
    SD_BUS_METHOD("Quit", "", "", mpris_method_quit, 0),

    SD_BUS_PROPERTY("CanQuit", "b", NULL,
                    offsetof(struct mpris_interface, can_quit),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_WRITABLE_PROPERTY("Fullscreen", "b", NULL, mpris_property_fullscreen_set,
                             offsetof(struct mpris_interface, fullscreen),
                             SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("CanSetFullscreen", "b", NULL,
                    offsetof(struct mpris_interface, can_set_fullscreen),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("CanRaise", "b", NULL,
                    offsetof(struct mpris_interface, can_raise),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("HasTrackList", "b", NULL,
                    offsetof(struct mpris_interface, has_track_list),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("Identity", "s", NULL,
                    offsetof(struct mpris_interface, identity),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("DesktopEntry", "s", NULL,
                    offsetof(struct mpris_interface, desktop_entry),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("SupportedUriSchemes", "as", NULL,
                    offsetof(struct mpris_interface, supported_uri_schemes),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),
    SD_BUS_PROPERTY("SupportedMimeTypes", "as", NULL,
                    offsetof(struct mpris_interface, supported_mime_types),
                    SD_BUS_VTABLE_PROPERTY_EMITS_CHANGE),

    SD_BUS_VTABLE_END
};

bool mpris_init_interfaces(struct dbus_state *dbus_state) {
    int ret;

    ret = sd_bus_add_object_vtable(dbus_state->bus, &dbus_state->mpris_vtable_slot,
                                   "/org/mpris/MediaPlayer2",
                                   "org.mpris.MediaPlayer2",
                                   mpris_vtable, &mpris_interface);
    if (ret < 0) {
        ERROR("dbus: failed to add MPRIS vtable: %s", strerror(-ret));
        goto err;
    }

    ret = sd_bus_add_object_vtable(dbus_state->bus, &dbus_state->player_vtable_slot,
                                   "/org/mpris/MediaPlayer2",
                                   "org.mpris.MediaPlayer2.Player",
                                   player_vtable, &player_interface);
    if (ret < 0) {
        ERROR("dbus: failed to add Player vtable: %s", strerror(-ret));
        goto err;
    }

    return true;

err:
    return false;
}

