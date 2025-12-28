#include "Utils.h"
#include "Constants.h"
#include "Globals.h"
#include "using.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <curl/curl.h>
#include <jsoncpp/json/json.h>
#include <set>
#include <sys/socket.h>

using namespace std;

static std::set<int> g_logSubscribers;
static std::mutex g_logSubscribersMutex;

void registerLogSubscriber(int fd) {
  std::lock_guard<std::mutex> lock(g_logSubscribersMutex);
  g_logSubscribers.insert(fd);
}

void unregisterLogSubscriber(int fd) {
  std::lock_guard<std::mutex> lock(g_logSubscribersMutex);
  g_logSubscribers.erase(fd);
}

// Centralized logging function that respects the shouldLog flag
void logToFile(const string &message, unsigned int category) {
  extern unsigned int shouldLog; // Now unsigned int
  static std::mutex logMutex;
  std::lock_guard<std::mutex> lock(logMutex);
  // logToFile only writes to g_logFile
  if ((shouldLog & category) && g_logFile.is_open()) {
    g_logFile << message << endl;
    g_logFile.flush();

    // Stream to subscribers
    std::lock_guard<std::mutex> subLock(g_logSubscribersMutex);
    if (!g_logSubscribers.empty()) {
      std::string streamMsg = message + "\n";
      for (int fd : g_logSubscribers) {
        // We use send() with MSG_NOSIGNAL to avoid crashing if client closed
        send(fd, streamMsg.c_str(), streamMsg.length(), MSG_NOSIGNAL);
      }
    }
  }
}

// Forced logging that always writes to combined.log
void forceLog(const string &message) {
  std::ofstream sf(directories.data + "combined.log", std::ios::app);
  if (sf.is_open()) {
    sf << message << std::endl;
    sf.close();
  }
}

std::string getChromeTabUrl(const std::string &preferredTitle) {
  // First, check if we have an active tab URL from the Chrome extension
  extern std::string getActiveTabUrlFromExtension();
  std::string extensionUrl = getActiveTabUrlFromExtension();
  extern bool isNativeHostConnected();
  if (isNativeHostConnected() && !extensionUrl.empty()) {
    logToFile("[getChromeTabUrl] Using URL from Chrome extension: " +
                  extensionUrl,
              LOG_CORE);
    return extensionUrl;
  }

  // Fallback to Chrome DevTools Protocol with title matching
  std::string response = httpGet("http://localhost:9222/json");
  Json::Value root;
  Json::Reader reader;
  if (!reader.parse(response, root)) {
    return "";
  }

  std::string fallbackUrl = "";
  std::string lastRealUrl = ""; // Last non-chrome:// URL
  int tabIndex = 0;

  for (const auto &tab : root) {
    std::string type = tab["type"].asString();
    std::string url = tab["url"].asString();
    std::string title = tab["title"].asString();

    // Skip over extension pages and devtools
    if (type == "page" &&
        url.find("chrome-extension://") == std::string::npos &&
        url.find("devtools://") == std::string::npos) {

      // Store the first valid page as ultimate fallback
      if (fallbackUrl.empty()) {
        fallbackUrl = url;
      }

      // Track last non-chrome:// URL (more likely to be the active tab)
      if (url.find("chrome://") == std::string::npos) {
        lastRealUrl = url;
      }

      // If we have a preferred title, look for a substring match
      if (!preferredTitle.empty()) {
        bool titleInWindow = preferredTitle.find(title) != std::string::npos;
        bool windowInTitle = title.find(preferredTitle) != std::string::npos;

        if (titleInWindow || windowInTitle) {
          return url;
        }
      }
    }
    tabIndex++;
  }

  // Prefer lastRealUrl over fallback (chrome:// URLs)
  if (!lastRealUrl.empty()) {
    return lastRealUrl;
  }

  return fallbackUrl;
}

bool isMultiline(const std::string &s) {
  if (s.empty())
    return false;
  size_t end = (s.back() == '\n') ? s.size() - 1 : s.size();
  return s.find('\n') < end;
}

std::string toJsonSingleLine(const std::string &s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    switch (c) {
    case '\n':
      out += "\\n";
      break;
    case '\"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    default:
      out += c;
    }
  }
  return out;
}

std::string readScriptFile(const std::string &relativeScriptPath) {
  std::string scriptContent;
  std::ifstream scriptFile(relativeScriptPath);
  if (!scriptFile.is_open()) {
    logToFile("[ERROR] Failed to open script file: " + relativeScriptPath +
              "\n");
    return "";
  }
  std::stringstream buffer;
  buffer << scriptFile.rdbuf();
  scriptContent = buffer.str();
  scriptFile.close();
  return scriptContent;
}

std::string substituteVariable(const std::string &content,
                               const std::string &variable,
                               const std::string &value) {
  std::string result = content;
  size_t pos = 0;
  std::string searchStr = "$" + variable;
  while ((pos = result.find(searchStr, pos)) != std::string::npos) {
    result.replace(pos, searchStr.length(), value);
    pos += value.length();
  }
  return result;
}

static size_t WriteCallback(void *contents, size_t size, size_t nmemb,
                            std::string *userp) {
  userp->append((char *)contents, size * nmemb);
  return size * nmemb;
}

std::string httpGet(const std::string &url) {
  CURL *curl = curl_easy_init();
  std::string response;
  if (curl) {
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    // Set timeout to avoid hanging
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 2L);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
  }
  return response;
}

std::string executeCommand(const char *cmd) {
  std::array<char, 256> buffer;
  std::string result;
  FILE *pipe = popen(cmd, "r");
  if (!pipe)
    return "";
  while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
    result += buffer.data();
  }
  pclose(pipe);
  if (!result.empty() && result.back() == '\n')
    result.pop_back();
  return result;
}

AppType stringToAppType(const std::string &appName) {
  if (appName == wmClassTerminal)
    return AppType::TERMINAL;
  if (appName == wmClassChrome || appName == "google-chrome")
    return AppType::CHROME;
  if (appName == wmClassCode || appName == "code")
    return AppType::CODE;

  // Case-insensitive check for Antigravity or ChatGPT
  std::string lower = appName;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  if (lower.find("antigravity") != std::string::npos ||
      lower.find("chatgpt") != std::string::npos) {
    return AppType::CHROME;
  }

  return AppType::OTHER;
}

std::string appTypeToString(AppType type) {
  switch (type) {
  case AppType::TERMINAL:
    return "TERMINAL";
  case AppType::CHROME:
    return "CHROME";
  case AppType::CODE:
    return "CODE";
  default:
    return "OTHER";
  }
}
