#include "tui/draw.h"
#include "tui/internal.h"
#include "player/playlist.h"

void draw_status_bar(void) {
    const int cols = COLS - 2;
    wchar_t line[cols];

    const struct song *s = NULL;
    const size_t index = playlist_get_current_song(&s);
    const size_t total = playlist_get_songs(NULL);
    if (s != NULL) {
        swprintf(line, cols, L"(%zu/%zu) %s - %s", index + 1, total, s->artist, s->title);
        mvwaddnwstr(tui.statusbar.win, 1, 1, line, cols);
    } else {
        swprintf(line, cols, L"(0/0)");
        mvwaddnwstr(tui.statusbar.win, 1, 1, line, cols);
    }
    wclrtoeol(tui.statusbar.win);

    swprintf(line, cols, L"%s %3li%%", tui.statusbar.pause ? "||" : "|>", tui.statusbar.pos);
    mvwaddnwstr(tui.statusbar.win, 2, 1, line, cols);

    if (tui.statusbar.mute) {
        swprintf(line, cols, L"VOL MUTE");
    } else {
        swprintf(line, cols, L"VOL %3li%%", tui.statusbar.volume);
    }
    mvwaddnwstr(tui.statusbar.win, 2, COLS - 9, line, cols);

    mvwaddch(tui.statusbar.win, 2, 9, '[');
    mvwaddch(tui.statusbar.win, 2, cols - 9, ']');

    const int range = cols - 9 - 10;
    const int thresh = tui.statusbar.pos * range / 100;
    for (int i = 0; i < cols - 9 - 10; i++) {
        mvwaddch(tui.statusbar.win, 2, i + 10, (i <= thresh) ? '#' : '-');
    }

    wborder(tui.statusbar.win, 0, 0, 0, 0, 0, 0, 0, 0);

    wnoutrefresh(tui.statusbar.win);
}

