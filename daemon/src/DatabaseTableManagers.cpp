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

void TerminalTable::upsertHistory(int index, const std::string &path) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("INSERT INTO terminal_history (entry_index, "
                              "path) VALUES (?, ?) "
                              "ON DUPLICATE KEY UPDATE path = ?"));
    pstmt->setInt(1, index);
    pstmt->setString(2, path);
    pstmt->setString(3, path);
    pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("TerminalTable: upsertHistory error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
}

std::string TerminalTable::getHistory(int index) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return "";
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "SELECT path FROM terminal_history WHERE entry_index = ?"));
    pstmt->setInt(1, index);
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

std::vector<std::pair<int, std::string>> TerminalTable::getAllHistoryEntries() {
  std::vector<std::pair<int, std::string>> results;
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return results;
  try {
    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    std::unique_ptr<sql::ResultSet> res(
        stmt->executeQuery("SELECT entry_index, path FROM terminal_history "
                           "ORDER BY entry_index"));
    while (res->next()) {
      results.push_back({res->getInt("entry_index"), res->getString("path")});
    }
  } catch (sql::SQLException &e) {
    logToFile("TerminalTable: getAllHistoryEntries error: " +
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
    // Joining with sessions to show current state for each TTY
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
        "SELECT s.tty, h.entry_index, h.path FROM "
        "terminal_history h JOIN terminal_sessions s ON "
        "h.entry_index = s.history_index ORDER BY s.tty, h.entry_index"));
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

int TerminalTable::getMaxHistoryIndex() {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return -1;
  try {
    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
        "SELECT MAX(entry_index) as max_idx FROM terminal_history"));
    if (res->next()) {
      if (res->isNull("max_idx")) {
        return -1;
      }
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
std::vector<std::pair<std::string, std::string>>
SettingsTable::getAllSettings() {
  std::vector<std::pair<std::string, std::string>> results;
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return results;
  try {
    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
        "SELECT setting_key, setting_value FROM system_settings"));
    while (res->next()) {
      results.push_back(
          {res->getString("setting_key"), res->getString("setting_value")});
    }
  } catch (sql::SQLException &e) {
    logToFile("SettingsTable: getAllSettings error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
  return results;
}

// PeerTable Implementation

void PeerTable::upsertPeer(const std::string &peer_id, const std::string &ip,
                           const std::string &mac, const std::string &hostname,
                           bool is_online) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "INSERT INTO peer_registry (peer_id, ip_address, mac_address, hostname, "
        "is_online, last_seen) VALUES (?, ?, ?, ?, ?, NOW()) "
        "ON DUPLICATE KEY UPDATE ip_address = ?, mac_address = ?, hostname = ?, "
        "is_online = ?, last_seen = NOW()"));
    pstmt->setString(1, peer_id);
    pstmt->setString(2, ip);
    pstmt->setString(3, mac);
    pstmt->setString(4, hostname);
    pstmt->setBoolean(5, is_online);
    pstmt->setString(6, ip);
    pstmt->setString(7, mac);
    pstmt->setString(8, hostname);
    pstmt->setBoolean(9, is_online);
    pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("PeerTable: upsertPeer error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
}

PeerRecord PeerTable::getPeer(const std::string &peer_id) {
  PeerRecord result{"", "", "", "", "", false};
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return result;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "SELECT peer_id, ip_address, mac_address, hostname, last_seen, is_online "
        "FROM peer_registry WHERE peer_id = ?"));
    pstmt->setString(1, peer_id);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      result.peer_id = res->getString("peer_id");
      result.ip_address = res->getString("ip_address");
      result.mac_address = res->getString("mac_address");
      result.hostname = res->getString("hostname");
      result.last_seen = res->getString("last_seen");
      result.is_online = res->getBoolean("is_online");
    }
  } catch (sql::SQLException &e) {
    logToFile("PeerTable: getPeer error: " + std::string(e.what()), 0xFFFFFFFF);
  }
  return result;
}

std::vector<PeerRecord> PeerTable::getAllPeers() {
  std::vector<PeerRecord> results;
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return results;
  try {
    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
        "SELECT peer_id, ip_address, mac_address, hostname, last_seen, is_online "
        "FROM peer_registry ORDER BY peer_id"));
    while (res->next()) {
      PeerRecord record;
      record.peer_id = res->getString("peer_id");
      record.ip_address = res->getString("ip_address");
      record.mac_address = res->getString("mac_address");
      record.hostname = res->getString("hostname");
      record.last_seen = res->getString("last_seen");
      record.is_online = res->getBoolean("is_online");
      results.push_back(record);
    }
  } catch (sql::SQLException &e) {
    logToFile("PeerTable: getAllPeers error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
  return results;
}

void PeerTable::updateOnlineStatus(const std::string &peer_id, bool is_online) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "UPDATE peer_registry SET is_online = ?, last_seen = NOW() "
        "WHERE peer_id = ?"));
    pstmt->setBoolean(1, is_online);
    pstmt->setString(2, peer_id);
    pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("PeerTable: updateOnlineStatus error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
}

void PeerTable::deletePeer(const std::string &peer_id) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("DELETE FROM peer_registry WHERE peer_id = ?"));
    pstmt->setString(1, peer_id);
    pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("PeerTable: deletePeer error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
}

std::string PeerTable::getIpAddress(const std::string &peer_id) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return "";
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "SELECT ip_address FROM peer_registry WHERE peer_id = ?"));
    pstmt->setString(1, peer_id);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next())
      return res->getString("ip_address");
  } catch (sql::SQLException &e) {
    logToFile("PeerTable: getIpAddress error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
  return "";
}
