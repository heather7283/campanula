#include "tui/draw.h"
#include "tui/internal.h"
#include "player/playlist.h"
#include "log.h"

static const char *tab_names[] = {
    [TUI_TAB_PLAYLIST] = "Playlist",
    [TUI_TAB_ARTISTS] = "Artists",
    [TUI_TAB_ALBUMS] = "Albums",
    [TUI_TAB_SONGS] = "Songs",
    [TUI_TAB_ARTIST] = "Artist",
    [TUI_TAB_ALBUM] = "Album",
};

void draw_tab_bar(void) {
    wmove(tui.tabbar_win, 0, 0);

    for (size_t i = 0; i < SIZEOF_VEC(tab_names); i++) {
        if (i == tui.active_tab) {
            wattron(tui.tabbar_win, A_BOLD);
        }

        waddstr(tui.tabbar_win, tab_names[i]);
        waddch(tui.tabbar_win, ' ');

        if (i == tui.active_tab) {
            wattroff(tui.tabbar_win, A_BOLD);
        }
    }

    wnoutrefresh(tui.tabbar_win);
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

    static const char units[][6] = { "bitps", "Kibps", "Mibps", "Gibps", "Tibps", "Pibps" };
    static const wchar_t labels[] = { [NET_SPEED_DL] = L'V', [NET_SPEED_UL] = L'Λ' };
    chars = swprintf(line, cols, L" NET %lu", tui.statusbar.net_conns);
    for (size_t i = 0; i < SIZEOF_VEC(tui.statusbar.net_speed); i++) {
        if (tui.statusbar.net_speed[i] == 0) {
            continue;
        }

        double n = tui.statusbar.net_speed[i] * 8; /* convert to bits per second */
        int j = 0;
        while (n >= 1024) {
            n /= 1024;
            j += 1;
        }

        chars += swprintf(line + chars, cols - chars,
                          L" %lc %.*f %s", labels[i], j == 0 ? 0 : 2, n, units[j]);
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

