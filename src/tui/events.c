#include "tui/events.h"
#include "tui/internal.h"
#include "tui/draw.h"
#include "tui/pad.h"
#include "player/control.h"
#include "player/events.h"
#include "network/events.h"
#include "log.h"

void tui_handle_resize(int width, int height) {
    DEBUG("resize: new term size %dx%d", width, height);

    resize_term(height, width);
    wclear(newscr); /* ncurses nonsense */

    tui.mainwin = tui_pad_ensure_size(tui.mainwin, AT_LEAST, height, AT_LEAST, width, false);
    draw_mainwin();

    if (tui.statusbar.win != NULL) {
        delwin(tui.statusbar.win);
    }
    tui.statusbar.win = newwin(STATUSBAR_HEIGHT, COLS, LINES - STATUSBAR_HEIGHT, 0);
    nodelay(tui.statusbar.win, TRUE); /* makes getch() return ERR instead of blocking */
    keypad(tui.statusbar.win, TRUE); /* enable recognition of escape sequences */
    draw_status_bar();

    if (tui.tabbar_win != NULL) {
        delwin(tui.tabbar_win);
    }
    tui.tabbar_win = newwin(1, COLS, 0, 0);
    draw_tab_bar();

    doupdate();
}

void tui_handle_key(uint32_t key) {
    switch (key) {
    case 'l':
        player_seek(5, true);
        break;
    case 'L':
        player_seek(60, true);
        break;
    case 'h':
        player_seek(-5, true);
        break;
    case 'H':
        player_seek(-60, true);
        break;
    case '>':
        player_next();
        break;
    case '<':
        player_prev();
        break;
    case '0':
        player_set_volume(2, true);
        break;
    case '9':
        player_set_volume(-2, true);
        break;
    case 'm':
        player_toggle_mute();
        break;
    case ' ':
        player_toggle_pause();
        break;
    case 'q':
        player_quit();
        break;
    }
}

void tui_handle_player_events(uint64_t event, const struct signal_data *data, void *userdata) {
    switch ((enum player_event)event) {
    case PLAYER_EVENT_VOLUME:
        tui.statusbar.volume = data->as.i64;
        break;
    case PLAYER_EVENT_MUTE:
        tui.statusbar.mute = data->as.boolean;
        break;
    case PLAYER_EVENT_PAUSE:
        tui.statusbar.pause = data->as.boolean;
        break;
    case PLAYER_EVENT_PERCENT_POSITION:
        tui.statusbar.pos = data->as.i64;
        break;
    case PLAYER_EVENT_PLAYLIST_POSITION:
        break;
    }

    draw_status_bar();
    doupdate();
}

void tui_handle_network_events(uint64_t event, const struct signal_data *data, void *userdata) {
    switch ((enum network_event)event) {
    case NETWORK_EVENT_SPEED_DL:
        tui.statusbar.net_speed[NET_SPEED_DL] = data->as.u64;
        break;
    case NETWORK_EVENT_SPEED_UL:
        tui.statusbar.net_speed[NET_SPEED_UL] = data->as.u64;
        break;
    case NETWORK_EVENT_CONNECTIONS:
        tui.statusbar.net_conns = data->as.u64;
        if (tui.statusbar.net_conns == 0) {
            tui.statusbar.net_speed[0] = tui.statusbar.net_speed[1] = 0;
        }
        break;
    }

    draw_status_bar();
    doupdate();
}

