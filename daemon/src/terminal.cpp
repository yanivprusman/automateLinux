#include "terminal.h"
#include <filesystem>
#include <iostream>
#include <fstream>

Terminal& Terminal::getInstance() {
static Terminal instance;
return instance;
}

std::string Terminal::getBaseDir() {
std::string exePath = std::filesystem::canonical("/proc/self/exe").parent_path().parent_path().string();
return exePath + "/";
}

Terminal::Terminal() : db(nullptr) {
initializeDatabase();
}

Terminal::~Terminal() {
if (db) {
sqlite3_close(db);
}
}

void Terminal::initializeDatabase() {
std::string baseDir = getBaseDir();
std::string dataDir = baseDir + "data";
std::filesystem::create_directories(dataDir);
std::string dbPath = dataDir + "/daemon.db";
int rc = sqlite3_open(dbPath.c_str(), &db);
if (rc) {
std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
return;
}
sqlite3_busy_timeout(db, 5000);
char* errMsg = nullptr;
const char* pragmaWal = "PRAGMA journal_mode=WAL;";
rc = sqlite3_exec(db, pragmaWal, nullptr, nullptr, &errMsg);
if (rc != SQLITE_OK) {
std::cerr << "WAL pragma error: " << errMsg << std::endl;
sqlite3_free(errMsg);
}
const char* createTerminalsTable = "CREATE TABLE IF NOT EXISTS terminals("
"tty TEXT PRIMARY KEY,"
"last_changed INTEGER,"
"is_active INTEGER);";
rc = sqlite3_exec(db, createTerminalsTable, nullptr, nullptr, &errMsg);
if (rc != SQLITE_OK) {
std::cerr << "Create terminals table error: " << errMsg << std::endl;
sqlite3_free(errMsg);
}
const char* createHistoryTable = "CREATE TABLE IF NOT EXISTS history("
"tty TEXT,"
"idx INTEGER,"
"directory TEXT,"
"PRIMARY KEY(tty, idx));";
rc = sqlite3_exec(db, createHistoryTable, nullptr, nullptr, &errMsg);
if (rc != SQLITE_OK) {
std::cerr << "Create history table error: " << errMsg << std::endl;
sqlite3_free(errMsg);
}
}

std::string Terminal::getLastChangedTTY() {
    std::lock_guard<std::mutex> lock(mtx);
    sqlite3_stmt* stmt;
    const char* query = "SELECT tty FROM terminals ORDER BY last_changed DESC LIMIT 1;";
    int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return "";
    }
    std::string result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    sqlite3_finalize(stmt);
    return result;
}

std::string Terminal::getLastChangedTTYNoLock() {
sqlite3_stmt* stmt;
const char* query = "SELECT tty FROM terminals ORDER BY last_changed DESC LIMIT 1;";
int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
if (rc != SQLITE_OK) {
return "";
}
std::string result;
if (sqlite3_step(stmt) == SQLITE_ROW) {
result = std::string(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
}
sqlite3_finalize(stmt);
return result;
}

void Terminal::copyHistoryFromLastTTY(const std::string& currentTty) {
    std::lock_guard<std::mutex> lock(mtx);
    std::string lastTty = getLastChangedTTYNoLock();
    if (lastTty.empty() || lastTty == currentTty) {
        return;
    }
    sqlite3_stmt* stmt;
    const char* query = "SELECT idx, directory FROM history WHERE tty = ? ORDER BY idx;";
    int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return;
    }
    sqlite3_bind_text(stmt, 1, lastTty.c_str(), -1, SQLITE_STATIC);
    // char* errMsg = nullptr;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int idx = sqlite3_column_int(stmt, 0);
        const char* dir = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string insertQuery = "INSERT INTO history (tty, idx, directory) VALUES (?, ?, ?);";
        sqlite3_stmt* insertStmt;
        rc = sqlite3_prepare_v2(db, insertQuery.c_str(), -1, &insertStmt, nullptr);
        if (rc == SQLITE_OK) {
            sqlite3_bind_text(insertStmt, 1, currentTty.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(insertStmt, 2, idx);
            sqlite3_bind_text(insertStmt, 3, dir, -1, SQLITE_STATIC);
            sqlite3_step(insertStmt);
            sqlite3_finalize(insertStmt);
        }
    }
    sqlite3_finalize(stmt);
    sqlite3_stmt* pointerStmt;
    const char* pointerQuery = "SELECT idx FROM history WHERE tty = ? ORDER BY idx DESC LIMIT 1;";
    rc = sqlite3_prepare_v2(db, pointerQuery, -1, &pointerStmt, nullptr);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(pointerStmt, 1, lastTty.c_str(), -1, SQLITE_STATIC);
        int pointer = 1;
        if (sqlite3_step(pointerStmt) == SQLITE_ROW) {
            pointer = sqlite3_column_int(pointerStmt, 0);
        }
        std::string updatePointerQuery = "INSERT OR REPLACE INTO history (tty, idx, directory) "
        "SELECT ?, ?, directory FROM history WHERE tty = ? AND idx = ?;";
        sqlite3_finalize(pointerStmt);
    }
}

void Terminal::reloadCacheFromDB(const std::string& tty) {
std::lock_guard<std::mutex> lock(mtx);
cache[tty].history.clear();
cache[tty].pointer = 1;
sqlite3_stmt* stmt;
const char* query = "SELECT idx, directory FROM history WHERE tty = ? ORDER BY idx;";
int rc = sqlite3_prepare_v2(db, query, -1, &stmt, nullptr);
if (rc != SQLITE_OK) {
return;
}
sqlite3_bind_text(stmt, 1, tty.c_str(), -1, SQLITE_STATIC);
while (sqlite3_step(stmt) == SQLITE_ROW) {
const char* dir = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
cache[tty].history.push_back(std::string(dir));
}
sqlite3_finalize(stmt);
sqlite3_stmt* pointerStmt;
const char* pointerQuery = "SELECT MAX(idx) FROM history WHERE tty = ?;";
rc = sqlite3_prepare_v2(db, pointerQuery, -1, &pointerStmt, nullptr);
if (rc == SQLITE_OK) {
sqlite3_bind_text(pointerStmt, 1, tty.c_str(), -1, SQLITE_STATIC);
if (sqlite3_step(pointerStmt) == SQLITE_ROW) {
int maxIdx = sqlite3_column_int(pointerStmt, 0);
if (maxIdx > 0) {
cache[tty].pointer = maxIdx;
}
}
sqlite3_finalize(pointerStmt);
}
}

void Terminal::writeCacheToDB(const std::string& tty) {
std::lock_guard<std::mutex> lock(mtx);
if (cache.find(tty) == cache.end()) {
return;
}
sqlite3_exec(db, "DELETE FROM history WHERE tty = ?;", nullptr, nullptr, nullptr);
for (size_t i = 0; i < cache[tty].history.size(); ++i) {
std::string insertQuery = "INSERT INTO history (tty, idx, directory) VALUES (?, ?, ?);";
sqlite3_stmt* stmt;
int rc = sqlite3_prepare_v2(db, insertQuery.c_str(), -1, &stmt, nullptr);
if (rc == SQLITE_OK) {
sqlite3_bind_text(stmt, 1, tty.c_str(), -1, SQLITE_STATIC);
sqlite3_bind_int(stmt, 2, i + 1);
sqlite3_bind_text(stmt, 3, cache[tty].history[i].c_str(), -1, SQLITE_STATIC);
sqlite3_step(stmt);
sqlite3_finalize(stmt);
}
}
std::string updatePointerQuery = "INSERT OR REPLACE INTO terminals (tty, last_changed, is_active) "
"VALUES (?, ?, 1);";
sqlite3_stmt* stmt;
int rc = sqlite3_prepare_v2(db, updatePointerQuery.c_str(), -1, &stmt, nullptr);
if (rc == SQLITE_OK) {
sqlite3_bind_text(stmt, 1, tty.c_str(), -1, SQLITE_STATIC);
sqlite3_bind_int(stmt, 2, static_cast<int>(std::time(nullptr)));
sqlite3_step(stmt);
sqlite3_finalize(stmt);
}
}

std::string Terminal::ttyOpened(const std::string& tty) {
    std::lock_guard<std::mutex> lock(mtx);
    std::string lastTty = getLastChangedTTY();
    if (!lastTty.empty() && lastTty != tty) {
    copyHistoryFromLastTTY(tty);
    } else if (lastTty.empty()) {
    cache[tty].history.clear();
    cache[tty].history.push_back("/home/yaniv/coding/");
    cache[tty].pointer = 1;
    writeCacheToDB(tty);
    }
    reloadCacheFromDB(tty);
    std::string updateQuery = "INSERT OR REPLACE INTO terminals (tty, last_changed, is_active) "
    "VALUES (?, ?, 1);";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(db, updateQuery.c_str(), -1, &stmt, nullptr);
    if (rc == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, tty.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, static_cast<int>(std::time(nullptr)));
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    }
    return "OK";
}

std::string Terminal::cdToPointer(const std::string& tty) {
std::lock_guard<std::mutex> lock(mtx);
if (cache.find(tty) == cache.end()) {
return "";
}
int pointer = cache[tty].pointer;
if (pointer < 1 || pointer > static_cast<int>(cache[tty].history.size())) {
return "";
}
return "cd " + cache[tty].history[pointer - 1];
}

std::string Terminal::updateDirHistory(const std::string& tty, const std::string& pwd) {
std::lock_guard<std::mutex> lock(mtx);
if (cache.find(tty) == cache.end()) {
reloadCacheFromDB(tty);
}
int pointer = cache[tty].pointer;
std::string currentDir;
if (pointer >= 1 && pointer <= static_cast<int>(cache[tty].history.size())) {
currentDir = cache[tty].history[pointer - 1];
}
std::string pwdWithSlash = pwd;
if (!pwdWithSlash.empty() && pwdWithSlash.back() != '/') {
pwdWithSlash += "/";
}
if (pwdWithSlash != currentDir) {
cache[tty].history.insert(cache[tty].history.begin() + pointer, pwdWithSlash);
cache[tty].pointer++;
}
writeCacheToDB(tty);
return "OK";
}

std::string Terminal::cdBackword(const std::string& tty) {
std::lock_guard<std::mutex> lock(mtx);
if (cache.find(tty) == cache.end()) {
reloadCacheFromDB(tty);
}
cache[tty].pointer--;
if (cache[tty].pointer < 1) {
cache[tty].pointer = 1;
}
writeCacheToDB(tty);
return cdToPointer(tty);
}

std::string Terminal::cdForword(const std::string& tty) {
std::lock_guard<std::mutex> lock(mtx);
if (cache.find(tty) == cache.end()) {
reloadCacheFromDB(tty);
}
int historySize = static_cast<int>(cache[tty].history.size());
cache[tty].pointer++;
if (cache[tty].pointer > historySize) {
cache[tty].pointer = historySize;
}
writeCacheToDB(tty);
return cdToPointer(tty);
}

std::string Terminal::ttyClosed(const std::string& tty) {
std::lock_guard<std::mutex> lock(mtx);
std::string updateQuery = "UPDATE terminals SET is_active = 0 WHERE tty = ?;";
sqlite3_stmt* stmt;
int rc = sqlite3_prepare_v2(db, updateQuery.c_str(), -1, &stmt, nullptr);
if (rc == SQLITE_OK) {
sqlite3_bind_text(stmt, 1, tty.c_str(), -1, SQLITE_STATIC);
sqlite3_step(stmt);
sqlite3_finalize(stmt);
}
cache.erase(tty);
return "OK";
}
