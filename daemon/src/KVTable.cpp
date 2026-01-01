#include "KVTable.h"
#include "Globals.h"
#include "Utils.h"
#include <iostream>

sql::Connection *KVTable::con = nullptr;

KVTable::KVTable() { createDB(); }

KVTable::~KVTable() {
  if (con) {
    delete con;
    con = nullptr;
  }
}

void KVTable::checkConnection() {
  if (!con || con->isClosed()) {
    if (con)
      delete con;
    con = MySQLManager::getConnection();
  }
}

int KVTable::createDB() {
  try {
    checkConnection();
    if (!con)
      return 0;

    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    stmt->execute(
        "CREATE TABLE IF NOT EXISTS kv (k VARCHAR(255) PRIMARY KEY, v TEXT)");
    return 1;
  } catch (sql::SQLException &e) {
    logToFile("KVTable: Error creating table: " + std::string(e.what()),
              0xFFFFFFFF);
    return 0;
  }
}

int KVTable::upsert(const std::string &key, const std::string &value) {
  try {
    checkConnection();
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "INSERT INTO kv (k, v) VALUES (?, ?) ON DUPLICATE KEY UPDATE v = ?"));
    pstmt->setString(1, key);
    pstmt->setString(2, value);
    pstmt->setString(3, value);
    pstmt->executeUpdate();
    return 1;
  } catch (sql::SQLException &e) {
    logToFile("KVTable: Error upserting: " + std::string(e.what()), 0xFFFFFFFF);
    return 0;
  }
}

std::string KVTable::get(const std::string &key) {
  try {
    checkConnection();
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("SELECT v FROM kv WHERE k = ?"));
    pstmt->setString(1, key);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      return res->getString("v");
    }
    return "";
  } catch (sql::SQLException &e) {
    logToFile("KVTable: Error getting value: " + std::string(e.what()),
              0xFFFFFFFF);
    return "";
  }
}

int KVTable::deleteEntry(const std::string &key) {
  try {
    checkConnection();
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("DELETE FROM kv WHERE k = ?"));
    pstmt->setString(1, key);
    pstmt->executeUpdate();
    return 1;
  } catch (sql::SQLException &e) {
    logToFile("KVTable: Error deleting entry: " + std::string(e.what()),
              0xFFFFFFFF);
    return 0;
  }
}

int KVTable::deleteByPrefix(const std::string &prefix) {
  try {
    checkConnection();
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("DELETE FROM kv WHERE k LIKE ?"));
    pstmt->setString(1, prefix + "%");
    pstmt->executeUpdate();
    return 1;
  } catch (sql::SQLException &e) {
    logToFile("KVTable: Error deleting by prefix: " + std::string(e.what()),
              0xFFFFFFFF);
    return 0;
  }
}

int KVTable::countKeysByPrefix(const std::string &prefix) {
  try {
    checkConnection();
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("SELECT COUNT(*) FROM kv WHERE k LIKE ?"));
    pstmt->setString(1, prefix + "%");
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      return res->getInt(1);
    }
    return 0;
  } catch (sql::SQLException &e) {
    logToFile("KVTable: Error counting keys: " + std::string(e.what()),
              0xFFFFFFFF);
    return 0;
  }
}

std::vector<std::pair<std::string, std::string>>
KVTable::getByPrefix(const std::string &prefix) {
  std::vector<std::pair<std::string, std::string>> results;
  try {
    checkConnection();
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("SELECT k, v FROM kv WHERE k LIKE ?"));
    pstmt->setString(1, prefix + "%");
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    while (res->next()) {
      results.push_back({res->getString("k"), res->getString("v")});
    }
  } catch (sql::SQLException &e) {
    logToFile("KVTable: Error getting by prefix: " + std::string(e.what()),
              0xFFFFFFFF);
  }
  return results;
}

std::vector<std::pair<std::string, std::string>> KVTable::getAll() {
  std::vector<std::pair<std::string, std::string>> results;
  try {
    checkConnection();
    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    std::unique_ptr<sql::ResultSet> res(
        stmt->executeQuery("SELECT k, v FROM kv"));
    while (res->next()) {
      results.push_back({res->getString("k"), res->getString("v")});
    }
  } catch (sql::SQLException &e) {
    logToFile("KVTable: Error getting all: " + std::string(e.what()),
              0xFFFFFFFF);
  }
  return results;
}

int KVTable::insertAt(const std::string &keyPrefix, int index,
                      const std::string &value) {
  return upsert(keyPrefix + std::to_string(index), value);
}