#include <assert.h>
#include <stdlib.h>

#include "api/types.h"

static void free_child(struct api_type_child *c) {
    free(c->id);
    free(c->title);

    free(c->parent);
    free(c->album);
    free(c->artist);
    free(c->content_type);
    free(c->suffix);
    free(c->album_id);
    free(c->artist_id);
}

static void print_child(const struct api_type_child *c, enum log_level lvl, int indent) {
    log_println(lvl, "%*sChild {", indent, "");

    log_println(lvl, "%*sid: %s", indent + 4, "", c->id);
    log_println(lvl, "%*stitle: %s", indent + 4, "", c->title);
    log_println(lvl, "%*sisDir: %d", indent + 4, "", c->is_dir);

    log_println(lvl, "%*salbum: %s", indent + 4, "", c->album);
    log_println(lvl, "%*salbumId: %s", indent + 4, "", c->album_id);

    log_println(lvl, "%*sartist: %s", indent + 4, "", c->artist);
    log_println(lvl, "%*sartistId: %s", indent + 4, "", c->artist_id);

    log_println(lvl, "%*strack: %d", indent + 4, "", c->track);
    log_println(lvl, "%*syear: %d", indent + 4, "", c->year);
    log_println(lvl, "%*ssize: %li", indent + 4, "", c->size);
    log_println(lvl, "%*sduration: %d", indent + 4, "", c->duration);
    log_println(lvl, "%*sbitRate: %d", indent + 4, "", c->bit_rate);

    log_println(lvl, "%*sparent: %s", indent + 4, "", c->parent);
    log_println(lvl, "%*scontentType: %s", indent + 4, "", c->content_type);
    log_println(lvl, "%*ssuffix: %s", indent + 4, "", c->suffix);

    log_println(lvl, "%*s}", indent, "");
}

static void free_artist(struct api_type_artist *a) {
    free(a->id);
    free(a->name);
}

static void print_artist(const struct api_type_artist *a, enum log_level lvl, int indent) {
    log_println(lvl, "%*sArtist {", indent, "");

    log_println(lvl, "%*sid: %s", indent + 4, "", a->id);
    log_println(lvl, "%*sname: %s", indent + 4, "", a->name);

    log_println(lvl, "%*s}", indent, "");
}

static void free_error(union subsonic_response_inner_object *o) {
    struct api_type_error *e = &o->error;
    free(e->message);
}

static void print_error(const union subsonic_response_inner_object *o,
                        enum log_level lvl, int indent) {
    const struct api_type_error *e = &o->error;
    log_println(lvl, "%*sError {", indent, "");
    log_println(lvl, "%*s%d", indent + 4, "", e->code);
    log_println(lvl, "%*s%s", indent + 4, "", e->message);
    log_println(lvl, "%*s}", indent, "");
}

static void free_songs(union subsonic_response_inner_object *o) {
    struct api_type_songs *s = &o->songs;
    ARRAY_FOREACH(&s->song, i) {
        struct api_type_child *c = ARRAY_AT(&s->song, i);
        free_child(c);
    }
    ARRAY_FREE(&s->song);
}

static void print_songs(const union subsonic_response_inner_object *o,
                        enum log_level lvl, int indent) {
    const struct api_type_songs *s = &o->songs;
    log_println(lvl, "%*sSongs {", indent, "");
    log_println(lvl, "%*ssong [", indent + 4, "");
    ARRAY_FOREACH(&s->song, i) {
        struct api_type_child *c = ARRAY_AT(&s->song, i);
        print_child(c, lvl, indent + 8);
    }
    log_println(lvl, "%*s]", indent + 4, "");
    log_println(lvl, "%*s}", indent, "");
}

static void free_album_list(union subsonic_response_inner_object *o) {
    struct api_type_album_list *l = &o->album_list;
    ARRAY_FOREACH(&l->album, i) {
        struct api_type_child *c = ARRAY_AT(&l->album, i);
        free_child(c);
    }
    ARRAY_FREE(&l->album);
}

static void print_album_list(const union subsonic_response_inner_object *o,
                             enum log_level lvl, int indent) {
    const struct api_type_album_list *l = &o->album_list;
    log_println(lvl, "%*sAlbumList {", indent, "");
    log_println(lvl, "%*salbum [", indent + 4, "");
    ARRAY_FOREACH(&l->album, i) {
        struct api_type_child *c = ARRAY_AT(&l->album, i);
        print_child(c, lvl, indent + 8);
    }
    log_println(lvl, "%*s]", indent + 4, "");
    log_println(lvl, "%*s}", indent, "");
}

static void free_search_result_2(union subsonic_response_inner_object *o) {
    struct api_type_search_result_2 *r = &o->search_result_2;
    ARRAY_FOREACH(&r->artist, i) {
        free_artist(ARRAY_AT(&r->artist, i));
    }
    ARRAY_FREE(&r->artist);
    ARRAY_FOREACH(&r->album, i) {
        free_child(ARRAY_AT(&r->album, i));
    }
    ARRAY_FREE(&r->album);
    ARRAY_FOREACH(&r->song, i) {
        free_child(ARRAY_AT(&r->song, i));
    }
    ARRAY_FREE(&r->song);
}

static void print_search_result_2(const union subsonic_response_inner_object *o,
                                  enum log_level lvl, int indent) {
    const struct api_type_search_result_2 *r = &o->search_result_2;
    log_println(lvl, "%*sSearchResult2 {", indent, "");

    log_println(lvl, "%*sartist [", indent + 4, "");
    ARRAY_FOREACH(&r->artist, i) {
        print_artist(ARRAY_AT(&r->artist, i), lvl, indent + 8);
    }
    log_println(lvl, "%*s]", indent + 4, "");

    log_println(lvl, "%*salbum [", indent + 4, "");
    ARRAY_FOREACH(&r->album, i) {
        print_child(ARRAY_AT(&r->album, i), lvl, indent + 8);
    }
    log_println(lvl, "%*s]", indent + 4, "");

    log_println(lvl, "%*ssong [", indent + 4, "");
    ARRAY_FOREACH(&r->song, i) {
        print_child(ARRAY_AT(&r->song, i), lvl, indent + 8);
    }
    log_println(lvl, "%*s]", indent + 4, "");

    log_println(lvl, "%*s}", indent, "");
}

static const struct {
    void (*free)(union subsonic_response_inner_object *o);
    void (*print)(const union subsonic_response_inner_object *o, enum log_level lvl, int indent);
} inner_object_funcs[] = {
    [API_TYPE_ERROR] = { free_error, print_error },
    [API_TYPE_SONGS] = { free_songs, print_songs },
    [API_TYPE_ALBUM_LIST] = { free_album_list, print_album_list },
    [API_TYPE_SEARCH_RESULT_2] = { free_search_result_2, print_search_result_2 },
};
static_assert(SIZEOF_ARRAY(inner_object_funcs) == SUBSONIC_RESPONSE_INNER_OBJECT_TYPE_COUNT);

void subsonic_response_free(struct subsonic_response *response) {
    if (response == NULL) {
        return;
    }

    inner_object_funcs[response->inner_object_type].free(&response->inner_object);

    free(response->version);
    free(response);
}

void subsonic_response_print(const struct subsonic_response *resp, enum log_level lvl) {
    if (resp == NULL) {
        log_println(lvl, "subsonic-response (NULL)");
        return;
    }

    log_println(lvl, "subsonic-response {");

    log_println(lvl, "%*sstatus: %d", 4, "", resp->status);
    log_println(lvl, "%*sversion: %s", 4, "", resp->version);

    inner_object_funcs[resp->inner_object_type].print(&resp->inner_object, lvl, 4);

    log_println(lvl, "}");
}

