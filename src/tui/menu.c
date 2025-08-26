#include "tui/menu.h"
#include "player/control.h"
#include "player/playlist.h"
#include "xmalloc.h"
#include "macros.h"
#include "log.h"

static void tui_menu_item_song_draw(const struct tui_menu_item *self,
                                     WINDOW *win, int ypos, int width) {
    const struct tui_menu_item_song *i = &self->as.song;
    const struct song *s = i->song;

    mvwprintw(win, ypos, 0, "%s - %s", s->artist, s->title);

    wattron(win, A_DIM);
    wprintw(win, " (%d:%02d)", s->duration / 60, s->duration % 60);
    wattroff(win, A_DIM);

    wclrtoeol(win);
}

static void tui_menu_item_song_free_contents(struct tui_menu_item *self) {
    struct tui_menu_item_song *s = &self->as.song;

    song_free_contents(s->song);
    free(s->song);
}

static bool tui_menu_item_song_is_selectable(const struct tui_menu_item *self) {
    return true;
}

static void tui_menu_item_song_activate(const struct tui_menu_item *self) {
    const struct tui_menu_item_song *i = &self->as.song;
    const struct song *s = i->song;

    playlist_append_song(s);
}

static void tui_menu_item_song_copy(const struct tui_menu_item *self,
                                    struct tui_menu_item *other) {
    other->type = self->type;
    const struct tui_menu_item_song *s = &self->as.song;
    struct tui_menu_item_song *o = &other->as.song;

    o->song = xmalloc(sizeof(*o->song));
    song_deep_copy(o->song, s->song);
}

static void tui_menu_item_empty_draw(const struct tui_menu_item *self,
                                     WINDOW *win, int ypos, int width) {
    wmove(win, ypos, 0);
    wclrtoeol(win);
}

static void tui_menu_item_empty_free_contents(struct tui_menu_item *self) {
    /* no-op */
}

static bool tui_menu_item_empty_is_selectable(const struct tui_menu_item *self) {
    return false;
}

static void tui_menu_item_empty_activate(const struct tui_menu_item *self) {
    /* no-op */
}

static void tui_menu_item_empty_copy(const struct tui_menu_item *self,
                                     struct tui_menu_item *other) {
    other->type = self->type;
}

static void tui_menu_item_playlist_item_draw(const struct tui_menu_item *self,
                                             WINDOW *win, int ypos, int width) {
    const struct tui_menu_item_playlist_item *i = &self->as.playlist_item;
    const struct song *s = i->song;

    if (i->current) {
        wattron(win, A_BOLD);
    }

    mvwprintw(win, ypos, 0, "%s%d. %s - %s",
              i->current ? "> " : "",
              i->index, s->artist, s->title);

    wattron(win, A_DIM);
    wprintw(win, " (%d:%02d)", s->duration / 60, s->duration % 60);
    wattroff(win, A_DIM);

    wclrtoeol(win);

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
    wclrtoeol(win);
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
    [TUI_MENU_ITEM_TYPE_EMPTY] = {
        .draw = tui_menu_item_empty_draw,
        .free_contents = tui_menu_item_empty_free_contents,
        .is_selectable = tui_menu_item_empty_is_selectable,
        .activate = tui_menu_item_empty_activate,
        .copy = tui_menu_item_empty_copy,
    },
    [TUI_MENU_ITEM_TYPE_LABEL] = {
        .draw = tui_menu_item_label_draw,
        .free_contents = tui_menu_item_label_free_contents,
        .is_selectable = tui_menu_item_label_is_selectable,
        .activate = tui_menu_item_label_activate,
        .copy = tui_menu_item_label_copy,
    },
    [TUI_MENU_ITEM_TYPE_PLAYLIST_ITEM] = {
        .draw = tui_menu_item_playlist_item_draw,
        .free_contents = tui_menu_item_playlist_item_free_contents,
        .is_selectable = tui_menu_item_playlist_item_is_selectable,
        .activate = tui_menu_item_playlist_item_activate,
        .copy = tui_menu_item_playlist_item_copy,
    },
    [TUI_MENU_ITEM_TYPE_SONG] = {
        .draw = tui_menu_item_song_draw,
        .free_contents = tui_menu_item_song_free_contents,
        .is_selectable = tui_menu_item_song_is_selectable,
        .activate = tui_menu_item_song_activate,
        .copy = tui_menu_item_song_copy,
    },
};
static_assert(SIZEOF_VEC(tui_menu_item_methods) == TUI_MENU_ITEM_TYPE_COUNT);

#define METHOD_CALL(pobj, method, ...) \
    tui_menu_item_methods[(pobj)->type].method((pobj) __VA_OPT__(,) __VA_ARGS__)

void tui_menu_position(struct tui_menu *menu, int screen_x, int screen_y, int width, int height) {
    menu->screen_x = screen_x;
    menu->screen_y = screen_y;
    menu->width = width - 1 /* scrollbar */;
    menu->height = height;

    if (menu->win != NULL) {
        delwin(menu->win);
        menu->win = NULL;
    }
    menu->win = newwin(menu->height, menu->width, menu->screen_y, menu->screen_x);
    scrollok(menu->win, true);

    if (menu->scrollbar_win != NULL) {
        delwin(menu->scrollbar_win);
        menu->scrollbar_win = NULL;
    }
    menu->scrollbar_win = newwin(menu->height, 1, menu->screen_y, menu->screen_x + menu->width);
}

void tui_menu_draw_scrollbar(struct tui_menu *menu) {
    const int n_items = VEC_SIZE(&menu->items);
    if (n_items == 0) {
        return;
    }

    const int n_visible = MIN(VEC_SIZE(&menu->items) - menu->scroll, menu->height);
    TRACE("tui_menu_draw_scrollbar: n_items %d n_visible %d", n_items, n_visible);

    int scrollbar_height;
    int scrollbar_pos;
    if (n_items <= n_visible) {
        scrollbar_pos = 0;
        scrollbar_height = menu->height;
    } else {
        const float h = ((float)n_visible / n_items) * menu->height;
        scrollbar_height = MAX(1, h);

        const float frac = (float)menu->scroll  / (n_items - n_visible);
        scrollbar_pos = frac * (menu->height - scrollbar_height);
    }
    TRACE("tui_menu_draw_scrollbar: pos %d height %d", scrollbar_pos, scrollbar_height);

    for (int i = 0; i < menu->height; i++) {
        const char c = (i < scrollbar_pos || i >= scrollbar_pos + scrollbar_height) ? ' ' : '#';
        mvwaddch(menu->scrollbar_win, i, 0, c);
    }
    wnoutrefresh(menu->scrollbar_win);
}

bool tui_menu_draw_item(struct tui_menu *menu, size_t index) {
    if (index < menu->scroll || index > menu->scroll + menu->height - 1) {
        return false;
    }

    if (index == menu->selected) {
        wattron(menu->win, A_REVERSE);
    }

    const struct tui_menu_item *item = VEC_AT(&menu->items, index);
    METHOD_CALL(item, draw, menu->win, index - menu->scroll, menu->width - 1 /* scrollbar */);

    if (index == menu->selected) {
        wattroff(menu->win, A_REVERSE);
    }

    wnoutrefresh(menu->win);

    return true;
}

void tui_menu_draw(struct tui_menu *menu) {
    const size_t lim = MIN((size_t)menu->height, VEC_SIZE(&menu->items) - menu->scroll);
    for (size_t i = 0; i < lim; i++) {
        tui_menu_draw_item(menu, menu->scroll + i);
    }

    if (lim < (size_t)menu->height) {
        wclrtobot(menu->win);
        wnoutrefresh(menu->win);
    }

    tui_menu_draw_scrollbar(menu);
}

struct tui_menu_item *tui_menu_get_item(struct tui_menu *menu, size_t index) {
    return VEC_AT(&menu->items, index);
}

bool tui_menu_remove_item(struct tui_menu *menu, size_t index) {
    bool ret = false;

    struct tui_menu_item *i = VEC_AT(&menu->items, index);
    METHOD_CALL(i, free_contents);

    VEC_ERASE(&menu->items, index);
    if (index >= VEC_SIZE(&menu->items)) {
        menu->selected = VEC_SIZE(&menu->items) - 1;
    }

    if (index < menu->scroll + menu->height) {
        const size_t count = MIN((menu->scroll + menu->height - index), VEC_SIZE(&menu->items));
        for (size_t i = 0; i < count; i++) {
            tui_menu_draw_item(menu, index + i);
        }
        ret = true;
    }

    tui_menu_draw_scrollbar(menu);

    return ret;
}

void tui_menu_clear(struct tui_menu *menu) {
    VEC_FOREACH(&menu->items, i) {
        struct tui_menu_item *item = VEC_AT(&menu->items, i);
        tui_menu_item_methods[item->type].free_contents(item);
    }

    VEC_CLEAR(&menu->items);
    menu->scroll = 0;
    menu->selected = 0;

    wclear(menu->win);

    tui_menu_draw_scrollbar(menu);
}

static bool tui_menu_ensure_visible(struct tui_menu *menu, size_t index) {
    if (index < menu->scroll) {
        const size_t diff = menu->scroll - index;
        wscrl(menu->win, -(int)diff);
        menu->scroll -= diff;

        for (size_t i = 0; i < MIN(diff, (size_t)menu->height); i++) {
            tui_menu_draw_item(menu, menu->scroll + i);
        }

        tui_menu_draw_scrollbar(menu);

        return true;
    } else if (index > menu->scroll + menu->height - 1) {
        const size_t diff = index + 1 - menu->height - menu->scroll;
        wscrl(menu->win, (int)diff);
        menu->scroll += diff;

        for (size_t i = 0; i < MIN(diff, (size_t)menu->height); i++) {
            tui_menu_draw_item(menu, menu->scroll + menu->height - 1 - i);
        }

        tui_menu_draw_scrollbar(menu);

        return true;
    }

    return false;
}

bool tui_menu_select_nth(struct tui_menu *menu, size_t index) {
    if (index >= VEC_SIZE(&menu->items)) {
        WARN("tried to select elem %zu of tui_menu that has %zu elems",
             index, VEC_SIZE(&menu->items));
        return false;
    } else if (index == menu->selected) {
        return false;
    } else if (METHOD_CALL(VEC_AT(&menu->items, index), is_selectable)) {
        const size_t prev_selected = menu->selected;
        menu->selected = index;

        tui_menu_ensure_visible(menu, menu->selected);

        tui_menu_draw_item(menu, prev_selected);
        tui_menu_draw_item(menu, menu->selected);

        return true;
    } else {
        return false;
    }
}

static bool tui_menu_select_prev_or_next(struct tui_menu *menu, int direction) {
    const size_t old_index = menu->selected;
    size_t next_index = old_index;
    bool looped = false;
    do {
        if (direction > 0) {
            next_index = (next_index + direction) % VEC_SIZE(&menu->items);
        } else {
            next_index = MIN(next_index + direction, VEC_SIZE(&menu->items) - 1);
        }
        if (next_index == old_index) {
            looped = true;
        }

        const struct tui_menu_item *i = VEC_AT(&menu->items, next_index);
        if (tui_menu_item_methods[i->type].is_selectable(i)) {
            menu->selected = next_index;
            break;
        }
    } while (!looped);

    if (old_index != menu->selected) {
        tui_menu_ensure_visible(menu, menu->selected);

        tui_menu_draw_item(menu, old_index);
        tui_menu_draw_item(menu, menu->selected);

        return true;
    } else {
        return false;
    }
}

bool tui_menu_select_next(struct tui_menu *menu) {
    return tui_menu_select_prev_or_next(menu, +1);
}

bool tui_menu_select_prev(struct tui_menu *menu) {
    return tui_menu_select_prev_or_next(menu, -1);
}

void tui_menu_activate(struct tui_menu *menu) {
    METHOD_CALL(VEC_AT(&menu->items, menu->selected), activate);
}

bool tui_menu_append_item(struct tui_menu *menu, const struct tui_menu_item *item) {
    struct tui_menu_item *new_item = VEC_EMPLACE_BACK(&menu->items);
    METHOD_CALL(item, copy, new_item);

    bool ret = tui_menu_draw_item(menu, VEC_SIZE(&menu->items) - 1);
    tui_menu_draw_scrollbar(menu);

    return ret;
}

bool tui_menu_insert_or_replace_item(struct tui_menu *menu, size_t index,
                                     const struct tui_menu_item *item) {
    struct tui_menu_item *new_item = NULL;
    bool ret = false;

    if (index >= VEC_SIZE(&menu->items)) {
        /* fill with blanks */
        const size_t first_blank = VEC_SIZE(&menu->items) - 1;
        const size_t n_blanks = index - VEC_SIZE(&menu->items);
        struct tui_menu_item *blanks = VEC_EMPLACE_BACK_N(&menu->items, n_blanks);

        for (size_t i = 0; i < n_blanks; i++) {
            blanks[i].type = TUI_MENU_ITEM_TYPE_EMPTY;
            ret = tui_menu_draw_item(menu, first_blank + i) || ret;
        }

        tui_menu_draw_scrollbar(menu);

        new_item = VEC_EMPLACE_BACK(&menu->items);
    } else {
        struct tui_menu_item *old_item = VEC_AT(&menu->items, index);
        METHOD_CALL(old_item, free_contents);

        new_item = old_item;
    }

    METHOD_CALL(item, copy, new_item);
    ret = tui_menu_draw_item(menu, index) || ret;

    return ret;
}

