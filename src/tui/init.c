#include <fcntl.h>
#include <errno.h>
#include <locale.h>

#include "tui/init.h"
#include "tui/utils.h"
#include "tui/internal.h"
#include "tui/events.h"
#include "player/events.h"
#include "network/events.h"
#include "log.h"
#include "eventloop.h"

static int sigwinch_handler_deferred(struct pollen_callback *, uint64_t, void *) {
    struct winsize winsize;
    if (ioctl(0 /* stdin */, TIOCGWINSZ, &winsize) < 0) {
        ERROR("failed to get new window size: %m");
        return -1;
    }

    tui_handle_resize(winsize.ws_col, winsize.ws_row);

    return 0;
}

static void sigwinch_handler(int) {
    pollen_efd_trigger(tui.resize_callback);
}

static int stdin_handler(struct pollen_callback *, int, uint32_t, void *) {
    /* can't use getch() and other functions provided by ncurses here
     * because they for some fucking reason call refresh() internally
     * which causes immense amounts of brain damage and ass pain.
     * I wish all ncurses developers a very slow and painful death. */
    static char buf[64];
    while (true) {
        ssize_t ret = read(0 /* stdin */, buf, sizeof(buf));
        if (ret < 0) {
            if (errno != EAGAIN) {
                ERROR("failed to read from stdin: %m");
                return ret;
            } else {
                return 0;
            }
        }

        for (size_t i = 0; i < (size_t)ret; i++) {
            const uint32_t wchar = buf[i];
            TRACE("stdin_handler: got char %c (dec %d hex %x)", buf[i], wchar, wchar);
            tui_handle_key(wchar);
        }
    }

    return 0;
}

bool tui_init(void) {
    setlocale(LC_ALL, ""); /* needed for wide ncurses api to work */

    tui.saved_fdflags = fcntl(0 /* stdin */, F_GETFL);
    if (tui.saved_fdflags < 0) {
        ERROR("fcntl(0, F_GETFL): %m");
        return 1;
    }

    if (fcntl(0 /* stdin */, F_SETFL, tui.saved_fdflags | O_NONBLOCK) < 0) {
        ERROR("fcntl(0, F_SETFL, ... | O_NONBLOCK): %m");
        return 1;
    }

    initscr();
    refresh(); /* https://stackoverflow.com/a/22121866 */
    cbreak();
    noecho();
    curs_set(0);
    ESCDELAY = 50 /* ms */;

    pollen_loop_add_fd(event_loop, 0 /* stdin */, EPOLLIN, false, stdin_handler, NULL);

    tui.resize_callback = pollen_loop_add_efd(event_loop, sigwinch_handler_deferred, NULL);
    sigaction(SIGWINCH, &(struct sigaction){
        /* From mpv/client.h:
         * If anything in the process registers signal handlers, they must set the
         * SA_RESTART flag. Otherwise you WILL get random failures on signals. */
        .sa_flags = SA_RESTART,
        .sa_handler = sigwinch_handler,
    }, NULL);

    /* TODO: figure out why this doesn't work (sometimes sigwinch is not received) */
    //pollen_loop_add_signal(event_loop, SIGWINCH, sigwinch_handler, NULL);

    player_event_subscribe(&tui.statusbar.player_listener, (uint64_t)-1 /* all */,
                           tui_handle_player_events, NULL);
    network_event_subscribe(&tui.statusbar.network_listener, (uint64_t)-1 /* all */,
                            tui_handle_network_events, NULL);

    /* trigger it manually to pick up initial size and draw everything */
    sigwinch_handler_deferred(NULL, 0, NULL);

    for (int i = 0; i < TUI_TAB_COUNT; i++) {
        tui.tabs[i].menu.hidden = true;
    }
    tui_tab_albums_activate();

    return true;
}

void tui_cleanup(void) {
    if (tui.statusbar.win != NULL) {
        delwin(tui.statusbar.win);
    }

    endwin();

    if (fcntl(0 /* stdin */, F_SETFL, tui.saved_fdflags) < 0) {
        ERROR("fcntl(0, F_SETFL, O_NONBLOCK): %m");
    }
}

