#include "db/query.h"
#include "db/internal.h"
#include "collections/array.h"
#include "xmalloc.h"
#include "log.h"

size_t db_search_albums(struct album **palbums, const char *query,
                        size_t page, size_t albums_per_page) {
    struct sqlite3_stmt *stmt;
    ARRAY(struct album) albums = {0};

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
        struct album *a = ARRAY_EMPLACE_BACK(&albums);

        a->id = xstrdup((char *)sqlite3_column_text(stmt, 0));
        a->name = xstrdup((char *)sqlite3_column_text(stmt, 1));
        a->artist = xstrdup((char *)sqlite3_column_text(stmt, 2));
        a->artist_id = xstrdup((char *)sqlite3_column_text(stmt, 3));
        a->song_count = sqlite3_column_int(stmt, 4);
        a->duration = sqlite3_column_int(stmt, 5);
    }
    if (ret != SQLITE_DONE) {
        ERROR("failed to fetch albums from db: %s", sqlite3_errmsg(db));
        ARRAY_FREE(&albums);
        *palbums = NULL;
        return 0;
    }

    *palbums = ARRAY_DATA(&albums);
    return ARRAY_SIZE(&albums);
}

size_t db_get_albums(struct album **palbums, size_t page, size_t albums_per_page) {
    return db_search_albums(palbums, NULL, page, albums_per_page);
}

