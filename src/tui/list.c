#include "tui/list.h"
#include "tui/pad.h"
#include "player/control.h"
#include "xmalloc.h"
#include "macros.h"
#include "log.h"

static void tui_menu_item_playlist_item_draw(const struct tui_menu_item *self,
                                             PAD *pad, int ypos, int width) {
    const struct tui_menu_item_playlist_item *i = &self->as.playlist_item;
    const struct song *s = i->song;

    if (i->current) {
        wattron(pad, A_BOLD);
    }

    mvwprintw(pad, ypos, 0, "%s%d. %s - %s",
              i->current ? "> " : "", i->index, s->artist, s->title);

    if (i->current) {
        wattroff(pad, A_BOLD);
    }
}

static void tui_menu_item_playlist_item_free_contents(struct tui_menu_item *self) {
    struct tui_menu_item_playlist_item *i = &self->as.playlist_item;

    song_free_contents(i->song);
    free(i->song);
}

static bool tui_menu_item_playlist_item_is_selectable(const struct tui_menu_item *self) {
    return true;
}

static void tui_menu_item_playlist_item_activate(const struct tui_menu_item *self) {
    player_play_nth(self->as.playlist_item.index);
}

static void tui_menu_item_label_draw(const struct tui_menu_item *self,
                                     PAD *pad, int ypos, int width) {
    const struct tui_menu_item_label *l = &self->as.label;

    mvwaddnstr(pad, ypos, 0, l->str, width);
}

static void tui_menu_item_label_free_contents(struct tui_menu_item *self) {
    struct tui_menu_item_label *l = &self->as.label;
    free(l->str);
}

static bool tui_menu_item_label_is_selectable(const struct tui_menu_item *self) {
    return false;
}

static void tui_menu_item_label_activate(const struct tui_menu_item *self) {
    /* no-op */
}

typedef void (*tui_menu_item_method_draw)(const struct tui_menu_item *self,
                                          PAD *pad, int ypos, int width);

typedef void (*tui_menu_item_method_free_contents)(struct tui_menu_item *self);

typedef bool (*tui_menu_item_method_is_selectable)(const struct tui_menu_item *self);

typedef void (*tui_menu_item_method_activate)(const struct tui_menu_item *self);

struct tui_menu_item_methods {
    const tui_menu_item_method_draw draw;
    const tui_menu_item_method_free_contents free_contents;
    const tui_menu_item_method_is_selectable is_selectable;
    const tui_menu_item_method_activate activate;
};

static const struct tui_menu_item_methods tui_menu_item_methods[] = {
    [TUI_LIST_ITEM_TYPE_LABEL] = {
        .draw = tui_menu_item_label_draw,
        .free_contents = tui_menu_item_label_free_contents,
        .is_selectable = tui_menu_item_label_is_selectable,
        .activate = tui_menu_item_label_activate,
    },
    [TUI_LIST_ITEM_TYPE_PLAYLIST_ITEM] = {
        .draw = tui_menu_item_playlist_item_draw,
        .free_contents = tui_menu_item_playlist_item_free_contents,
        .is_selectable = tui_menu_item_playlist_item_is_selectable,
        .activate = tui_menu_item_playlist_item_activate,
    }
};
static_assert(SIZEOF_ARRAY(tui_menu_item_methods) == TUI_LIST_ITEM_TYPE_COUNT);

#define METHOD_CALL(pobj, method, ...) \
    tui_menu_item_methods[(pobj)->type].method((pobj) __VA_OPT__(,) __VA_ARGS__)

void tui_menu_position(struct tui_menu *list, int screen_x, int screen_y, int width, int height) {
    list->screen_x = screen_x;
    list->screen_y = screen_y;
    list->width = width;
    list->height = height;
    list->pad = tui_pad_ensure_size(list->pad,
                                    AT_LEAST, MAX((size_t)height, ARRAY_SIZE(&list->items)),
                                    AT_LEAST, width,
                                    false);
}

void tui_menu_draw_nth(struct tui_menu *list, size_t index) {
    if (index == list->selected) {
        wattron(list->pad, A_REVERSE);
    }

    const struct tui_menu_item *item = ARRAY_AT(&list->items, index);
    METHOD_CALL(item, draw, list->pad, index, list->width);

    if (index == list->selected) {
        wattroff(list->pad, A_REVERSE);
    }
}

void tui_menu_draw(struct tui_menu *list) {
    ARRAY_FOREACH(&list->items, i) {
        tui_menu_draw_nth(list, i);
    }
    wclrtobot(list->pad);

    if (pnoutrefresh(list->pad, 0, 0,
                     list->screen_y, list->screen_x,
                     list->height - 1, list->width - 1) != OK) {
        ERROR("pnoutrefresh");
    }
}

void tui_menu_clear(struct tui_menu *list) {
    ARRAY_FOREACH(&list->items, i) {
        struct tui_menu_item *item = ARRAY_AT(&list->items, i);
        tui_menu_item_methods[item->type].free_contents(item);
    }

    ARRAY_CLEAR(&list->items);
    list->scroll = 0;
    list->selected = 0;

    wclear(list->pad);
}

bool tui_menu_select_nth(struct tui_menu *list, size_t index) {
    if (index >= ARRAY_SIZE(&list->items)) {
        WARN("tried to select elem %zu of tui_menu that has %zu elems",
             index, ARRAY_SIZE(&list->items));
        return false;
    } else if (index == list->selected) {
        return false;
    } else {
        const size_t prev_selected = list->selected;
        list->selected = index;
        tui_menu_draw_nth(list, prev_selected);
        tui_menu_draw_nth(list, list->selected);
        return true;
    }
}

static bool tui_menu_select_prev_or_next(struct tui_menu *list, int direction) {
    const size_t old_index = list->selected;
    size_t next_index = old_index;
    bool looped = false;
    do {
        if (direction > 0) {
            next_index = (next_index + direction) % ARRAY_SIZE(&list->items);
        } else {
            next_index = MIN(next_index + direction, ARRAY_SIZE(&list->items) - 1);
        }
        if (next_index == old_index) {
            looped = true;
        }

        const struct tui_menu_item *i = ARRAY_AT(&list->items, next_index);
        if (tui_menu_item_methods[i->type].is_selectable(i)) {
            list->selected = next_index;
            break;
        }
    } while (!looped);

    if (old_index != list->selected) {
        tui_menu_draw_nth(list, old_index);
        tui_menu_draw_nth(list, list->selected);

        if (pnoutrefresh(list->pad, 0, 0,
                         list->screen_y, list->screen_x,
                         list->height - 1, list->width - 1) != OK) {
            ERROR("pnoutrefresh");
        }

        return true;
    } else {
        return false;
    }
}

bool tui_menu_select_next(struct tui_menu *list) {
    return tui_menu_select_prev_or_next(list, +1);
}

bool tui_menu_select_prev(struct tui_menu *list) {
    return tui_menu_select_prev_or_next(list, -1);
}

void tui_menu_activate(struct tui_menu *list) {
    METHOD_CALL(ARRAY_AT(&list->items, list->selected), activate);
}

void tui_menu_add_label(struct tui_menu *list,
                        const char *label) {
    struct tui_menu_item *i = ARRAY_EMPLACE_BACK(&list->items);
    *i = (struct tui_menu_item){
        .type = TUI_LIST_ITEM_TYPE_LABEL,
        .as.label = {
            .str = xstrdup(label),
        },
    };
}

void tui_menu_add_playlist_item(struct tui_menu *list,
                                int index, bool current, const struct song *song) {
    struct tui_menu_item *i = ARRAY_EMPLACE_BACK(&list->items);
    *i = (struct tui_menu_item){
        .type = TUI_LIST_ITEM_TYPE_PLAYLIST_ITEM,
        .as.playlist_item = {
            .index = index,
            .current = current,
            .song = xmalloc(sizeof(struct song)),
        },
    };
    song_deep_copy(i->as.playlist_item.song, song);
}

