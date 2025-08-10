#include "tui/events.h"
#include "tui/internal.h"
#include "tui/draw.h"
#include "player/control.h"
#include "player/events.h"
#include "log.h"

void tui_handle_resize(int width, int height) {
    DEBUG("resize: new term size %dx%d", width, height);

    resize_term(height, width);

    if (tui.statusbar.win != NULL) {
        delwin(tui.statusbar.win);
    }
    tui.statusbar.win = newwin(0, 0, LINES - STATUSBAR_HEIGHT, 0);

    draw_status_bar();
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
    case ' ':
        player_toggle_pause();
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

