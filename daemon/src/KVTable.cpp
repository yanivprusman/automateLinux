#include "KVTable.h"
#include <vector>
#include <algorithm>

sqlite3* KVTable::db = nullptr; 

KVTable::KVTable() {
    if (db == nullptr) {
        int rc = create();
        if(rc != SQLITE_OK) {
            cerr << "Failed to create KVTable DB, SQLite error code: " << rc << endl;
        }
    }
}

int KVTable::createDB() {
    std::filesystem::create_directories(directories.data); 
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

string KVTable::get(const string& key) {
    if (db == nullptr) {
        int rc = create();
        if (rc != SQLITE_OK) {
            return "";
        }
    }
    const char* sql = "SELECT value FROM kv WHERE key = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return "";
    }
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    string value;
    if (rc == SQLITE_ROW) {
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        if (text) {
            value = reinterpret_cast<const char*>(text);
        }
    }
    sqlite3_finalize(stmt);
    return value;
}

int KVTable::insertAt(const string& keyPrefix, int index, const string& value) {
    // First, shift all keys with this prefix that have indices >= insertion point
    // Do this BEFORE inserting the new entry to avoid conflicts
    const string likePattern = keyPrefix + "%";
    const char* sql = "SELECT key FROM kv WHERE key LIKE ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return rc;
    
    sqlite3_bind_text(stmt, 1, likePattern.c_str(), -1, SQLITE_TRANSIENT);
    
    std::vector<std::pair<int, string>> keysToShift; // {oldIndex, key}
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* key_str = (const char*)sqlite3_column_text(stmt, 0);
        string key(key_str);
        
        // Extract index from key (everything after keyPrefix)
        if (key.substr(0, keyPrefix.length()) == keyPrefix) {
            try {
                int keyIndex = stoi(key.substr(keyPrefix.length()));
                if (keyIndex >= index) {
                    keysToShift.push_back({keyIndex, key});
                }
            } catch (...) {
                // Skip keys that don't end with a valid number
            }
        }
    }
    sqlite3_finalize(stmt);
    
    // Shift keys in reverse order to avoid conflicts
    std::sort(keysToShift.rbegin(), keysToShift.rend());
    
    for (const auto& p : keysToShift) {
        int oldIndex = p.first;
        string oldKey = p.second;
        string newKeyShifted = keyPrefix + to_string(oldIndex + 1);
        string oldValue = get(oldKey);
        
        // Delete old key
        string deleteSQL = "DELETE FROM kv WHERE key = ?";
        sqlite3_stmt* deleteStmt;
        rc = sqlite3_prepare_v2(db, deleteSQL.c_str(), -1, &deleteStmt, nullptr);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(deleteStmt, 1, oldKey.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_step(deleteStmt);
            sqlite3_finalize(deleteStmt);
        }
        
        // Insert with new index
        upsert(newKeyShifted, oldValue);
    }
    
    // Finally, insert the new entry at the given index
    string newKey = keyPrefix + to_string(index);
    rc = upsert(newKey, value);
    
    return rc;
}

std::vector<std::pair<string, string>> KVTable::getByPrefix(const string& prefix) {
    std::vector<std::pair<string, string>> result;
    const string likePattern = prefix + "%";
    const char* sql = "SELECT key, value FROM kv WHERE key LIKE ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return result;
    sqlite3_bind_text(stmt, 1, likePattern.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* key_text = sqlite3_column_text(stmt, 0);
        const unsigned char* value_text = sqlite3_column_text(stmt, 1);
        string key = key_text ? reinterpret_cast<const char*>(key_text) : "";
        string value = value_text ? reinterpret_cast<const char*>(value_text) : "";
        result.push_back({key, value});
    }
    sqlite3_finalize(stmt);
    return result;
}

int KVTable::deleteByPrefix(const string& prefix) {
    const string likePattern = prefix + "%";
    const char* sql = "DELETE FROM kv WHERE key LIKE ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return rc;
    
    sqlite3_bind_text(stmt, 1, likePattern.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return (rc == SQLITE_DONE) ? SQLITE_OK : rc;
}

int KVTable::deleteEntry(const string& key) {
    const char* sql = "DELETE FROM kv WHERE key = ?";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return rc;
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_TRANSIENT);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return rc;
    int changes = sqlite3_changes(db);
    if (changes == 0) {
        return SQLITE_ERROR; 
    }
    return SQLITE_OK;  
}

std::vector<std::pair<string, string>> KVTable::getAll() {
    std::vector<std::pair<string, string>> result;
    if (db == nullptr) {
        int rc = create();
        if (rc != SQLITE_OK) {
            return result;
        }
    }
    const char* sql = "SELECT key, value FROM kv";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return result;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* key_text = sqlite3_column_text(stmt, 0);
        const unsigned char* value_text = sqlite3_column_text(stmt, 1);
        string key = key_text ? reinterpret_cast<const char*>(key_text) : "";
        string value = value_text ? reinterpret_cast<const char*>(value_text) : "";
        result.push_back({key, value});
    }
    sqlite3_finalize(stmt);
    return result;
}