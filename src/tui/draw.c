#include "tui/draw.h"
#include "tui/internal.h"
#include "player/playlist.h"

void draw_mainwin(void) {
    wchar_t line[COLS];
    const int xpos = COLS / 2 - 6;
    const int ypos = LINES / 2 - 2;

    wclear(tui.mainwin);

    swprintf(line, COLS, L"     〇      ");
    mvwaddwstr(tui.mainwin, ypos - 2, xpos, line);
    swprintf(line, COLS, L"   { 零 }    ");
    mvwaddwstr(tui.mainwin, ypos - 1, xpos, line);
    swprintf(line, COLS, L" THIS SPACE  ");
    mvwaddwstr(tui.mainwin, ypos, xpos, line);
    swprintf(line, COLS, L"INTENTIONALLY");
    mvwaddwstr(tui.mainwin, ypos + 1, xpos, line);
    swprintf(line, COLS, L" LEFT BLANK. ");
    mvwaddwstr(tui.mainwin, ypos + 2, xpos, line);

    wnoutrefresh(tui.mainwin);
}

void draw_status_bar(void) {
    const int cols = COLS - 2;
    wchar_t line[cols];
    int chars;

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

    const uint64_t bps = tui.statusbar.net_speed * 8;
    if (bps >= 1024 * 1024) {
        chars = swprintf(line, cols, L" NET %.2f Mibps (%lu)",
                         (double)bps / (1024 * 1024), tui.statusbar.net_conns);
    } else if (tui.statusbar.net_speed >= 1024) {
        chars = swprintf(line, cols, L" NET %.2f Kibps (%lu)",
                         (double)bps / 1024, tui.statusbar.net_conns);
    } else {
        chars = swprintf(line, cols, L" NET %lu bitps (%lu)",
                         tui.statusbar.net_speed, tui.statusbar.net_conns);
    }
    mvwaddnwstr(tui.statusbar.win, 1, COLS - chars - 1, line, cols);

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

