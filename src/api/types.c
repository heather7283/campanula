#include <stdlib.h>

#include "api/types.h"

static void free_error(struct api_type_error *o) {
    free(o->message);
}

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

static void free_songs(struct api_type_songs *o) {
    ARRAY_FOREACH(&o->song, i) {
        struct api_type_child *c = &ARRAY_AT(&o->song, i);
        free_child(c);
    }
    ARRAY_FREE(&o->song);
}

/* I heckin love C man. What a great language */
void subsonic_response_free(struct subsonic_response *response) {
    if (response == NULL) {
        return;
    }

    switch (response->inner_object_type) {
    case API_TYPE_ERROR:
        free_error(&response->inner_object.error);
        break;
    case API_TYPE_SONGS:
        free_songs(&response->inner_object.random_songs);
        break;
    }

    free(response->version);
    free(response);
}

