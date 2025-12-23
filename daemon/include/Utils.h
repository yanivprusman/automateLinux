#ifndef UTILS_H
#define UTILS_H

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

bool isMultiline(const std::string &s);
std::string toJsonSingleLine(const std::string &s);
std::string readScriptFile(const std::string &relativeScriptPath);
std::string substituteVariable(const std::string &content,
                               const std::string &variable,
                               const std::string &value);
std::string httpGet(const std::string &url);
std::string executeCommand(const char *cmd);

#endif // UTILS_H
