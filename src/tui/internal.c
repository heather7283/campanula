#include <stdlib.h>
#include <limits.h>

#include "tui/internal.h"
#include "tui/draw.h"
#include "cleanup.h"
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

void tui_tab_playlist_activate(void) {
    tui_menu_hide(&tui.tabs[tui.tab].menu);

    tui.tab = TUI_TAB_PLAYLIST;
    tui_menu_show(&tui.tabs[tui.tab].menu);

    draw_tab_bar();
    doupdate();
}

void tui_tab_songs_populate(void) {
    [[gnu::cleanup(cleanup_free)]] struct song *songs = NULL;
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
}

void tui_tab_songs_activate(void) {
    tui_menu_hide(&tui.tabs[tui.tab].menu);
    tui.tab = TUI_TAB_SONGS;
    tui_menu_show(&tui.tabs[tui.tab].menu);

    if (!tui.tabs[tui.tab].songs.populated) {
        tui_tab_songs_populate();
        tui.tabs[tui.tab].songs.populated = true;
    }

    draw_tab_bar();
    doupdate();
}

void tui_tab_albums_populate(void) {
    [[gnu::cleanup(cleanup_free)]] struct album *albums = NULL;
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
}

void tui_tab_albums_activate(void) {
    tui_menu_hide(&tui.tabs[tui.tab].menu);

    tui.tab = TUI_TAB_ALBUMS;
    tui_menu_show(&tui.tabs[tui.tab].menu);

    if (!tui.tabs[tui.tab].albums.populated) {
        tui_tab_albums_populate();
        tui.tabs[tui.tab].albums.populated = true;
    }

    draw_tab_bar();
    doupdate();
}

void tui_tab_artists_populate(void) {
    [[gnu::cleanup(cleanup_free)]] struct artist *artists = NULL;
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
}

void tui_tab_artists_activate(void) {
    tui_menu_hide(&tui.tabs[tui.tab].menu);

    tui.tab = TUI_TAB_ARTISTS;
    tui_menu_show(&tui.tabs[tui.tab].menu);

    if (!tui.tabs[tui.tab].artists.populated) {
        tui_tab_artists_populate();
        tui.tabs[tui.tab].artists.populated = true;
    }

    draw_tab_bar();
    doupdate();
}

void tui_tab_album_populate(const struct album *album) {
    tui_menu_clear(&tui.tabs[tui.tab].menu);

    tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
        .type = TUI_MENU_ITEM_TYPE_ALBUM,
        .as.album.album = (struct album *)album,
    });

    tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
        .type = TUI_MENU_ITEM_TYPE_EMPTY,
    });

    [[gnu::cleanup(cleanup_free)]] struct song *songs = NULL;
    size_t nsongs = db_get_songs_in_album(&songs, album);
    for (size_t i = 0; i < nsongs; i++) {
        tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
            .type = TUI_MENU_ITEM_TYPE_SONG,
            .as.song.song = &songs[i],
        });

        song_free_contents(&songs[i]);
    }
}

void tui_tab_album_activate(const struct album *album) {
    tui_menu_hide(&tui.tabs[tui.tab].menu);

    tui.tab = TUI_TAB_ALBUM;
    tui_menu_show(&tui.tabs[tui.tab].menu);

    if (album != NULL) {
        tui_tab_album_populate(album);
    }

    draw_tab_bar();
    doupdate();
}

void tui_tab_artist_populate(const struct artist *artist) {
    tui_menu_clear(&tui.tabs[tui.tab].menu);

    tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
        .type = TUI_MENU_ITEM_TYPE_ARTIST,
        .as.artist.artist = (struct artist *)artist,
    });

    tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
        .type = TUI_MENU_ITEM_TYPE_EMPTY,
    });

    tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
        .type = TUI_MENU_ITEM_TYPE_LABEL,
        .as.label.str = "Albums:",
    });

    [[gnu::cleanup(cleanup_free)]] struct album *albums = NULL;
    size_t nalbums = db_get_albums_for_artist(&albums, artist);
    for (size_t i = 0; i < nalbums; i++) {
        tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
            .type = TUI_MENU_ITEM_TYPE_ALBUM,
            .as.album.album = &albums[i],
        });

        album_free_contents(&albums[i]);
    }

    tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
        .type = TUI_MENU_ITEM_TYPE_EMPTY,
    });

    tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
        .type = TUI_MENU_ITEM_TYPE_LABEL,
        .as.label.str = "Songs:",
    });

    [[gnu::cleanup(cleanup_free)]] struct song *songs = NULL;
    size_t nsongs = db_get_songs_for_artist(&songs, artist);
    for (size_t i = 0; i < nsongs; i++) {
        tui_menu_append_item(&tui.tabs[tui.tab].menu, &(struct tui_menu_item){
            .type = TUI_MENU_ITEM_TYPE_SONG,
            .as.song.song = &songs[i],
        });

        song_free_contents(&songs[i]);
    }
}

void tui_tab_artist_activate(const struct artist *artist) {
    tui_menu_hide(&tui.tabs[tui.tab].menu);

    tui.tab = TUI_TAB_ARTIST;
    tui_menu_show(&tui.tabs[tui.tab].menu);

    if (artist != NULL) {
        tui_tab_artist_populate(artist);
    }

    draw_tab_bar();
    doupdate();
}

