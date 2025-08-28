#include <stdlib.h>
#include <limits.h>

#include "tui/internal.h"
#include "tui/draw.h"
#include "collections/string.h"
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

void tui_switch_tab_artists(void) {
    tui_menu_hide(&tui.tabs[tui.tab].menu);

    tui.tab = TUI_TAB_ARTISTS;
    tui_menu_show(&tui.tabs[tui.tab].menu);

    if (!tui.tabs[tui.tab].artists.populated) {
        struct artist *artists;
        const size_t nartists = db_get_artists(&artists, 0, INT64_MAX);

        tui_menu_clear(&tui.tabs[tui.tab].menu);
        for (size_t i = 0; i < nartists; i++) {
            struct artist *s = &artists[i];

            tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
                .type = TUI_MENU_ITEM_TYPE_ARTIST,
                .as.artist = {
                    .artist = s,
                },
            });
            artist_free_contents(s);
        }
        free(artists);

        tui.tabs[tui.tab].artists.populated = true;
    }

    draw_tab_bar();
    doupdate();
}

void tui_switch_tab_album(const struct album *album) {
    tui_menu_hide(&tui.tabs[tui.tab].menu);

    tui.tab = TUI_TAB_ALBUM;
    tui_menu_show(&tui.tabs[tui.tab].menu);

    if (album != NULL) {
        tui_menu_clear(&tui.tabs[tui.tab].menu);

        tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
            .type = TUI_MENU_ITEM_TYPE_ALBUM,
            .as.album.album = (struct album *)album,
        });

        tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
            .type = TUI_MENU_ITEM_TYPE_EMPTY,
        });

        struct song *songs;
        size_t nsongs = db_get_songs_in_album(&songs, album);
        for (size_t i = 0; i < nsongs; i++) {
            tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
                .type = TUI_MENU_ITEM_TYPE_SONG,
                .as.song.song = &songs[i],
            });

            song_free_contents(&songs[i]);
        }
        free(songs);
    }

    draw_tab_bar();
    doupdate();
}

void tui_switch_tab_artist(const struct artist *artist);

