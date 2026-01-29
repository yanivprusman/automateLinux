#include "cmdBrowser.h"
#include "Constants.h"
#include "KeyboardManager.h"
#include "Types.h"
#include "Utils.h"
#include <mutex>
#include <unistd.h>

using namespace std;

// External globals
extern unsigned int shouldLog;
extern int g_clientSocket;

// Forward declaration from cmdLogging.cpp
void setNativeHostSocketForSync(int socket);

// Local state for active tab URL and native host
static std::mutex g_activeTabUrlMutex;
static std::string g_activeTabUrl = "";
static int g_nativeHostSocket = -1;
static std::mutex g_nativeHostSocketMutex;

void setBrowserClientSocket(int socket) {
  // Not needed for current implementation
  (void)socket;
}

std::string getActiveTabUrlFromExtension() {
  std::lock_guard<std::mutex> lock(g_activeTabUrlMutex);
  return g_activeTabUrl;
}

void triggerChromeChatGPTFocus() {
  std::lock_guard<std::mutex> lock(g_nativeHostSocketMutex);
  if (g_nativeHostSocket != -1) {
    std::string msg =
        std::string(R"({"action":"focusChatGPT"})") + mustEndWithNewLine;
    ssize_t written = write(g_nativeHostSocket, msg.c_str(), msg.length());
    if (written < 0) {
      logToFile("[Chrome Extension] Write to native host failed (errno=" +
                    std::to_string(errno) + "). Invalidating socket.",
                LOG_CHROME);
      g_nativeHostSocket = -1;
    } else {
      logToFile("[Chrome] Sent focus request to native host (fd=" +
                    std::to_string(g_nativeHostSocket) +
                    ", written=" + std::to_string(written) + " bytes): " + msg,
                LOG_CHROME);
    }
  } else {
    logToFile("[Chrome] Cannot focus ChatGPT: native host not registered",
              LOG_CHROME);
  }
}

bool isNativeHostConnected() {
  std::lock_guard<std::mutex> lock(g_nativeHostSocketMutex);
  return g_nativeHostSocket != -1;
}

static void syncLoggingWithBridge() {
  std::lock_guard<std::mutex> lock(g_nativeHostSocketMutex);
  if (g_nativeHostSocket != -1) {
    std::string msg = std::string(R"({"action":"updateMask","mask":)") +
                      std::to_string(shouldLog) + "}" + mustEndWithNewLine;
    write(g_nativeHostSocket, msg.c_str(), msg.length());
  }
}

CmdResult handleSetActiveTabUrl(const json &command) {
  std::string url = command[COMMAND_ARG_URL].get<std::string>();
  {
    std::lock_guard<std::mutex> lock(g_activeTabUrlMutex);
    g_activeTabUrl = url;
  }
  logToFile("[Chrome] Active tab changed to: " + url, LOG_CHROME);
  KeyboardManager::setContext(AppType::CHROME, url, "");
  return CmdResult(0, std::string(R"({"status":"ok"})") + mustEndWithNewLine);
}

CmdResult handleRegisterNativeHost(const json &) {
  {
    std::lock_guard<std::mutex> lock(g_nativeHostSocketMutex);
    g_nativeHostSocket = g_clientSocket;
  }
  logToFile("[Chrome] Native messaging host registered", LOG_CHROME);
  // Sync with logging module
  setNativeHostSocketForSync(g_clientSocket);
  syncLoggingWithBridge();
  return CmdResult(0, std::string(R"({"status":"registered"})") +
                          mustEndWithNewLine);
}

CmdResult handleFocusChatGPT(const json &) {
  triggerChromeChatGPTFocus();
  return CmdResult(0, "Focus request sent\n");
}

CmdResult handleFocusAck(const json &) {
  KeyboardManager::onFocusAck();
  return CmdResult(0, std::string(R"({"status":"ok"})") + mustEndWithNewLine);
}
