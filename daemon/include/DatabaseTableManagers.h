#ifndef DATABASE_TABLE_MANAGERS_H
#define DATABASE_TABLE_MANAGERS_H

#include "MySQLManager.h"
#include <map>
#include <string>
#include <vector>

class TerminalTable {
public:
  static void upsertHistory(int index, const std::string &path);
  static std::string getHistory(int index);
  static void setSessionPointer(int tty, int index);
  static int getSessionPointer(int tty);
  static void deleteSession(int tty);
  static std::vector<std::pair<int, std::string>> getAllHistoryEntries();
  static std::vector<std::tuple<int, int, std::string>>
  getAllHistory(); // For showDB - session-history join
  static std::vector<std::pair<int, int>> getAllSessions();
  static int getMaxHistoryIndex();
};

class ConfigTable {
public:
  // For macros and filters
  static void setConfig(const std::string &key, const std::string &jsonValue);
  static std::string getConfig(const std::string &key);
};

class DeviceTable {
public:
  static void setDevicePath(const std::string &type, const std::string &path);
  static std::string getDevicePath(const std::string &type);
};

class SettingsTable {
public:
  static void setSetting(const std::string &key, const std::string &value);
  static std::string getSetting(const std::string &key);
  static int deleteSetting(const std::string &key);
  static std::vector<std::pair<std::string, std::string>> getAllSettings();
};

// Peer record structure for distributed daemon communication
struct PeerRecord {
  std::string peer_id;
  std::string ip_address;
  std::string mac_address;
  std::string hostname;
  std::string last_seen;
  bool is_online;
};

class PeerTable {
public:
  static void upsertPeer(const std::string &peer_id, const std::string &ip,
                         const std::string &mac, const std::string &hostname,
                         bool is_online);
  static PeerRecord getPeer(const std::string &peer_id);
  static std::vector<PeerRecord> getAllPeers();
  static void updateOnlineStatus(const std::string &peer_id, bool is_online);
  static void deletePeer(const std::string &peer_id);
  static std::string getIpAddress(const std::string &peer_id);
  static int clearAllPeers();  // Returns count of deleted rows
};

// Extra app record structure for installed applications
struct ExtraAppRecord {
  std::string app_id;
  std::string display_name;
  std::string repo_url;
  bool has_server_component;
  std::string server_service_template;
  std::string client_service_template;
  std::string port_key_client;
  std::string port_key_server;
  std::string dev_path;
  std::string prod_path;
  std::string server_build_subdir;
  std::string client_subdir;
};

class ExtraAppTable {
public:
  static void upsertApp(const ExtraAppRecord &app);
  static ExtraAppRecord getApp(const std::string &app_id);
  static std::vector<ExtraAppRecord> getAllApps();
  static void deleteApp(const std::string &app_id);
  static bool appExists(const std::string &app_id);
};

#endif // DATABASE_TABLE_MANAGERS_H
