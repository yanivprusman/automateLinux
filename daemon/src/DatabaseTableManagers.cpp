#include "DatabaseTableManagers.h"
#include "Utils.h"
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <memory>
#include <tuple>

// Helper to check and get connection
static sql::Connection *getCon() {
  sql::Connection *con = MySQLManager::getConnection();
  if (!con) {
    logToFile("DatabaseTableManagers: Failed to get connection", 0xFFFFFFFF);
  }
  return con;
}

// TerminalTable Implementation

void TerminalTable::upsertHistory(int tty, int index, const std::string &path) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("INSERT INTO terminal_history (tty, entry_index, "
                              "path) VALUES (?, ?, ?) "
                              "ON DUPLICATE KEY UPDATE path = ?"));
    pstmt->setInt(1, tty);
    pstmt->setInt(2, index);
    pstmt->setString(3, path);
    pstmt->setString(4, path);
    pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("TerminalTable: upsertHistory error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
}

std::string TerminalTable::getHistory(int tty, int index) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return "";
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "SELECT path FROM terminal_history WHERE tty = ? AND entry_index = ?"));
    pstmt->setInt(1, tty);
    pstmt->setInt(2, index);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next())
      return res->getString("path");
  } catch (sql::SQLException &e) {
    logToFile("TerminalTable: getHistory error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
  return "";
}

void TerminalTable::setSessionPointer(int tty, int index) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "INSERT INTO terminal_sessions (tty, history_index) VALUES (?, ?) "
        "ON DUPLICATE KEY UPDATE history_index = ?"));
    pstmt->setInt(1, tty);
    pstmt->setInt(2, index);
    pstmt->setInt(3, index);
    pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("TerminalTable: setSessionPointer error: " +
                  std::string(e.what()),
              0xFFFFFFFF);
  }
}

int TerminalTable::getSessionPointer(int tty) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return -1;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "SELECT history_index FROM terminal_sessions WHERE tty = ?"));
    pstmt->setInt(1, tty);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next())
      return res->getInt("history_index");
  } catch (sql::SQLException &e) {
    logToFile("TerminalTable: getSessionPointer error: " +
                  std::string(e.what()),
              0xFFFFFFFF);
  }
  return -1;
}

void TerminalTable::deleteSession(int tty) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("DELETE FROM terminal_sessions WHERE tty = ?"));
    pstmt->setInt(1, tty);
    pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("TerminalTable: deleteSession error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
}

std::vector<std::pair<int, std::string>>
TerminalTable::getAllHistoryForTty(int tty) {
  std::vector<std::pair<int, std::string>> results;
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return results;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("SELECT entry_index, path FROM terminal_history "
                              "WHERE tty = ? ORDER BY entry_index"));
    pstmt->setInt(1, tty);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    while (res->next()) {
      results.push_back({res->getInt("entry_index"), res->getString("path")});
    }
  } catch (sql::SQLException &e) {
    logToFile("TerminalTable: getAllHistoryForTty error: " +
                  std::string(e.what()),
              0xFFFFFFFF);
  }
  return results;
}

std::vector<std::tuple<int, int, std::string>> TerminalTable::getAllHistory() {
  std::vector<std::tuple<int, int, std::string>> results;
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return results;
  try {
    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    std::unique_ptr<sql::ResultSet> res(
        stmt->executeQuery("SELECT tty, entry_index, path FROM "
                           "terminal_history ORDER BY tty, entry_index"));
    while (res->next()) {
      results.push_back(std::make_tuple(res->getInt("tty"),
                                        res->getInt("entry_index"),
                                        res->getString("path")));
    }
  } catch (sql::SQLException &e) {
    logToFile("TerminalTable: getAllHistory error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
  return results;
}

std::vector<std::pair<int, int>> TerminalTable::getAllSessions() {
  std::vector<std::pair<int, int>> results;
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return results;
  try {
    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    std::unique_ptr<sql::ResultSet> res(
        stmt->executeQuery("SELECT tty, history_index FROM terminal_sessions"));
    while (res->next()) {
      results.push_back({res->getInt("tty"), res->getInt("history_index")});
    }
  } catch (sql::SQLException &e) {
    logToFile("TerminalTable: getAllSessions error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
  return results;
}

int TerminalTable::getMaxHistoryIndex(int tty) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return -1;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "SELECT MAX(entry_index) as max_idx FROM terminal_history WHERE tty "
        "= ?"));
    pstmt->setInt(1, tty);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      return res->getInt("max_idx");
    }
  } catch (sql::SQLException &e) {
    logToFile("TerminalTable: getMaxHistoryIndex error: " +
                  std::string(e.what()),
              0xFFFFFFFF);
  }
  return -1;
}
void ConfigTable::setConfig(const std::string &key,
                            const std::string &jsonValue) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("INSERT INTO automation_configs (config_key, "
                              "config_value) VALUES (?, ?) "
                              "ON DUPLICATE KEY UPDATE config_value = ?"));
    pstmt->setString(1, key);
    pstmt->setString(2, jsonValue);
    pstmt->setString(3, jsonValue);
    pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("ConfigTable: setConfig error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
}

std::string ConfigTable::getConfig(const std::string &key) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return "";
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("SELECT config_value FROM automation_configs "
                              "WHERE config_key = ?"));
    pstmt->setString(1, key);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next())
      return res->getString("config_value");
  } catch (sql::SQLException &e) {
    logToFile("ConfigTable: getConfig error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
  return "";
}

// DeviceTable Implementation

void DeviceTable::setDevicePath(const std::string &type,
                                const std::string &path) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("INSERT INTO device_registry (device_type, path) "
                              "VALUES (?, ?) "
                              "ON DUPLICATE KEY UPDATE path = ?"));
    pstmt->setString(1, type);
    pstmt->setString(2, path);
    pstmt->setString(3, path);
    pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("DeviceTable: setDevicePath error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
}

std::string DeviceTable::getDevicePath(const std::string &type) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return "";
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("SELECT path FROM device_registry WHERE "
                              "device_type = ?"));
    pstmt->setString(1, type);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next())
      return res->getString("path");
  } catch (sql::SQLException &e) {
    logToFile("DeviceTable: getDevicePath error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
  return "";
}

// SettingsTable Implementation

void SettingsTable::setSetting(const std::string &key,
                               const std::string &value) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("INSERT INTO system_settings (setting_key, "
                              "setting_value) VALUES (?, ?) "
                              "ON DUPLICATE KEY UPDATE setting_value = ?"));
    pstmt->setString(1, key);
    pstmt->setString(2, value);
    pstmt->setString(3, value);
    pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("SettingsTable: setSetting error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
}

std::string SettingsTable::getSetting(const std::string &key) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return "";
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("SELECT setting_value FROM system_settings WHERE "
                              "setting_key = ?"));
    pstmt->setString(1, key);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next())
      return res->getString("setting_value");
  } catch (sql::SQLException &e) {
    logToFile("SettingsTable: getSetting error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
  return "";
}

int SettingsTable::deleteSetting(const std::string &key) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return 0;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("DELETE FROM system_settings "
                              "WHERE setting_key = ?"));
    pstmt->setString(1, key);
    return pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("SettingsTable: deleteSetting error: " + std::string(e.what()),
              0xFFFFFFFF);
    return 0;
  }
}
