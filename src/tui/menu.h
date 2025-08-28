#ifndef SRC_TUI_MENU_H
#define SRC_TUI_MENU_H

#include <curses.h>

#include "collections/vec.h"
#include "types/song.h"

enum tui_menu_item_type {
    TUI_MENU_ITEM_TYPE_EMPTY,
    TUI_MENU_ITEM_TYPE_LABEL,
    TUI_MENU_ITEM_TYPE_PLAYLIST_ITEM,
    TUI_MENU_ITEM_TYPE_SONG,
    TUI_MENU_ITEM_TYPE_ALBUM,
    TUI_MENU_ITEM_TYPE_ARTIST,

    TUI_MENU_ITEM_TYPE_COUNT,
};

struct tui_menu_item_empty {
};

struct tui_menu_item_label {
    char *str;
};

struct tui_menu_item_playlist_item {
    int index;
    bool current;
    struct song *song;
};

struct tui_menu_item_song {
    struct song *song;
};

struct tui_menu_item_album {
    struct album *album;
};

struct tui_menu_item_artist {
    struct artist *artist;
};

struct tui_menu_item {
    enum tui_menu_item_type type;
    union {
        struct tui_menu_item_empty empty;
        struct tui_menu_item_label label;
        struct tui_menu_item_playlist_item playlist_item;
        struct tui_menu_item_song song;
        struct tui_menu_item_album album;
        struct tui_menu_item_artist artist;
    } as;
};

struct tui_menu {
    WINDOW *win;
    bool hidden;
    int screen_x, screen_y;
    int width, height;

    WINDOW *scrollbar_win;

    size_t scroll;
    size_t selected;
    VEC(struct tui_menu_item) items;
};

void tui_menu_position(struct tui_menu *menu, int screen_x, int screen_y, int width, int height);
void tui_menu_hide(struct tui_menu *menu);
void tui_menu_show(struct tui_menu *menu);

void tui_menu_draw(struct tui_menu *menu);
bool tui_menu_draw_item(struct tui_menu *menu, size_t index);
void tui_menu_draw_scrollbar(struct tui_menu *menu);

struct tui_menu_item *tui_menu_get_item(struct tui_menu *menu, size_t index);

bool tui_menu_remove_item(struct tui_menu *menu, size_t index);
void tui_menu_clear(struct tui_menu *menu);

bool tui_menu_select_nth(struct tui_menu *menu, size_t index);
bool tui_menu_select_next(struct tui_menu *menu);
bool tui_menu_select_prev(struct tui_menu *menu);

bool tui_menu_append_item(struct tui_menu *menu, const struct tui_menu_item *item);
bool tui_menu_insert_or_replace_item(struct tui_menu *menu, size_t index,
                                     const struct tui_menu_item *item);

void tui_menu_action_activate(struct tui_menu *menu);
void tui_menu_action_append(struct tui_menu *menu);

#endif /* #ifndef SRC_TUI_MENU_H */

