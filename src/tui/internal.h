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

enum tui_tab_type {
    TUI_TAB_PLAYLIST = 0,
    TUI_TAB_ARTISTS = 1,
    TUI_TAB_ALBUMS = 2,
    TUI_TAB_SONGS = 3,
    TUI_TAB_ARTIST = 4,
    TUI_TAB_ALBUM = 5,

    TUI_TAB_COUNT,
};

struct tui_tab {
    struct tui_menu menu;
    union {
        struct {
            int current;
        } playlist;
        struct {
            bool populated;
        } songs;
        struct {
            struct artist *artist;
        } artist;
        struct {
            struct album *album;
        } album;
    };
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

    WINDOW *tabbar_win;

    enum tui_tab_type tab;
    struct tui_tab tabs[TUI_TAB_COUNT];
};

extern struct tui tui;

void tui_switch_tab_playlist(void);
void tui_switch_tab_songs(void);

#endif /* #ifndef SRC_TUI_INTERNAL_H */

