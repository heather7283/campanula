#include "tui/menu.h"
#include "player/control.h"
#include "xmalloc.h"
#include "macros.h"
#include "log.h"

static void tui_menu_item_playlist_item_draw(const struct tui_menu_item *self,
                                             WINDOW *win, int ypos, int width) {
    const struct tui_menu_item_playlist_item *i = &self->as.playlist_item;
    const struct song *s = i->song;

    if (i->current) {
        wattron(win, A_BOLD);
    }

    mvwprintw(win, ypos, 0, "%s%d. %s - %s",
              i->current ? "> " : "", i->index, s->artist, s->title);

    if (i->current) {
        wattroff(win, A_BOLD);
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

static void tui_menu_item_playlist_item_copy(const struct tui_menu_item *self,
                                             struct tui_menu_item *other) {
    other->type = self->type;
    const struct tui_menu_item_playlist_item *s = &self->as.playlist_item;
    struct tui_menu_item_playlist_item *o = &other->as.playlist_item;

    o->current = s->current;
    o->index = s->index;
    o->song = xmalloc(sizeof(*o->song));
    song_deep_copy(o->song, s->song);
}

static void tui_menu_item_label_draw(const struct tui_menu_item *self,
                                     WINDOW *win, int ypos, int width) {
    const struct tui_menu_item_label *l = &self->as.label;

    mvwaddnstr(win, ypos, 0, l->str, width);
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

static void tui_menu_item_label_copy(const struct tui_menu_item *self,
                                     struct tui_menu_item *other) {
    other->type = self->type;
    const struct tui_menu_item_label *s = &self->as.label;
    struct tui_menu_item_label *o = &other->as.label;

    o->str = xstrdup(s->str);
}

typedef void (*tui_menu_item_method_draw)(const struct tui_menu_item *self,
                                          WINDOW *win, int ypos, int width);

typedef void (*tui_menu_item_method_free_contents)(struct tui_menu_item *self);

typedef bool (*tui_menu_item_method_is_selectable)(const struct tui_menu_item *self);

typedef void (*tui_menu_item_method_activate)(const struct tui_menu_item *self);

typedef void (*tui_menu_item_method_copy)(const struct tui_menu_item *self,
                                          struct tui_menu_item *other);

struct tui_menu_item_methods {
    const tui_menu_item_method_draw draw;
    const tui_menu_item_method_free_contents free_contents;
    const tui_menu_item_method_is_selectable is_selectable;
    const tui_menu_item_method_activate activate;
    const tui_menu_item_method_copy copy;
};

static const struct tui_menu_item_methods tui_menu_item_methods[] = {
    [TUI_LIST_ITEM_TYPE_LABEL] = {
        .draw = tui_menu_item_label_draw,
        .free_contents = tui_menu_item_label_free_contents,
        .is_selectable = tui_menu_item_label_is_selectable,
        .activate = tui_menu_item_label_activate,
        .copy = tui_menu_item_label_copy,
    },
    [TUI_LIST_ITEM_TYPE_PLAYLIST_ITEM] = {
        .draw = tui_menu_item_playlist_item_draw,
        .free_contents = tui_menu_item_playlist_item_free_contents,
        .is_selectable = tui_menu_item_playlist_item_is_selectable,
        .activate = tui_menu_item_playlist_item_activate,
        .copy = tui_menu_item_playlist_item_copy,
    }
};
static_assert(SIZEOF_VEC(tui_menu_item_methods) == TUI_LIST_ITEM_TYPE_COUNT);

#define METHOD_CALL(pobj, method, ...) \
    tui_menu_item_methods[(pobj)->type].method((pobj) __VA_OPT__(,) __VA_ARGS__)

void tui_menu_position(struct tui_menu *list, int screen_x, int screen_y, int width, int height) {
    list->screen_x = screen_x;
    list->screen_y = screen_y;
    list->width = width;
    list->height = height;

    if (list->win != NULL) {
        delwin(list->win);
    }
    list->win = newwin(height, width, screen_y, screen_x);
    scrollok(list->win, true);
}

void tui_menu_draw_nth(struct tui_menu *list, size_t index) {
    if (index < list->scroll || index > list->scroll + list->height - 1) {
        return;
    }

    if (index == list->selected) {
        wattron(list->win, A_REVERSE);
    }

    const struct tui_menu_item *item = VEC_AT(&list->items, index);
    METHOD_CALL(item, draw, list->win, index - list->scroll, list->width);

    if (index == list->selected) {
        wattroff(list->win, A_REVERSE);
    }
}

void tui_menu_draw(struct tui_menu *list) {
    const size_t lim = MIN((size_t)list->height, VEC_SIZE(&list->items));
    for (size_t i = 0; i < lim; i++) {
        tui_menu_draw_nth(list, list->scroll + i);
    }
    wclrtobot(list->win);

    if (wnoutrefresh(list->win) != OK) {
        ERROR("wnoutrefresh");
    }
}

void tui_menu_clear(struct tui_menu *list) {
    VEC_FOREACH(&list->items, i) {
        struct tui_menu_item *item = VEC_AT(&list->items, i);
        tui_menu_item_methods[item->type].free_contents(item);
    }

    VEC_CLEAR(&list->items);
    list->scroll = 0;
    list->selected = 0;

    wclear(list->win);
}

static bool tui_menu_ensure_visible(struct tui_menu *menu, size_t index) {
    if (index < menu->scroll) {
        const size_t diff = menu->scroll - index;
        wscrl(menu->win, -(int)diff);
        menu->scroll -= diff;

        for (size_t i = 0; i < MIN(diff, (size_t)menu->height); i++) {
            tui_menu_draw_nth(menu, menu->scroll + i);
        }

        return true;
    } else if (index > menu->scroll + menu->height - 1) {
        const size_t diff = index + 1 - menu->height - menu->scroll;
        wscrl(menu->win, (int)diff);
        menu->scroll += diff;

        for (size_t i = 0; i < MIN(diff, (size_t)menu->height); i++) {
            tui_menu_draw_nth(menu, menu->scroll + menu->height - 1 - i);
        }

        return true;
    }

    return false;
}

bool tui_menu_select_nth(struct tui_menu *list, size_t index) {
    if (index >= VEC_SIZE(&list->items)) {
        WARN("tried to select elem %zu of tui_menu that has %zu elems",
             index, VEC_SIZE(&list->items));
        return false;
    } else if (index == list->selected) {
        return false;
    } else if (METHOD_CALL(VEC_AT(&list->items, index), is_selectable)) {
        const size_t prev_selected = list->selected;
        list->selected = index;

        tui_menu_ensure_visible(list, list->selected);

        tui_menu_draw_nth(list, prev_selected);
        tui_menu_draw_nth(list, list->selected);
        return true;
    } else {
        return false;
    }
}

static bool tui_menu_select_prev_or_next(struct tui_menu *list, int direction) {
    const size_t old_index = list->selected;
    size_t next_index = old_index;
    bool looped = false;
    do {
        if (direction > 0) {
            next_index = (next_index + direction) % VEC_SIZE(&list->items);
        } else {
            next_index = MIN(next_index + direction, VEC_SIZE(&list->items) - 1);
        }
        if (next_index == old_index) {
            looped = true;
        }

        const struct tui_menu_item *i = VEC_AT(&list->items, next_index);
        if (tui_menu_item_methods[i->type].is_selectable(i)) {
            list->selected = next_index;
            break;
        }
    } while (!looped);

    if (old_index != list->selected) {
        tui_menu_ensure_visible(list, list->selected);

        tui_menu_draw_nth(list, old_index);
        tui_menu_draw_nth(list, list->selected);

        if (wnoutrefresh(list->win) != OK) {
            ERROR("wnoutrefresh");
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
    METHOD_CALL(VEC_AT(&list->items, list->selected), activate);
}

void tui_menu_append_item(struct tui_menu *list, const struct tui_menu_item *item) {
    struct tui_menu_item *i = VEC_EMPLACE_BACK(&list->items);
    METHOD_CALL(item, copy, i);
}

