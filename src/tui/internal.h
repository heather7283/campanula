#ifndef SRC_TUI_INTERNAL_H
#define SRC_TUI_INTERNAL_H

#include <ncurses.h>

#include "tui/pad.h"
#include "tui/list.h"
#include "signals.h"

#define STATUSBAR_HEIGHT 4

enum {
    NET_SPEED_DL = 0,
    NET_SPEED_UL = 1,
};

enum tui_tab {
    TUI_TAB_PLAYLIST,
    TUI_TAB_ARTISTS,
    TUI_TAB_ALBUMS,
    TUI_TAB_SONGS,
    TUI_TAB_ARTIST,
    TUI_TAB_ALBUM,
};

struct tui_tab_artists {
    int page, items_per_page, scroll;
    ARRAY(struct artist) artists;
};

struct tui_tab_albums {
    int page, items_per_page, scroll;
    ARRAY(struct album) albums;
};

struct tui_tab_songs {
    int page, items_per_page, scroll;
    ARRAY(struct song) songs;
};

struct tui_tab_artist {
    int scroll;
    struct artist *artist;
    ARRAY(struct album) albums;
    ARRAY(struct song) songs;
};

struct tui_tab_album {
    int scroll;
    struct artist *artist;
    ARRAY(struct album) albums;
    ARRAY(struct song) songs;
};

struct tui {
    struct pollen_callback *resize_callback;
    struct {
        struct signal_listener player_listener, network_listener;

        WINDOW *win;

        int64_t pos, volume;
        bool pause, mute;

        uint64_t net_conns;
        uint64_t net_speed[2];
    } statusbar;

    WINDOW *tabbar_win;

    enum tui_tab active_tab;
    struct tui_list list;
};

extern struct tui tui;

#endif /* #ifndef SRC_TUI_INTERNAL_H */

