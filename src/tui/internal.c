#include <limits.h>

#include "tui/internal.h"
#include "tui/draw.h"
#include "player/playlist.h"
#include "db/query.h"

struct tui tui = {
    .mainwin = {
        .playlist = {
            .current = -1,
        },
    },
};

void tui_switch_tab_playlist(void) {
    tui.tab = TUI_TAB_PLAYLIST;

    const struct song *songs;
    const size_t nsongs = playlist_get_songs(&songs);
    const size_t current = playlist_get_current_song(NULL);

    tui_menu_clear(&tui.mainwin.menu);
    for (size_t i = 0; i < nsongs; i++) {
        const struct song *s = &songs[i];

        tui_menu_append_item(&tui.mainwin.menu, &(struct tui_menu_item){
            .type = TUI_MENU_ITEM_TYPE_PLAYLIST_ITEM,
            .as.playlist_item = {
                .song = (struct song *)s,
                .current = (i == current),
                .index = i,
            },
        });
    }

    draw_status_bar();

    doupdate();
}

void tui_switch_tab_songs(void) {
    tui.tab = TUI_TAB_SONGS;

    struct song *songs;
    const size_t nsongs = db_get_songs(&songs, 0, 100);

    tui_menu_clear(&tui.mainwin.menu);
    for (size_t i = 0; i < nsongs; i++) {
        const struct song *s = &songs[i];

        tui_menu_append_item(&tui.mainwin.menu, &(struct tui_menu_item){
            .type = TUI_MENU_ITEM_TYPE_SONG,
            .as.song = {
                .song = (struct song *)s,
            },
        });
    }

    draw_status_bar();

    doupdate();
}

