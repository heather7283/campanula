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

void print_child(const struct api_type_child *c, enum log_level lvl, int indent) {
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

void print_artist(const struct api_type_artist *a, enum log_level lvl, int indent) {
    log_println(lvl, "%*sArtist {", indent, "");

    log_println(lvl, "%*sid: %s", indent + 4, "", a->id);
    log_println(lvl, "%*sname: %s", indent + 4, "", a->name);

    log_println(lvl, "%*s}", indent, "");
}

static void free_artist_id3(struct api_type_artist_id3 *a) {
    free(a->id);
    free(a->name);
}

void print_artist_id3(const struct api_type_artist_id3 *a, enum log_level lvl, int indent) {
    log_println(lvl, "%*sArtistID3 {", indent, "");

    log_println(lvl, "%*sid: %s", indent + 4, "", a->id);
    log_println(lvl, "%*sname: %s", indent + 4, "", a->name);
    log_println(lvl, "%*salbumCount: %d", indent + 4, "", a->album_count);

    log_println(lvl, "%*s}", indent, "");
}

static void free_album_id3(struct api_type_album_id3 *a) {
    free(a->id);
    free(a->name);
    free(a->created);
    free(a->artist);
    free(a->artist_id);
}

void print_album_id3(const struct api_type_album_id3 *a, enum log_level lvl, int indent) {
    log_println(lvl, "%*sAlbumID3 {", indent, "");

    log_println(lvl, "%*sid: %s", indent + 4, "", a->id);
    log_println(lvl, "%*sname: %s", indent + 4, "", a->name);
    log_println(lvl, "%*ssongCount: %d", indent + 4, "", a->song_count);
    log_println(lvl, "%*sduration: %d", indent + 4, "", a->duration);
    log_println(lvl, "%*screated: %s", indent + 4, "", a->created);

    log_println(lvl, "%*sartist: %s", indent + 4, "", a->artist);
    log_println(lvl, "%*sartistId: %s", indent + 4, "", a->artist_id);
    log_println(lvl, "%*syear: %d", indent + 4, "", a->year);

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
    VEC_FOREACH(&s->song, i) {
        struct api_type_child *c = VEC_AT(&s->song, i);
        free_child(c);
    }
    VEC_FREE(&s->song);
}

static void print_songs(const union subsonic_response_inner_object *o,
                        enum log_level lvl, int indent) {
    const struct api_type_songs *s = &o->songs;
    log_println(lvl, "%*sSongs {", indent, "");
    log_println(lvl, "%*ssong (%zu) [", indent + 4, "", VEC_SIZE(&s->song));
    VEC_FOREACH(&s->song, i) {
        struct api_type_child *c = VEC_AT(&s->song, i);
        print_child(c, lvl, indent + 8);
    }
    log_println(lvl, "%*s]", indent + 4, "");
    log_println(lvl, "%*s}", indent, "");
}

static void free_album_list(union subsonic_response_inner_object *o) {
    struct api_type_album_list *l = &o->album_list;
    VEC_FOREACH(&l->album, i) {
        struct api_type_child *c = VEC_AT(&l->album, i);
        free_child(c);
    }
    VEC_FREE(&l->album);
}

static void print_album_list(const union subsonic_response_inner_object *o,
                             enum log_level lvl, int indent) {
    const struct api_type_album_list *l = &o->album_list;
    log_println(lvl, "%*sAlbumList {", indent, "");
    log_println(lvl, "%*salbum (%zu) [", indent + 4, "", VEC_SIZE(&l->album));
    VEC_FOREACH(&l->album, i) {
        struct api_type_child *c = VEC_AT(&l->album, i);
        print_child(c, lvl, indent + 8);
    }
    log_println(lvl, "%*s]", indent + 4, "");
    log_println(lvl, "%*s}", indent, "");
}

static void free_search_result_2(union subsonic_response_inner_object *o) {
    struct api_type_search_result_2 *r = &o->search_result_2;
    VEC_FOREACH(&r->artist, i) {
        free_artist(VEC_AT(&r->artist, i));
    }
    VEC_FREE(&r->artist);
    VEC_FOREACH(&r->album, i) {
        free_child(VEC_AT(&r->album, i));
    }
    VEC_FREE(&r->album);
    VEC_FOREACH(&r->song, i) {
        free_child(VEC_AT(&r->song, i));
    }
    VEC_FREE(&r->song);
}

static void print_search_result_2(const union subsonic_response_inner_object *o,
                                  enum log_level lvl, int indent) {
    const struct api_type_search_result_2 *r = &o->search_result_2;
    log_println(lvl, "%*sSearchResult2 {", indent, "");

    log_println(lvl, "%*sartist (%zu) [", indent + 4, "", VEC_SIZE(&r->artist));
    VEC_FOREACH(&r->artist, i) {
        print_artist(VEC_AT(&r->artist, i), lvl, indent + 8);
    }
    log_println(lvl, "%*s]", indent + 4, "");

    log_println(lvl, "%*salbum (%zu) [", indent + 4, "", VEC_SIZE(&r->album));
    VEC_FOREACH(&r->album, i) {
        print_child(VEC_AT(&r->album, i), lvl, indent + 8);
    }
    log_println(lvl, "%*s]", indent + 4, "");

    log_println(lvl, "%*ssong (%zu) [", indent + 4, "", VEC_SIZE(&r->song));
    VEC_FOREACH(&r->song, i) {
        print_child(VEC_AT(&r->song, i), lvl, indent + 8);
    }
    log_println(lvl, "%*s]", indent + 4, "");

    log_println(lvl, "%*s}", indent, "");
}

static void free_search_result_3(union subsonic_response_inner_object *o) {
    struct api_type_search_result_3 *r = &o->search_result_3;
    VEC_FOREACH(&r->artist, i) {
        free_artist_id3(VEC_AT(&r->artist, i));
    }
    VEC_FREE(&r->artist);
    VEC_FOREACH(&r->album, i) {
        free_album_id3(VEC_AT(&r->album, i));
    }
    VEC_FREE(&r->album);
    VEC_FOREACH(&r->song, i) {
        free_child(VEC_AT(&r->song, i));
    }
    VEC_FREE(&r->song);
}

static void print_search_result_3(const union subsonic_response_inner_object *o,
                                  enum log_level lvl, int indent) {
    const struct api_type_search_result_3 *r = &o->search_result_3;
    log_println(lvl, "%*sSearchResult3 {", indent, "");

    log_println(lvl, "%*sartist (%zu) [", indent + 4, "", VEC_SIZE(&r->artist));
    VEC_FOREACH(&r->artist, i) {
        print_artist_id3(VEC_AT(&r->artist, i), lvl, indent + 8);
    }
    log_println(lvl, "%*s]", indent + 4, "");

    log_println(lvl, "%*salbum (%zu) [", indent + 4, "", VEC_SIZE(&r->album));
    VEC_FOREACH(&r->album, i) {
        print_album_id3(VEC_AT(&r->album, i), lvl, indent + 8);
    }
    log_println(lvl, "%*s]", indent + 4, "");

    log_println(lvl, "%*ssong (%zu) [", indent + 4, "", VEC_SIZE(&r->song));
    VEC_FOREACH(&r->song, i) {
        print_child(VEC_AT(&r->song, i), lvl, indent + 8);
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
    [API_TYPE_SEARCH_RESULT_3] = { free_search_result_3, print_search_result_3 },
};
static_assert(SIZEOF_VEC(inner_object_funcs) == SUBSONIC_RESPONSE_INNER_OBJECT_TYPE_COUNT);

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

