#include "KVTable.h"

KVTable::KVTable() {
    int rc = create();
    if(rc != SQLITE_OK) {
        cerr << "Failed to create KVTable DB, SQLite error code: " << rc << endl;
    }
}

int KVTable::createDB() {
    int rc = sqlite3_open((directories.data + "daemon.db").c_str(), &db);
    if (rc != SQLITE_OK) {
        return rc;
    }
    return SQLITE_OK;
}

int KVTable::create() {
    createDB();
    const char* sql =
        "CREATE TABLE IF NOT EXISTS kv ("
        " key TEXT PRIMARY KEY,"
        " value TEXT);";
    return sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
}

int KVTable::upsert(const string& key, const string& value) {
    const char* sql =
        "INSERT INTO kv(key, value) "
        "VALUES(?, ?) "
        "ON CONFLICT(key) DO UPDATE SET "
        " value = excluded.value ";

    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return rc;

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, value.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? SQLITE_OK : rc;
}

int KVTable::countKeysByPrefix(const string& prefix) {
    if (db == nullptr) {
        int rc = create();
        if (rc != SQLITE_OK) {
            return -1;
        }
    }
    const char* sql = "SELECT COUNT(*) FROM kv WHERE key LIKE ? || '%'";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, prefix.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    int count = 0;
    if (rc == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

