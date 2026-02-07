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
                           bool is_online, int daemon_version) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "INSERT INTO peer_registry (peer_id, ip_address, mac_address, hostname, "
        "is_online, last_seen, daemon_version) VALUES (?, ?, ?, ?, ?, NOW(), ?) "
        "ON DUPLICATE KEY UPDATE ip_address = ?, mac_address = ?, hostname = ?, "
        "is_online = ?, last_seen = NOW(), daemon_version = ?"));
    pstmt->setString(1, peer_id);
    pstmt->setString(2, ip);
    pstmt->setString(3, mac);
    pstmt->setString(4, hostname);
    pstmt->setBoolean(5, is_online);
    pstmt->setInt(6, daemon_version);
    pstmt->setString(7, ip);
    pstmt->setString(8, mac);
    pstmt->setString(9, hostname);
    pstmt->setBoolean(10, is_online);
    pstmt->setInt(11, daemon_version);
    pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("PeerTable: upsertPeer error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
}

PeerRecord PeerTable::getPeer(const std::string &peer_id) {
  PeerRecord result;
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return result;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "SELECT peer_id, ip_address, mac_address, hostname, last_seen, is_online, "
        "daemon_version FROM peer_registry WHERE peer_id = ?"));
    pstmt->setString(1, peer_id);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      result.peer_id = res->getString("peer_id");
      result.ip_address = res->getString("ip_address");
      result.mac_address = res->getString("mac_address");
      result.hostname = res->getString("hostname");
      result.last_seen = res->getString("last_seen");
      result.is_online = res->getBoolean("is_online");
      result.daemon_version = res->getInt("daemon_version");
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
        "SELECT peer_id, ip_address, mac_address, hostname, last_seen, is_online, "
        "daemon_version FROM peer_registry ORDER BY peer_id"));
    while (res->next()) {
      PeerRecord record;
      record.peer_id = res->getString("peer_id");
      record.ip_address = res->getString("ip_address");
      record.mac_address = res->getString("mac_address");
      record.hostname = res->getString("hostname");
      record.last_seen = res->getString("last_seen");
      record.is_online = res->getBoolean("is_online");
      record.daemon_version = res->getInt("daemon_version");
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

void PeerTable::touchLastSeen(const std::string &peer_id) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "UPDATE peer_registry SET last_seen = NOW(), is_online = 1 WHERE peer_id = ?"));
    pstmt->setString(1, peer_id);
    pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("PeerTable: touchLastSeen error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
}

void PeerTable::touchLastSeen(const std::string &peer_id, int daemon_version) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "UPDATE peer_registry SET last_seen = NOW(), is_online = 1, "
        "daemon_version = ? WHERE peer_id = ?"));
    pstmt->setInt(1, daemon_version);
    pstmt->setString(2, peer_id);
    pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("PeerTable: touchLastSeen error: " + std::string(e.what()),
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

int PeerTable::clearAllPeers() {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return 0;
  try {
    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    return stmt->executeUpdate("DELETE FROM peer_registry");
  } catch (sql::SQLException &e) {
    logToFile("PeerTable: clearAllPeers error: " + std::string(e.what()),
              0xFFFFFFFF);
    return 0;
  }
}

// ExtraAppTable Implementation

void ExtraAppTable::upsertApp(const ExtraAppRecord &app) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "INSERT INTO extra_apps (app_id, display_name, repo_url, "
        "has_server_component, server_service_template, client_service_template, "
        "port_key_client, port_key_server, dev_path, prod_path, "
        "server_build_subdir, client_subdir) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON DUPLICATE KEY UPDATE display_name = ?, repo_url = ?, "
        "has_server_component = ?, server_service_template = ?, "
        "client_service_template = ?, port_key_client = ?, port_key_server = ?, "
        "dev_path = ?, prod_path = ?, server_build_subdir = ?, client_subdir = ?"));
    // Insert values
    pstmt->setString(1, app.app_id);
    pstmt->setString(2, app.display_name);
    pstmt->setString(3, app.repo_url);
    pstmt->setBoolean(4, app.has_server_component);
    pstmt->setString(5, app.server_service_template);
    pstmt->setString(6, app.client_service_template);
    pstmt->setString(7, app.port_key_client);
    pstmt->setString(8, app.port_key_server);
    pstmt->setString(9, app.dev_path);
    pstmt->setString(10, app.prod_path);
    pstmt->setString(11, app.server_build_subdir);
    pstmt->setString(12, app.client_subdir);
    // Update values
    pstmt->setString(13, app.display_name);
    pstmt->setString(14, app.repo_url);
    pstmt->setBoolean(15, app.has_server_component);
    pstmt->setString(16, app.server_service_template);
    pstmt->setString(17, app.client_service_template);
    pstmt->setString(18, app.port_key_client);
    pstmt->setString(19, app.port_key_server);
    pstmt->setString(20, app.dev_path);
    pstmt->setString(21, app.prod_path);
    pstmt->setString(22, app.server_build_subdir);
    pstmt->setString(23, app.client_subdir);
    pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("ExtraAppTable: upsertApp error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
}

ExtraAppRecord ExtraAppTable::getApp(const std::string &app_id) {
  ExtraAppRecord result{"", "", "", false, "", "", "", "", "", "", "", ""};
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return result;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "SELECT app_id, display_name, repo_url, has_server_component, "
        "server_service_template, client_service_template, port_key_client, "
        "port_key_server, dev_path, prod_path, server_build_subdir, client_subdir "
        "FROM extra_apps WHERE app_id = ?"));
    pstmt->setString(1, app_id);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      auto safeStr = [&](const char *col) -> std::string {
        return res->isNull(col) ? "" : std::string(res->getString(col));
      };
      result.app_id = safeStr("app_id");
      result.display_name = safeStr("display_name");
      result.repo_url = safeStr("repo_url");
      result.has_server_component = res->getBoolean("has_server_component");
      result.server_service_template = safeStr("server_service_template");
      result.client_service_template = safeStr("client_service_template");
      result.port_key_client = safeStr("port_key_client");
      result.port_key_server = safeStr("port_key_server");
      result.dev_path = safeStr("dev_path");
      result.prod_path = safeStr("prod_path");
      result.server_build_subdir = safeStr("server_build_subdir");
      result.client_subdir = safeStr("client_subdir");
    }
  } catch (sql::SQLException &e) {
    logToFile("ExtraAppTable: getApp error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
  return result;
}

std::vector<ExtraAppRecord> ExtraAppTable::getAllApps() {
  std::vector<ExtraAppRecord> results;
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return results;
  try {
    std::unique_ptr<sql::Statement> stmt(con->createStatement());
    std::unique_ptr<sql::ResultSet> res(stmt->executeQuery(
        "SELECT app_id, display_name, repo_url, has_server_component, "
        "server_service_template, client_service_template, port_key_client, "
        "port_key_server, dev_path, prod_path, server_build_subdir, client_subdir "
        "FROM extra_apps ORDER BY app_id"));
    while (res->next()) {
      auto safeStr = [&](const char *col) -> std::string {
        return res->isNull(col) ? "" : std::string(res->getString(col));
      };
      ExtraAppRecord record;
      record.app_id = safeStr("app_id");
      record.display_name = safeStr("display_name");
      record.repo_url = safeStr("repo_url");
      record.has_server_component = res->getBoolean("has_server_component");
      record.server_service_template = safeStr("server_service_template");
      record.client_service_template = safeStr("client_service_template");
      record.port_key_client = safeStr("port_key_client");
      record.port_key_server = safeStr("port_key_server");
      record.dev_path = safeStr("dev_path");
      record.prod_path = safeStr("prod_path");
      record.server_build_subdir = safeStr("server_build_subdir");
      record.client_subdir = safeStr("client_subdir");
      results.push_back(record);
    }
  } catch (sql::SQLException &e) {
    logToFile("ExtraAppTable: getAllApps error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
  return results;
}

void ExtraAppTable::deleteApp(const std::string &app_id) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(
        con->prepareStatement("DELETE FROM extra_apps WHERE app_id = ?"));
    pstmt->setString(1, app_id);
    pstmt->executeUpdate();
  } catch (sql::SQLException &e) {
    logToFile("ExtraAppTable: deleteApp error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
}

bool ExtraAppTable::appExists(const std::string &app_id) {
  std::unique_ptr<sql::Connection> con(getCon());
  if (!con)
    return false;
  try {
    std::unique_ptr<sql::PreparedStatement> pstmt(con->prepareStatement(
        "SELECT 1 FROM extra_apps WHERE app_id = ?"));
    pstmt->setString(1, app_id);
    std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    return res->next();
  } catch (sql::SQLException &e) {
    logToFile("ExtraAppTable: appExists error: " + std::string(e.what()),
              0xFFFFFFFF);
  }
  return false;
}
