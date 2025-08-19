#ifndef SRC_TUI_INTERNAL_H
#define SRC_TUI_INTERNAL_H

#include <ncurses.h>

#include "signals.h"

#define STATUSBAR_HEIGHT 4

typedef WINDOW PAD;

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

    enum tui_tab active_tab;
    WINDOW *tabbar_win;

    PAD *mainwin;
};

extern struct tui tui;

#endif /* #ifndef SRC_TUI_INTERNAL_H */

