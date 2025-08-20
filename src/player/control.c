#include <mpv/client.h>

#include "player/control.h"
#include "player/internal.h"
#include "collections/string.h"
#include "types/song.h"
#include "log.h"

void player_set_pause(bool pause) {
    int p = pause;
    int ret = mpv_set_property(player.mpv_handle, "pause", MPV_FORMAT_FLAG, &p);
    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to %s the player: %s", pause ? "pause" : "unpause", mpv_error_string(ret));
    }
}

void player_toggle_pause(void) {
    int ret = mpv_command(player.mpv_handle, (const char *[]){
        "cycle-values", "pause", "yes", "no", NULL
    });
    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to toggle pause: %s", mpv_error_string(ret));
    }
}

void player_seek(long off, bool relative) {
    struct mpv_node node = {
        .format = MPV_FORMAT_NODE_MAP,
        .u.list = &(struct mpv_node_list){
            .num = 3,
            .keys = (char *[]){ "name", "target", "flags" },
            .values = (struct mpv_node[]){
                { .format = MPV_FORMAT_STRING, .u.string = "seek" },
                { .format = MPV_FORMAT_INT64, .u.int64 = off },
                { .format = MPV_FORMAT_STRING, .u.string = relative ? "relative" : "absolute" },
            },
        },
    };

    struct mpv_node n; /* remove this once my fix makes it into mpv release */
    int ret = mpv_command_node(player.mpv_handle, &node, &n);
    mpv_free_node_contents(&n);

    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to seek: %s", mpv_error_string(ret));
    }
}

void player_set_volume(int volume, bool relative) {
    struct mpv_node node = {
        .format = MPV_FORMAT_NODE_ARRAY,
        .u.list = &(struct mpv_node_list){
            .num = 3,
            .values = (struct mpv_node[]){
                { .format = MPV_FORMAT_STRING, .u.string = relative ? "add" : "set" },
                { .format = MPV_FORMAT_STRING, .u.string = "volume" },
                { .format = MPV_FORMAT_INT64, .u.int64 = volume },
            },
        },
    };

    struct mpv_node n; /* remove this once my fix makes it into mpv release */
    int ret = mpv_command_node(player.mpv_handle, &node, &n);
    mpv_free_node_contents(&n);

    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to set volume: %s", mpv_error_string(ret));
    }
}

void player_toggle_mute(void) {
    int ret = mpv_command(player.mpv_handle, (const char *[]){
        "cycle-values", "mute", "yes", "no", NULL
    });
    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to toggle pause: %s", mpv_error_string(ret));
    }
}

void player_next(void) {
    int ret = mpv_command(player.mpv_handle, (const char *[]){ "playlist-next", NULL });
    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("playlist-next failed: %s", mpv_error_string(ret));
    }
}

void player_prev(void) {
    int ret = mpv_command(player.mpv_handle, (const char *[]){ "playlist-prev", NULL });
    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("playlist-prev failed: %s", mpv_error_string(ret));
    }
}

void player_play_nth(int index) {
    int64_t i = index;
    int ret = mpv_set_property(player.mpv_handle, "playlist-pos", MPV_FORMAT_INT64, &i);
    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to set playlist-pos to %li: %s", i, mpv_error_string(ret));
    }
}

bool player_loadfile(const struct song *song) {
    struct string url = {0};
    string_appendf(&url, "%s://%s", MPV_PROTOCOL, song->id);

    struct string title = {0};
    string_appendf(&title, "%s - %s",
                   song->artist ? song->artist : "Unknown",
                   song->title ? song->title : "Untitled");

    struct mpv_node node = {
        .format = MPV_FORMAT_NODE_MAP,
        .u.list = &(struct mpv_node_list){
            .num = 5,
            .keys = (char *[]){ "name", "url", "flags", "index", "options", },
            .values = (struct mpv_node[]){
                { .format = MPV_FORMAT_STRING, .u.string = "loadfile" },
                { .format = MPV_FORMAT_STRING, .u.string = url.str },
                { .format = MPV_FORMAT_STRING, .u.string = "append-play" },
                { .format = MPV_FORMAT_INT64, .u.int64 = -1 },
                {
                    .format = MPV_FORMAT_NODE_MAP,
                    .u.list = &(struct mpv_node_list){
                        .num = 1,
                        .keys = (char *[]){ "force-media-title", },
                        .values = (struct mpv_node[]){
                            { .format = MPV_FORMAT_STRING, .u.string = title.str },
                        },
                    },
                },
            },
        },
    };

    struct mpv_node n;
    int ret = mpv_command_node(player.mpv_handle, &node, &n);
    mpv_free_node_contents(&n);

    string_free(&url);
    string_free(&title);

    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to load song: %s", mpv_error_string(ret));
        return false;
    }

    return true;
}

bool player_stop(void) {
    int ret = mpv_command(player.mpv_handle, (const char *[]){ "stop" });
    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to clear playlist (stop cmd): %s", mpv_error_string(ret));
        return false;
    }

    return true;
}

bool player_quit(void) {
    int ret = mpv_command(player.mpv_handle, (const char *[]){ "quit" });
    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to quit player: %s", mpv_error_string(ret));
        return false;
    }

    return true;
}

