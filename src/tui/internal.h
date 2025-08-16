#ifndef SRC_TUI_INTERNAL_H
#define SRC_TUI_INTERNAL_H

#include <ncurses.h>

#include "signals.h"

#define STATUSBAR_HEIGHT 4

struct tui {
    struct pollen_callback *resize_callback;
    struct {
        struct signal_listener player_listener, network_listener;

        WINDOW *win;

        int64_t pos, volume;
        bool pause, mute;

        uint64_t net_speed, net_conns;
    } statusbar;

    WINDOW *mainwin;
};

extern struct tui tui;

#endif /* #ifndef SRC_TUI_INTERNAL_H */

