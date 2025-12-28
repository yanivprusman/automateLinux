#ifndef UTILS_H
#define UTILS_H

#include "Constants.h" // Added for AppType helpers and LOG_CORE
#include "Types.h"
#include <chrono>
#include <string>

using std::string;

struct Timer {
  std::chrono::high_resolution_clock::time_point start;
  Timer() : start(std::chrono::high_resolution_clock::now()) {}
  double elapsed() const {
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    return duration.count();
  }
  std::string elapsedStr() const {
    return std::to_string(elapsed()) + " seconds";
  }
};

void logToFile(
    const std::string &message,
    unsigned int category =
        8); // 8 is LOG_CORE (defined in Constants.h, but we can't include it
            // easily here without circular depends if Constants imports Utils?
            // No. But safe to use int literal or assume inclusion)
// Better: include Constants.h or just use int. CONSTANTS_H is safe.
// Let's just use unsigned int and default to LOG_CORE (8) value matching
// Constants.h, or safer: include Constants.h
void forceLog(const std::string &message);
bool isMultiline(const std::string &s);
std::string toJsonSingleLine(const std::string &s);
std::string readScriptFile(const std::string &relativeScriptPath);
std::string substituteVariable(const std::string &content,
                               const std::string &variable,
                               const std::string &value);
std::string getChromeTabUrl(const std::string &preferredTitle = "");
std::string httpGet(const std::string &url);
std::string executeCommand(const char *cmd);
AppType stringToAppType(const std::string &appName);
std::string appTypeToString(AppType type);

void registerLogSubscriber(int fd);
void unregisterLogSubscriber(int fd);

#endif // UTILS_H
