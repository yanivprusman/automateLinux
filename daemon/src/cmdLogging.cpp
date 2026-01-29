#include "cmdLogging.h"
#include "Constants.h"
#include "DatabaseTableManagers.h"
#include "KeyboardManager.h"
#include "Utils.h"
#include <mutex>

using namespace std;

// External globals
extern unsigned int shouldLog;
extern int g_clientSocket;

// Local state for syncing with native host
static std::mutex g_nativeHostSyncMutex;
static int g_nativeHostSocketForSync = -1;

void setLoggingClientSocket(int socket) {
  // This is used internally - not needed for current implementation
  (void)socket;
}

static void syncLoggingWithBridge() {
  std::lock_guard<std::mutex> lock(g_nativeHostSyncMutex);
  if (g_nativeHostSocketForSync != -1) {
    std::string msg = std::string(R"({"action":"updateMask","mask":)") +
                      std::to_string(shouldLog) + "}" + mustEndWithNewLine;
    write(g_nativeHostSocketForSync, msg.c_str(), msg.length());
  }
}

// Called by cmdBrowser when native host registers
void setNativeHostSocketForSync(int socket) {
  std::lock_guard<std::mutex> lock(g_nativeHostSyncMutex);
  g_nativeHostSocketForSync = socket;
}

CmdResult handleShouldLog(const json &command) {
  unsigned int newLogMask = shouldLog;
  auto val = command[COMMAND_ARG_ENABLE];

  if (val.is_string()) {
    string enableStr = val.get<string>();
    if (enableStr == "true") {
      newLogMask = LOG_ALL;
    } else if (enableStr == "false") {
      newLogMask = LOG_NONE;
    } else {
      try {
        newLogMask = std::stoul(enableStr);
      } catch (...) {
        return CmdResult(1, "Invalid logging value string.\n");
      }
    }
  } else if (val.is_number()) {
    newLogMask = val.get<unsigned int>();
  } else {
    return CmdResult(1, "Invalid logging value type.\n");
  }

  shouldLog = newLogMask;
  SettingsTable::setSetting("shouldLogState", to_string(shouldLog));
  syncLoggingWithBridge();
  return CmdResult(0, string("Logging mask set to: ") + to_string(shouldLog) +
                          "\n");
}

CmdResult handleGetShouldLog(const json &) {
  return CmdResult(0, to_string(shouldLog) + string("\n"));
}

CmdResult handleRegisterLogListener(const json &) {
  registerLogSubscriber(g_clientSocket);
  return CmdResult(0, "Subscribed to logs\n");
}

CmdResult handleGetEventFilters(const json &) {
  return CmdResult(0, KeyboardManager::mapper.getEventFiltersJson().dump());
}

CmdResult handleSetEventFilters(const json &command) {
  try {
    json j = json::parse(command[COMMAND_ARG_VALUE].get<string>());
    KeyboardManager::mapper.setEventFilters(j);
    return CmdResult(0, "Event filters updated successfully");
  } catch (const std::exception &e) {
    return CmdResult(1, std::string("Failed to parse filters: ") + e.what());
  }
}
