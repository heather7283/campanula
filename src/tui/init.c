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
    pollen_efd_trigger(tui.resize_callback, 1);
}

static int stdin_handler(struct pollen_callback *, int, uint32_t, void *) {
    wint_t ch;
    int ret;
    while (errno = 0, (ret = wget_wch(tui.statusbar.win, &ch)) != ERR || errno == EINTR) {
        switch (ret) {
        case OK: /* wide character */
            DEBUG("got wchar %s (%d)", key_name_from_key_code(ch), ch);
            tui_handle_key(ch);
            break;
        case KEY_CODE_YES: /* special keycode */
            DEBUG("got function key code %d (%X)", ch, ch);
            break;
        }
    }

    return 0;
}

bool tui_init(void) {
    setlocale(LC_ALL, ""); /* needed for wide ncurses api to work */

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

    tui_list_add_label(&tui.list, "aboba");
    tui_list_add_label(&tui.list, "amogus");
    tui_list_add_label(&tui.list, "skibidi");

    player_event_subscribe(&tui.statusbar.player_listener, (uint64_t)-1 /* all */,
                           tui_handle_player_events, NULL);
    network_event_subscribe(&tui.statusbar.network_listener, (uint64_t)-1 /* all */,
                            tui_handle_network_events, NULL);

    /* trigger it manually to pick up initial size and draw everything */
    sigwinch_handler_deferred(NULL, 0, NULL);

    return true;
}

void tui_cleanup(void) {
    if (tui.statusbar.win != NULL) {
        delwin(tui.statusbar.win);
    }

    endwin();
}

