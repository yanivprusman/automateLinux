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
};

#endif // DATABASE_TABLE_MANAGERS_H
