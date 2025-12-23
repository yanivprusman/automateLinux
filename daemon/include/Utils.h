#ifndef UTILS_H
#define UTILS_H

#include <chrono>
#include <string>

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

inline bool isMultiline(const std::string &s) {
  if (s.empty())
    return false;
  size_t end = (s.back() == '\n') ? s.size() - 1 : s.size();
  return s.find('\n') < end;
}

inline std::string toJsonSingleLine(const std::string &s) {
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

#endif // UTILS_H
