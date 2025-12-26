#include "Utils.h"
#include "Constants.h"
#include "Globals.h" // For g_logFile
#include <array>
#include <cstdio>
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <sstream>
#include <unistd.h>
#include <vector>

using std::endl;
using std::string;
using std::vector;

// Centralized logging function that respects the shouldLog flag
void logToFile(const string &message, unsigned int category) {
  extern unsigned int shouldLog; // Now unsigned int
  if ((shouldLog & category) && g_logFile.is_open()) {
    g_logFile << message << endl; // Add newline for flush consistency
    g_logFile.flush();
  }
  // Mirror to stderr for debugging
  if (shouldLog & category) {
    std::cerr << "[LOG] " << message << std::endl;
  }
}

// Forced logging that always writes to combined.log
void forceLog(const string &message) {
  std::ofstream sf(directories.data + "combined.log", std::ios::app);
  if (sf.is_open()) {
    sf << message << std::endl;
    sf.close();
  }
  // Also echo to cerr for journalctl
  std::cerr << message << std::endl;
}

std::string getChromeTabUrl(const std::string &preferredTitle) {
  // First, check if we have an active tab URL from the Chrome extension
  extern std::string getActiveTabUrlFromExtension();
  std::string extensionUrl = getActiveTabUrlFromExtension();
  if (!extensionUrl.empty()) {
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
    logToFile("[getChromeTabUrl] Failed to parse Chrome DevTools response",
              LOG_CORE);
    return "";
  }

  logToFile("[getChromeTabUrl] preferredTitle=[" + preferredTitle + "]",
            LOG_CORE);

  std::string fallbackUrl = "";
  std::string lastRealUrl = ""; // Last non-chrome:// URL
  int tabIndex = 0;

  for (const auto &tab : root) {
    std::string type = tab["type"].asString();
    std::string url = tab["url"].asString();
    std::string title = tab["title"].asString();

    logToFile("[getChromeTabUrl] Tab #" + std::to_string(tabIndex) +
                  ": type=[" + type + "] url=[" + url + "] title=[" + title +
                  "]",
              LOG_CORE);

    // Skip over extension pages and devtools
    if (type == "page" &&
        url.find("chrome-extension://") == std::string::npos &&
        url.find("devtools://") == std::string::npos) {

      // Store the first valid page as ultimate fallback
      if (fallbackUrl.empty()) {
        fallbackUrl = url;
        logToFile("[getChromeTabUrl] Set fallback URL: " + fallbackUrl,
                  LOG_CORE);
      }

      // Track last non-chrome:// URL (more likely to be the active tab)
      if (url.find("chrome://") == std::string::npos) {
        lastRealUrl = url;
        logToFile("[getChromeTabUrl] Updated lastRealUrl: " + lastRealUrl,
                  LOG_CORE);
      }

      // If we have a preferred title, look for a substring match
      if (!preferredTitle.empty()) {
        bool titleInWindow = preferredTitle.find(title) != std::string::npos;
        bool windowInTitle = title.find(preferredTitle) != std::string::npos;

        logToFile("[getChromeTabUrl] Matching: titleInWindow=" +
                      std::to_string(titleInWindow) +
                      " windowInTitle=" + std::to_string(windowInTitle),
                  LOG_CORE);

        if (titleInWindow || windowInTitle) {
          logToFile("[getChromeTabUrl] TITLE MATCH FOUND! Returning URL: " +
                        url,
                    LOG_CORE);
          return url;
        }
      }
    }
    tabIndex++;
  }

  // Prefer lastRealUrl over fallback (chrome:// URLs)
  if (!lastRealUrl.empty()) {
    logToFile("[getChromeTabUrl] No title match, returning lastRealUrl: " +
                  lastRealUrl,
              LOG_CORE);
    return lastRealUrl;
  }

  logToFile("[getChromeTabUrl] No real URL found, returning fallback: " +
                fallbackUrl,
            LOG_CORE);
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
