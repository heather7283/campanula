#ifndef SRC_TUI_LIST_H
#define SRC_TUI_LIST_H

#include "tui/pad.h"
#include "collections/array.h"
#include "types/song.h"

enum tui_list_item_type {
    TUI_LIST_ITEM_TYPE_LABEL,
    TUI_LIST_ITEM_TYPE_PLAYLIST_ITEM,

    TUI_LIST_ITEM_TYPE_COUNT,
};

struct tui_list_item_label {
    char *str;
};

struct tui_list_item_playlist_item {
    int index;
    bool current;
    struct song *song;
};

struct tui_list_item {
    enum tui_list_item_type type;
    union {
        struct tui_list_item_label label;
        struct tui_list_item_playlist_item playlist_item;
    } as;
};

struct tui_list {
    PAD *pad;
    int screen_x, screen_y;
    int width, height;

    size_t scroll;
    size_t selected;
    ARRAY(struct tui_list_item) items;
};

void tui_list_position(struct tui_list *list, int screen_x, int screen_y, int width, int height);

void tui_list_draw(struct tui_list *list);
void tui_list_draw_nth(struct tui_list *list, size_t index);

void tui_list_clear(struct tui_list *list);

bool tui_list_select_nth(struct tui_list *list, size_t index);
bool tui_list_select_next(struct tui_list *list);
bool tui_list_select_prev(struct tui_list *list);

void tui_list_activate(struct tui_list *list);

void tui_list_add_label(struct tui_list *list,
                        const char *label);
void tui_list_add_playlist_item(struct tui_list *list,
                                int index, bool current, const struct song *song);

#endif /* #ifndef SRC_TUI_LIST_H */

