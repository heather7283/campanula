#include <sqlite3.h>

#define STMT_BIND(stmt, type, name, ...) \
    sqlite3_bind_##type((stmt), sqlite3_bind_parameter_index((stmt), (name)), ##__VA_ARGS__)

struct sqlite_statement {
    const char *source;
    struct sqlite3_stmt *stmt;
};

enum sqlite_statement_type {
    STATEMENT_ENABLE_FOREIGN_KEYS,

    STATEMENT_CREATE_TABLE_ARTISTS,
    STATEMENT_CREATE_TABLE_ALBUMS,
    STATEMENT_CREATE_TABLE_SONGS,

    STATEMENT_BEGIN,
    STATEMENT_COMMIT,
    STATEMENT_ROLLBACK,

    STATEMENT_MARK_ARTISTS_AS_DELETED,
    STATEMENT_MARK_ALBUMS_AS_DELETED,
    STATEMENT_MARK_SONGS_AS_DELETED,
    STATEMENT_DELETE_DELETED_ARTISTS,
    STATEMENT_DELETE_DELETED_ALBUMS,
    STATEMENT_DELETE_DELETED_SONGS,

    STATEMENT_INSERT_ARTIST,
    STATEMENT_INSERT_ALBUM,
    STATEMENT_INSERT_SONG,

    SQLITE_STATEMENT_TYPE_COUNT
};

extern struct sqlite_statement statements[SQLITE_STATEMENT_TYPE_COUNT];

extern struct sqlite3 *db;

