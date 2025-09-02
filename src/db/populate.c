#include "db/populate.h"
#include "db/internal.h"
#include "api/requests.h"

static bool insert_artist(const struct api_type_artist_id3 *a) {
    [[gnu::cleanup(statement_resetp)]]
    struct sqlite3_stmt *stmt = statements[STATEMENT_INSERT_ARTIST].stmt;

    STMT_BIND(stmt, text, "$id", a->id, -1, SQLITE_STATIC);
    STMT_BIND(stmt, text, "$name", a->name, -1, SQLITE_STATIC);

    int ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        ERROR("failed to insert artist %s into db: %s", a->id, sqlite3_errmsg(db));
        return false;
    }

    return true;
}

static bool insert_album(const struct api_type_album_id3 *a) {
    [[gnu::cleanup(statement_resetp)]]
    struct sqlite3_stmt *stmt = statements[STATEMENT_INSERT_ALBUM].stmt;

    STMT_BIND(stmt, text, "$id", a->id, -1, SQLITE_STATIC);
    STMT_BIND(stmt, text, "$name", a->name, -1, SQLITE_STATIC);
    STMT_BIND(stmt, text, "$artist", a->artist, -1, SQLITE_STATIC);
    STMT_BIND(stmt, text, "$artist_id", a->artist_id, -1, SQLITE_STATIC);
    STMT_BIND(stmt, text, "$created", a->created, -1, SQLITE_STATIC);
    STMT_BIND(stmt, int64, "$song_count", a->song_count);
    STMT_BIND(stmt, int64, "$duration", a->duration);

    int ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        ERROR("failed to insert album %s into db: %s", a->id, sqlite3_errmsg(db));
        return false;
    }

    return true;
}

static bool insert_song(const struct api_type_child *s) {
    [[gnu::cleanup(statement_resetp)]]
    struct sqlite3_stmt *stmt = statements[STATEMENT_INSERT_SONG].stmt;

    STMT_BIND(stmt, text, "$id", s->id, -1, SQLITE_STATIC);
    STMT_BIND(stmt, text, "$title", s->title, -1, SQLITE_STATIC);
    STMT_BIND(stmt, text, "$artist", s->artist, -1, SQLITE_STATIC);
    STMT_BIND(stmt, text, "$album", s->album, -1, SQLITE_STATIC);

    STMT_BIND(stmt, int64, "$track", s->track);
    STMT_BIND(stmt, int64, "$year", s->year);
    STMT_BIND(stmt, int64, "$duration", s->duration);
    STMT_BIND(stmt, int64, "$bitrate", s->bit_rate);
    STMT_BIND(stmt, int64, "$size", s->size);

    STMT_BIND(stmt, text, "$filetype", s->suffix, -1, SQLITE_STATIC);

    STMT_BIND(stmt, text, "$artist_id", s->artist_id, -1, SQLITE_STATIC);
    STMT_BIND(stmt, text, "$album_id", s->album_id, -1, SQLITE_STATIC);

    int ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        ERROR("failed to insert song %s into db: %s; song:", s->id, sqlite3_errmsg(db));
        print_child(s, LOG_ERROR, 0);
        return false;
    }

    return true;
}

struct db_populate_data {
    enum { START, ARTISTS, ALBUMS, SONGS, END } state;
    size_t count, offset;
};

static void on_db_populate_response(const char *errmsg,
                                    const struct subsonic_response *resp, void *data) {
    struct db_populate_data *d = data;

    if (errmsg != NULL) {
        ERROR("while populating db: api error: %s", errmsg);
        goto err;
    }

    const struct api_type_search_result_3 *sr3 = &resp->inner_object.search_result_3;
    switch (d->state) {
    case START: {
        TRACE("db_populate: starting transaction");
        if (!statement_execute(STATEMENT_BEGIN)) {
            ERROR("failed to start transaction: %s", sqlite3_errmsg(db));
            return;
        }

        TRACE("db_populate: marking all entries as deleted");
        if (!statement_execute(STATEMENT_MARK_ARTISTS_AS_DELETED)) {
            ERROR("failed to mark artists as deleted: %s", sqlite3_errmsg(db));
            goto err;
        }
        if (!statement_execute(STATEMENT_MARK_ALBUMS_AS_DELETED)) {
            ERROR("failed to mark albums as deleted: %s", sqlite3_errmsg(db));
            goto err;
        }
        if (!statement_execute(STATEMENT_MARK_SONGS_AS_DELETED)) {
            ERROR("failed to mark albums as deleted: %s", sqlite3_errmsg(db));
            goto err;
        }

        d->state = ARTISTS;
        goto artists;
    }
    case ARTISTS: {
        VEC_FOREACH(&sr3->artist, i) {
            struct api_type_artist_id3 *artist = VEC_AT(&sr3->artist, i);
            if (!insert_artist(artist)) {
                goto err;
            }
        }
        if (VEC_SIZE(&sr3->artist) < d->count) {
            /* there are no more entries of this type */
            d->state = ALBUMS;
            d->offset = 0;
            goto albums;
        } else {
            d->offset += VEC_SIZE(&sr3->artist);
            goto artists;
        }
        break;
    }
    case ALBUMS: {
        VEC_FOREACH(&sr3->album, i) {
            struct api_type_album_id3 *album = VEC_AT(&sr3->album, i);
            if (!insert_album(album)) {
                goto err;
            }
        }
        if (VEC_SIZE(&sr3->album) < d->count) {
            /* there are no more entries of this type */
            d->state = SONGS;
            d->offset = 0;
            goto songs;
        } else {
            d->offset += VEC_SIZE(&sr3->album);
            goto albums;
        }
        break;
    }
    case SONGS: {
        VEC_FOREACH(&sr3->song, i) {
            struct api_type_child *child = VEC_AT(&sr3->song, i);
            if (!insert_song(child)) {
                goto err;
            }
        }
        if (VEC_SIZE(&sr3->song) < d->count) {
            /* there are no more entries of this type */
            goto fin;
        } else {
            d->offset += VEC_SIZE(&sr3->song);
            goto songs;
        }
        break;
    }
    case END: {
        assert(0 && "UNREACHABLE: END");
    }
    }

artists:
    TRACE("db_populate: requesting %zu artists at offset %zu", d->count, d->offset);
    api_search3("", d->count, d->offset, 0, 0, 0, 0, NULL, on_db_populate_response, d);
    return;

albums:
    TRACE("db_populate: requesting %zu albums at offset %zu", d->count, d->offset);
    api_search3("", 0, 0, d->count, d->offset, 0, 0, NULL, on_db_populate_response, d);
    return;

songs:
    TRACE("db_populate: requesting %zu songs at offset %zu", d->count, d->offset);
    api_search3("", 0, 0, 0, 0, d->count, d->offset, NULL, on_db_populate_response, d);
    return;

fin:
    TRACE("db_populate: deleting deleted entries");

    if (!statement_execute(STATEMENT_DELETE_DELETED_SONGS)) {
        ERROR("db_populate: failed to delete deleted songs: %s", sqlite3_errmsg(db));
        goto err;
    }
    TRACE("db_populate: deleted %lli songs", sqlite3_changes64(db));

    if (!statement_execute(STATEMENT_DELETE_DELETED_ALBUMS)) {
        ERROR("db_populate: failed to delete deleted albums: %s", sqlite3_errmsg(db));
        goto err;
    }
    TRACE("db_populate: deleted %lli albums", sqlite3_changes64(db));

    if (!statement_execute(STATEMENT_DELETE_DELETED_ARTISTS)) {
        ERROR("db_populate: failed to delete deleted artists: %s", sqlite3_errmsg(db));
        goto err;
    }
    TRACE("db_populate: deleted %lli artists", sqlite3_changes64(db));

    TRACE("db_populate: committing transaction");
    if (!statement_execute(STATEMENT_COMMIT)) {
        ERROR("db_populate: failed to commit transaction: %s", sqlite3_errmsg(db));
        goto err;
    }

    d->state = END;

    return;

err:
    TRACE("db_populate: rolling back transaction");
    if (!statement_execute(STATEMENT_ROLLBACK)) {
        ERROR("db_populate: failed to rollback transaction: %s", sqlite3_errmsg(db));
    }

    d->state = END;

    return;
}

bool db_populate(void) {
    static struct db_populate_data data = {
        .count = 5000,
        .offset = 0,
        .state = END,
    };

    if (data.state != END) {
        ERROR("state is not END (another db_populate is still not finished?)");
        return false;
    }

    data.state = START;
    on_db_populate_response(NULL, NULL, &data); /* kickstart it */
    return true;
}

