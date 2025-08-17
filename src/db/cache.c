#include "db/cache.h"
#include "db/internal.h"
#include "xmalloc.h"
#include "log.h"

bool db_get_cached_song(struct cached_song *song, const char *id) {
    struct sqlite3_stmt *const stmt = statements[STATEMENT_GET_CACHED_SONG].stmt;

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    STMT_BIND(stmt, text, "$id", id, -1, SQLITE_STATIC);

    int ret = sqlite3_step(stmt);
    if (ret != SQLITE_ROW) {
        WARN("failed to retreive cached song for id %s: %s", id, sqlite3_errmsg(db));
        return false;
    }

    song->id = xstrdup((char *)sqlite3_column_text(stmt, 0));
    song->filename = xstrdup((char *)sqlite3_column_text(stmt, 1));
    song->filetype = xstrdup((char *)sqlite3_column_text(stmt, 2));
    song->bitrate = sqlite3_column_int64(stmt, 3);
    song->size = sqlite3_column_int64(stmt, 4);

    return true;
}

bool db_delete_cached_song(const char *id) {
    struct sqlite3_stmt *const stmt = statements[STATEMENT_DELETE_CACHED_SONG].stmt;

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    STMT_BIND(stmt, text, "$id", id, -1, SQLITE_STATIC);

    int ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        WARN("failed to delete cached song for song id %s: %s", id, sqlite3_errmsg(db));
        return false;
    }

    return true;
}

bool db_add_cached_song(const struct cached_song *song) {
    struct sqlite3_stmt *const stmt = statements[STATEMENT_ADD_CACHED_SONG].stmt;

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    STMT_BIND(stmt, text, "$id", song->id, -1, SQLITE_STATIC);
    STMT_BIND(stmt, text, "$filename", song->filename, -1, SQLITE_STATIC);
    STMT_BIND(stmt, text, "$filetype", song->filetype, -1, SQLITE_STATIC);
    STMT_BIND(stmt, int64, "$bitrate", song->bitrate);
    STMT_BIND(stmt, int64, "$size", song->size);

    int ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        WARN("failed to add cached song for id %s: %s", song->id, sqlite3_errmsg(db));
        return false;
    }

    return true;
}

bool db_touch_cached_song(const char *song_id) {
    struct sqlite3_stmt *const stmt = statements[STATEMENT_TOUCH_CACHED_SONG].stmt;

    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);

    STMT_BIND(stmt, text, "$id", song_id, -1, SQLITE_STATIC);

    int ret = sqlite3_step(stmt);
    if (ret != SQLITE_DONE) {
        WARN("failed to touch cached song for id %s: %s", song_id, sqlite3_errmsg(db));
        return false;
    }

    return true;
}

