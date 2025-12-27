#ifndef TYPES_H
#define TYPES_H

enum class AppType { OTHER, TERMINAL, CHROME, CODE };

// G-Key number constants
constexpr int G_NONE = 0;  // No G-key (regular key/mouse button)
constexpr int G1 = 1;
constexpr int G2 = 2;
constexpr int G5 = 5;
constexpr int G6 = 6;

#include <string>
#include <vector>

using std::string;
using std::vector;

struct CommandSignature {
  string name;
  vector<string> requiredArgs;
  CommandSignature(const string &n, const vector<string> &args)
      : name(n), requiredArgs(args) {}
};

struct CmdResult {
  int status;
  std::string message;
  CmdResult(int s = 0, const std::string &msg = "") : status(s), message(msg) {}
};

#endif // TYPES_H
