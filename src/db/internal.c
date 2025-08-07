#include <assert.h>

#include <sqlite3.h>

#include "macros.h"
#include "config.h"
#include "log.h"

struct sqlite_statement {
    const char *source;
    struct sqlite3_stmt *stmt;
};

enum sqlite_statement_type {
    STATEMENT_CREATE_TABLE_ARTISTS,
    STATEMENT_CREATE_TABLE_ALBUMS,
    STATEMENT_CREATE_TABLE_SONGS,

    SQLITE_STATEMENT_TYPE_COUNT
};

static struct sqlite_statement statements[] = {
    [STATEMENT_CREATE_TABLE_ARTISTS] = { .source =
        "CREATE TABLE IF NOT EXISTS artists ("
        "    id TEXT NOT NULL PRIMARY KEY,"
        "    name TEXT NOT NULL"
        ")"
    },
    [STATEMENT_CREATE_TABLE_ALBUMS] = { .source =
        "CREATE TABLE IF NOT EXISTS albums ("
        "    id TEXT NOT NULL PRIMARY KEY,"
        "    title TEXT NOT NULL,"
        "    artist TEXT NOT NULL,"
        ""
        "    artist_id TEXT,"
        "    FOREIGN KEY(artist_id) REFERENCES artists(id)"
        ")"
    },
    [STATEMENT_CREATE_TABLE_SONGS] = { .source =
        "CREATE TABLE IF NOT EXISTS songs ("
        "    id TEXT NOT NULL PRIMARY KEY,"
        "    title TEXT NOT NULL,"
        "    artist TEXT NOT NULL,"
        "    album TEXT NOT NULL,"
        ""
        "    track INTEGER,"
        "    year INTEGER,"
        "    duration INTEGER,"
        "    bitrate INTEGER,"
        "    size INTEGER,"
        "    filetype TEXT,"
        ""
        "    artist_id TEXT,"
        "    album_id TEXT,"
        "    FOREIGN KEY(artist_id) REFERENCES artists(id),"
        "    FOREIGN KEY(album_id) REFERENCES albums(id)"
        ")"
    },
};
static_assert(SIZEOF_ARRAY(statements) == SQLITE_STATEMENT_TYPE_COUNT);

static struct sqlite3 *db = NULL;

bool db_init(void) {
    int ret = 0;

    ret = sqlite3_open(config.database_path, &db);
    if (ret != SQLITE_OK) {
        ERROR("failed to open db at %s: %s", config.database_path, sqlite3_errstr(ret));
        goto err;
    }

    ret = sqlite3_exec(db, statements[STATEMENT_CREATE_TABLE_ARTISTS].source, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        ERROR("failed to create artists table: %s", sqlite3_errmsg(db));
        goto err;
    }
    ret = sqlite3_exec(db, statements[STATEMENT_CREATE_TABLE_ALBUMS].source, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        ERROR("failed to create albums table: %s", sqlite3_errmsg(db));
        goto err;
    }
    ret = sqlite3_exec(db, statements[STATEMENT_CREATE_TABLE_SONGS].source, NULL, NULL, NULL);
    if (ret != SQLITE_OK) {
        ERROR("failed to create songs table: %s", sqlite3_errmsg(db));
        goto err;
    }

    return true;

err:
    sqlite3_close(db);
    return false;
}

void db_cleanup(void) {
    sqlite3_close(db);
    db = NULL;
}

