#include "tui/events.h"
#include "tui/internal.h"
#include "tui/draw.h"
#include "tui/list.h"
#include "tui/utils.h"
#include "player/control.h"
#include "player/events.h"
#include "player/playlist.h"
#include "network/events.h"
#include "log.h"

void tui_handle_resize(int width, int height) {
    DEBUG("resize: new term size %dx%d", width, height);

    resize_term(height, width);
    wclear(newscr); /* ncurses nonsense */

    tui_menu_position(&tui.list, 0, 1, COLS, LINES - STATUSBAR_HEIGHT - 1);
    tui_menu_draw(&tui.list);

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
    case 'k':
        if (tui_menu_select_prev(&tui.list)) {
            doupdate();
        }
        break;
    case 'j':
        if (tui_menu_select_next(&tui.list)) {
            doupdate();
        }
        break;
    case '\n':
        tui_menu_activate(&tui.list);
        break;
    case 'q':
        player_quit();
        break;
    default:
        TRACE("unhandled wchar %s (%d 0x%X)", key_name_from_key_code(key), key, key);
    }
}

void tui_handle_player_events(uint64_t event, const struct signal_data *data, void *userdata) {
    switch ((enum player_event)event) {
    case PLAYER_EVENT_VOLUME:
        tui.statusbar.volume = data->as.i64;
        draw_status_bar();
        break;
    case PLAYER_EVENT_MUTE:
        tui.statusbar.mute = data->as.boolean;
        draw_status_bar();
        break;
    case PLAYER_EVENT_PAUSE:
        tui.statusbar.pause = data->as.boolean;
        draw_status_bar();
        break;
    case PLAYER_EVENT_PERCENT_POSITION:
        tui.statusbar.pos = data->as.i64;
        draw_status_bar();
        break;
    case PLAYER_EVENT_PLAYLIST_POSITION:
        draw_status_bar();
        break;
    case PLAYER_EVENT_PLAYLIST:
        /* FIXME: this is very retarded, ugly and inefficient but I have no idea what else to do */
        const size_t prev_selected = tui.list.selected;
        const struct song *songs;
        const size_t nsongs = playlist_get_songs(&songs);
        const size_t current = playlist_get_current_song(NULL);
        tui_menu_clear(&tui.list);
        for (size_t i = 0; i < nsongs; i++) {
            tui_menu_add_playlist_item(&tui.list, i, i == current, &songs[i]);
        }
        tui.list.selected = MIN(prev_selected, VEC_SIZE(&tui.list.items) - 1);
        tui_menu_draw(&tui.list);
        draw_status_bar();
        break;
    }

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

