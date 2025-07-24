#include <stdlib.h>

#include "api/types.h"
#include "collections.h"

static void free_error(struct api_type_error *o) {
    free(o->message);
}

static void free_songs(struct api_type_songs *o) {
    ARRAY_FREE(&o->song);
    /* TODO: free elements too */
}

/* I heckin love C man. What a great language */
void subsonic_response_free(struct subsonic_response *response) {
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

