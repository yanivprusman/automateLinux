#ifndef TYPES_H
#define TYPES_H

enum class AppType { OTHER, TERMINAL, CHROME, CODE };

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
