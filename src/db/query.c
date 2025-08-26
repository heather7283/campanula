#include "db/query.h"
#include "db/internal.h"
#include "collections/vec.h"
#include "xmalloc.h"
#include "log.h"

size_t db_search_artists(struct artist **partists, const char *query,
                         size_t page, size_t artists_per_page) {
    struct sqlite3_stmt *stmt;
    VEC(struct artist) artists = {0};

    if (query == NULL) {
        stmt = statements[STATEMENT_GET_ARTISTS_WITH_PAGINATION].stmt;
    } else {
        stmt = statements[STATEMENT_SEARCH_ARTISTS_WITH_PAGINATION].stmt;
    }

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    STMT_BIND(stmt, int64, "$select_count", artists_per_page);
    STMT_BIND(stmt, int64, "$select_offset", page * artists_per_page);
    if (query != NULL) {
        STMT_BIND(stmt, text, "$query", query, -1, SQLITE_STATIC);
    }

    int ret;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        struct artist *a = VEC_EMPLACE_BACK(&artists);

        a->id = xstrdup((char *)sqlite3_column_text(stmt, 0));
        a->name = xstrdup((char *)sqlite3_column_text(stmt, 1));
    }
    if (ret != SQLITE_DONE) {
        ERROR("failed to fetch albums from db: %s", sqlite3_errmsg(db));
        VEC_FREE(&artists);
        *partists = NULL;
        return 0;
    }

    *partists = VEC_DATA(&artists);
    return VEC_SIZE(&artists);
}

size_t db_get_artists(struct artist **artists, size_t page, size_t artists_per_page) {
    return db_search_artists(artists, NULL, page, artists_per_page);
}

size_t db_search_albums(struct album **palbums, const char *query,
                        size_t page, size_t albums_per_page) {
    struct sqlite3_stmt *stmt;
    VEC(struct album) albums = {0};

    if (query == NULL) {
        stmt = statements[STATEMENT_GET_ALBUMS_WITH_PAGINATION].stmt;
    } else {
        stmt = statements[STATEMENT_SEARCH_ALBUMS_WITH_PAGINATION].stmt;
    }

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    STMT_BIND(stmt, int64, "$select_count", albums_per_page);
    STMT_BIND(stmt, int64, "$select_offset", page * albums_per_page);
    if (query != NULL) {
        STMT_BIND(stmt, text, "$query", query, -1, SQLITE_STATIC);
    }

    int ret;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        struct album *a = VEC_EMPLACE_BACK(&albums);

        a->id = xstrdup((char *)sqlite3_column_text(stmt, 0));
        a->name = xstrdup((char *)sqlite3_column_text(stmt, 1));
        a->artist = xstrdup((char *)sqlite3_column_text(stmt, 2));
        a->artist_id = xstrdup((char *)sqlite3_column_text(stmt, 3));
        a->song_count = sqlite3_column_int(stmt, 4);
        a->duration = sqlite3_column_int(stmt, 5);
    }
    if (ret != SQLITE_DONE) {
        ERROR("failed to fetch albums from db: %s", sqlite3_errmsg(db));
        VEC_FREE(&albums);
        *palbums = NULL;
        return 0;
    }

    *palbums = VEC_DATA(&albums);
    return VEC_SIZE(&albums);
}

size_t db_get_albums(struct album **palbums, size_t page, size_t albums_per_page) {
    return db_search_albums(palbums, NULL, page, albums_per_page);
}

size_t db_get_songs_in_album(struct song **psongs, const struct album *album) {
    struct sqlite3_stmt *const stmt = statements[STATEMENT_GET_SONGS_IN_ALBUM].stmt;
    VEC(struct song) songs = {0};

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    STMT_BIND(stmt, text, "$album_id", album->id, -1, SQLITE_STATIC);

    int ret;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        struct song *s = VEC_EMPLACE_BACK(&songs);

        s->id = xstrdup((char *)sqlite3_column_text(stmt, 0));
        s->title = xstrdup((char *)sqlite3_column_text(stmt, 1));
        s->artist = xstrdup((char *)sqlite3_column_text(stmt, 2));
        s->album = xstrdup((char *)sqlite3_column_text(stmt, 3));

        s->track = sqlite3_column_int(stmt, 4);
        s->year = sqlite3_column_int(stmt, 5);
        s->duration = sqlite3_column_int(stmt, 6);
        s->bitrate = sqlite3_column_int(stmt, 7);
        s->size = sqlite3_column_int(stmt, 8);

        s->filetype = xstrdup((char *)sqlite3_column_text(stmt, 9));
        s->artist_id = xstrdup((char *)sqlite3_column_text(stmt, 10));
        s->album_id = xstrdup((char *)sqlite3_column_text(stmt, 11));
    }
    if (ret != SQLITE_DONE) {
        ERROR("failed to fetch songs from db: %s", sqlite3_errmsg(db));
        VEC_FREE(&songs);
        *psongs = NULL;
        return 0;
    }

    *psongs = VEC_DATA(&songs);
    return VEC_SIZE(&songs);
}

size_t db_get_songs(struct song **psongs, size_t page, size_t songs_per_page) {
    struct sqlite3_stmt *const stmt = statements[STATEMENT_GET_SONGS_WITH_PAGINATION].stmt;
    VEC(struct song) songs = {0};

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    STMT_BIND(stmt, int64, "$select_count", songs_per_page);
    STMT_BIND(stmt, int64, "$select_offset", page * songs_per_page);

    int ret;
    while ((ret = sqlite3_step(stmt)) == SQLITE_ROW) {
        struct song *s = VEC_EMPLACE_BACK(&songs);

        s->id = xstrdup((char *)sqlite3_column_text(stmt, 0));
        s->title = xstrdup((char *)sqlite3_column_text(stmt, 1));
        s->artist = xstrdup((char *)sqlite3_column_text(stmt, 2));
        s->album = xstrdup((char *)sqlite3_column_text(stmt, 3));

        s->track = sqlite3_column_int(stmt, 4);
        s->year = sqlite3_column_int(stmt, 5);
        s->duration = sqlite3_column_int(stmt, 6);
        s->bitrate = sqlite3_column_int(stmt, 7);
        s->size = sqlite3_column_int(stmt, 8);

        s->filetype = xstrdup((char *)sqlite3_column_text(stmt, 9));
        s->artist_id = xstrdup((char *)sqlite3_column_text(stmt, 10));
        s->album_id = xstrdup((char *)sqlite3_column_text(stmt, 11));
    }
    if (ret != SQLITE_DONE) {
        ERROR("failed to fetch songs from db: %s", sqlite3_errmsg(db));
        VEC_FREE(&songs);
        *psongs = NULL;
        return 0;
    }

    *psongs = VEC_DATA(&songs);
    return VEC_SIZE(&songs);
}

