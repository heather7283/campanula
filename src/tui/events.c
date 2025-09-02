#include <assert.h>

#include "tui/events.h"
#include "tui/internal.h"
#include "tui/draw.h"
#include "tui/menu.h"
#include "tui/utils.h"
#include "player/control.h"
#include "player/events.h"
#include "player/playlist.h"
#include "network/events.h"
#include "db/populate.h"
#include "log.h"

void tui_handle_resize(int width, int height) {
    DEBUG("resize: new term size %dx%d", width, height);

    resize_term(height, width);
    wclear(newscr); /* ncurses nonsense */

    for (int i = 0; i < TUI_TAB_COUNT; i++) {
        tui_menu_position(&tui.tabs[i].menu, 0, 1, COLS, LINES - STATUSBAR_HEIGHT - 1);
    }
    tui_menu_draw(&tui.tabs[tui.tab].menu);

    if (tui.statusbar.win != NULL) {
        delwin(tui.statusbar.win);
    }
    tui.statusbar.win = newwin(STATUSBAR_HEIGHT, COLS, LINES - STATUSBAR_HEIGHT, 0);
    nodelay(tui.statusbar.win, true); /* makes getch() return ERR instead of blocking */
    keypad(tui.statusbar.win, true); /* enable recognition of escape sequences */
    leaveok(tui.statusbar.win, true);
    draw_status_bar();

    if (tui.tabbar_win != NULL) {
        delwin(tui.tabbar_win);
    }
    tui.tabbar_win = newwin(1, COLS, 0, 0);
    leaveok(tui.tabbar_win, true);
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
    case 'R':
        db_populate();
        break;
    case 'r':
        switch (tui.tab) {
        case TUI_TAB_ARTISTS:
            tui_tab_artists_populate();
            break;
        case TUI_TAB_ALBUMS:
            tui_tab_albums_populate();
            break;
        case TUI_TAB_SONGS:
            tui_tab_songs_populate();
            break;
        case TUI_TAB_ARTIST:
            if (tui.tabs[TUI_TAB_ARTIST].artist.artist != NULL) {
                tui_tab_artist_populate(tui.tabs[TUI_TAB_ARTIST].artist.artist);
            }
            break;
        case TUI_TAB_ALBUM:
            if (tui.tabs[TUI_TAB_ALBUM].album.album != NULL) {
                tui_tab_album_populate(tui.tabs[TUI_TAB_ALBUM].album.album);
            }
            break;
        default:
        }
        doupdate();
        break;
    case 'k':
        if (tui_menu_select_prev(&tui.tabs[tui.tab].menu)) {
            doupdate();
        }
        break;
    case 'j':
        if (tui_menu_select_next(&tui.tabs[tui.tab].menu)) {
            doupdate();
        }
        break;
    case 'a':
        tui_menu_action_append(&tui.tabs[tui.tab].menu);
        break;
    case '\n':
        tui_menu_action_activate(&tui.tabs[tui.tab].menu);
        break;
    case 'p':
        tui_tab_playlist_activate();
        doupdate();
        break;
    case '1':
        tui_tab_artists_activate();
        doupdate();
        break;
    case '2':
        tui_tab_albums_activate();
        doupdate();
        break;
    case '3':
        tui_tab_songs_activate();
        doupdate();
        break;
    case '4':
        tui_tab_artist_activate(NULL);
        doupdate();
        break;
    case '5':
        tui_tab_album_activate(NULL);
        doupdate();
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
    case PLAYER_EVENT_DURATION:
        tui.statusbar.duration = data->as.u64;
        draw_status_bar();
        break;
    case PLAYER_EVENT_TIME_POSITION:
        tui.statusbar.time_pos = data->as.u64;
        draw_status_bar();
        break;
    case PLAYER_EVENT_PLAYLIST_POSITION: {
        const int new_index = (int)data->as.i64;
        const int old_index = tui.tabs[TUI_TAB_PLAYLIST].playlist.current;
        struct tui_tab *const tab = &tui.tabs[TUI_TAB_PLAYLIST];

        const struct song *songs;
        playlist_get_songs(&songs);

        /* mark old one as not current */
        if (old_index >= 0) {
            struct tui_menu_item *old = tui_menu_get_item(&tab->menu, old_index);
            assert(old->type == TUI_MENU_ITEM_TYPE_PLAYLIST_ITEM);
            old->as.playlist_item.current = false;
            tui_menu_draw_item(&tab->menu, old_index);
        }

        /* mark new one as current */
        if (new_index >= 0) {
            struct tui_menu_item *new = tui_menu_get_item(&tab->menu, new_index);
            assert(new->type == TUI_MENU_ITEM_TYPE_PLAYLIST_ITEM);
            new->as.playlist_item.current = true;
            tui_menu_draw_item(&tab->menu, new_index);
        }

        tab->playlist.current = new_index;

        draw_status_bar();
        break;
    }
    case PLAYER_EVENT_PLAYLIST_SONG_ADDED: {
        const int index = (int)data->as.u64;

        const struct song *songs;
        playlist_get_songs(&songs);

        const struct tui_menu_item item = {
            .type = TUI_MENU_ITEM_TYPE_PLAYLIST_ITEM,
            .as.playlist_item = {
                .index = index,
                .current = (index == playlist_get_current_song(NULL)),
                .song = (struct song *)&songs[index],
            },
        };
        tui_menu_insert_or_replace_item(&tui.tabs[TUI_TAB_PLAYLIST].menu, index, &item);

        break;
    }
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

