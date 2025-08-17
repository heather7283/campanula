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

    PAD *mainwin;
};

extern struct tui tui;

#endif /* #ifndef SRC_TUI_INTERNAL_H */

