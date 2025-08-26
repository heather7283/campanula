#ifndef SRC_TUI_INTERNAL_H
#define SRC_TUI_INTERNAL_H

#include <ncurses.h>

#include "tui/menu.h"
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
    VEC(struct artist) artists;
};

struct tui_tab_albums {
    int page, items_per_page, scroll;
    VEC(struct album) albums;
};

struct tui_tab_songs {
    int page, items_per_page, scroll;
    VEC(struct song) songs;
};

struct tui_tab_artist {
    int scroll;
    struct artist *artist;
    VEC(struct album) albums;
    VEC(struct song) songs;
};

struct tui_tab_album {
    int scroll;
    struct artist *artist;
    VEC(struct album) albums;
    VEC(struct song) songs;
};

struct tui {
    struct pollen_callback *resize_callback;
    struct {
        struct signal_listener player_listener, network_listener;

        WINDOW *win;

        int64_t pos, time_pos, duration, volume;
        bool pause, mute;

        uint64_t net_conns;
        uint64_t net_speed[2];
    } statusbar;

    enum tui_tab tab;
    WINDOW *tabbar_win;

    struct {
        struct tui_menu menu;
        union {
            struct {
                int64_t current;
            } playlist;
        };
    } mainwin;
};

extern struct tui tui;

void tui_switch_tab_playlist(void);
void tui_switch_tab_songs(void);

#endif /* #ifndef SRC_TUI_INTERNAL_H */

