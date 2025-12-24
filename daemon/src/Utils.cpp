#include "Utils.h"
#include "Globals.h" // For g_logFile
#include <array>
#include <cstdio>
#include <curl/curl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using std::cerr;
using std::endl;
using std::vector;

// Centralized logging function that respects the shouldLog flag
void logToFile(const string &message) {
  extern bool shouldLog;
  if (shouldLog && g_logFile.is_open()) {
    g_logFile << message;
    g_logFile.flush();
  }
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
