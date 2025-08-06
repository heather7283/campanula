#include <mpv/client.h>

#include "player/control.h"
#include "player/internal.h"
#include "collections/string.h"
#include "log.h"

void player_set_pause(bool pause) {
    int p = pause;
    int ret = mpv_set_property(player.mpv_handle, "pause", MPV_FORMAT_FLAG, &p);
    if (ret != MPV_ERROR_SUCCESS) {
        ERROR("failed to %s the player: %s", pause ? "pause" : "unpause", mpv_error_string(ret));
    }
}

bool player_load_song(const struct song *song) {
    struct string url = {0};
    string_appendf(&url, "%s://%s", MPV_PROTOCOL, song->id);

    struct string title = {0};
    string_appendf(&title, "%s - %s",
                   song->artist ? song->artist : "Unknown",
                   song->title ? song->title : "Untitled");

    struct mpv_node node = {
        .format = MPV_FORMAT_NODE_MAP,
        .u.list = &(struct mpv_node_list){
            .num = 3,
            .keys = (char *[]){ "name", "url", "flags", "index", "options", },
            .values = (struct mpv_node[]){
                (struct mpv_node){ .format = MPV_FORMAT_STRING, .u.string = "loadfile" },
                (struct mpv_node){ .format = MPV_FORMAT_STRING, .u.string = url.str },
                (struct mpv_node){ .format = MPV_FORMAT_STRING, .u.string = "append-play" },
                (struct mpv_node){ .format = MPV_FORMAT_INT64, .u.int64 = -1 },
                (struct mpv_node){
                    .format = MPV_FORMAT_NODE_MAP,
                    .u.list = &(struct mpv_node_list){
                        .num = 1,
                        .keys = (char *[]) { "force-media-title", },
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

