#include <stdlib.h>
#include <limits.h>

#include "tui/internal.h"
#include "tui/draw.h"
#include "player/playlist.h"
#include "db/query.h"

struct tui tui = {
    .tab = TUI_TAB_SONGS,
    .tabs = {
        [TUI_TAB_PLAYLIST] = {
            .playlist = {
                .current = -1,
            },
        },
    },
};

void tui_switch_tab_playlist(void) {
    tui_menu_hide(&tui.tabs[tui.tab].menu);

    tui.tab = TUI_TAB_PLAYLIST;
    tui_menu_show(&tui.tabs[tui.tab].menu);

    draw_tab_bar();
    doupdate();
}

void tui_switch_tab_songs(void) {
    tui_menu_hide(&tui.tabs[tui.tab].menu);

    tui.tab = TUI_TAB_SONGS;
    tui_menu_show(&tui.tabs[tui.tab].menu);

    if (!tui.tabs[tui.tab].songs.populated) {
        struct song *songs;
        const size_t nsongs = db_get_songs(&songs, 0, INT64_MAX);

        tui_menu_clear(&tui.tabs[tui.tab].menu);
        for (size_t i = 0; i < nsongs; i++) {
            struct song *s = &songs[i];

            tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
                .type = TUI_MENU_ITEM_TYPE_SONG,
                .as.song = {
                    .song = s,
                },
            });
            song_free_contents(s);
        }
        free(songs);

        tui.tabs[tui.tab].songs.populated = true;
    }

    draw_tab_bar();
    doupdate();
}

void tui_switch_tab_albums(void) {
    tui_menu_hide(&tui.tabs[tui.tab].menu);

    tui.tab = TUI_TAB_ALBUMS;
    tui_menu_show(&tui.tabs[tui.tab].menu);

    if (!tui.tabs[tui.tab].albums.populated) {
        struct album *albums;
        const size_t nalbums = db_get_albums(&albums, 0, INT64_MAX);

        tui_menu_clear(&tui.tabs[tui.tab].menu);
        for (size_t i = 0; i < nalbums; i++) {
            struct album *s = &albums[i];

            tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
                .type = TUI_MENU_ITEM_TYPE_ALBUM,
                .as.album = {
                    .album = s,
                },
            });
            album_free_contents(s);
        }
        free(albums);

        tui.tabs[tui.tab].albums.populated = true;
    }

    draw_tab_bar();
    doupdate();
}

