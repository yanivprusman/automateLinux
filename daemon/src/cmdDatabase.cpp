#include "cmdDatabase.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include <sstream>

using namespace std;

static string errorEntryNotFound(const string &key) {
  return string("Entry not found for key ") + key + mustEndWithNewLine;
}

CmdResult handleShowDb(const json &) {
  // Show summary of all tables
  std::stringstream ss;
  ss << "--- Terminal History ---\n";
  for (const auto &p : TerminalTable::getAllHistory()) {
    ss << "TTY " << std::get<0>(p) << " Index " << std::get<1>(p) << ": "
       << std::get<2>(p) << "\n";
  }
  ss << "--- Terminal Sessions ---\n";
  for (const auto &p : TerminalTable::getAllSessions()) {
    ss << "TTY " << p.first << " -> Index " << p.second << "\n";
  }
  ss << "--- Device Registry ---\n";
  ss << "Keyboard: " << DeviceTable::getDevicePath("keyboard") << "\n";
  ss << "Mouse: " << DeviceTable::getDevicePath("mouse") << "\n";
  ss << "--- System Settings ---\n";
  ss << "shouldLogState: " << SettingsTable::getSetting("shouldLogState")
     << "\n";

  return CmdResult(0, ss.str());
}

CmdResult handleUpsertEntry(const json &command) {
  string key = command[COMMAND_ARG_KEY].get<string>();
  string value = command[COMMAND_ARG_VALUE].get<string>();
  SettingsTable::setSetting(key, value);
  return CmdResult(0, "Entry upserted to settings table\n");
}

CmdResult handleGetEntry(const json &command) {
  string key = command[COMMAND_ARG_KEY].get<string>();
  string value = SettingsTable::getSetting(key);
  if (value.empty()) {
    return CmdResult(1, "\n");
  }
  return CmdResult(0, value + "\n");
}

CmdResult handleDeleteEntry(const json &command) {
  string key = command[COMMAND_ARG_KEY].get<string>();
  int rc = SettingsTable::deleteSetting(key);
  if (rc >= 1) {
    return CmdResult(0, "Entry deleted from settings table\n");
  }
  return CmdResult(1, errorEntryNotFound(key));
}

CmdResult handleShowEntriesByPrefix(const json &command) {
  string prefix = command[COMMAND_ARG_PREFIX].get<string>();
  // This command is now ambiguous since we have multiple tables.
  // For backward compatibility, return a message that it's deprecated.
  (void)prefix;
  return CmdResult(
      1, "showEntriesByPrefix is deprecated in the new multi-table schema.\n");
}

CmdResult handleDeleteEntriesByPrefix(const json &command) {
  (void)command;
  return CmdResult(1, "deleteEntriesByPrefix is deprecated.\n");
}
