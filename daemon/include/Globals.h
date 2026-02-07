#ifndef GLOBALS_H
#define GLOBALS_H

#include "Types.h"
#include "using.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using std::string;
using std::vector;
using std::filesystem::canonical;

struct Directories {
  string base;
  string dev;
  string data;
  string symlinks;
  string terminal;
  Directories() {
    std::error_code ec;
    std::filesystem::path p = std::filesystem::canonical("/proc/self/exe", ec);
    if (ec) {
      p = "/opt/automateLinux/daemon/daemon"; // Fallback if canonical fails
    }
    p = p.parent_path();

    bool found = false;
    // Walk up until we find the repository root defined by these markers
    while (p.has_parent_path() && p != p.root_path()) {
      if (std::filesystem::exists(p / "data") &&
          std::filesystem::exists(p / "daemon") &&
          std::filesystem::exists(p / "symlinks")) {
        found = true;
        break;
      }
      p = p.parent_path();
    }

    if (!found) {
      // Final absolute fallback
      base = "/opt/automateLinux/";
    } else {
      base = p.string() + "/";
    }
    dev = "/opt/dev/";
    data = base + "data/";
    symlinks = base + "symlinks/";
    terminal = base + "terminal/";
  }
};

struct Files {
  struct File {
    string name;
    string dir;
    string fullPath() const { return dir + name; }
  };
  vector<File> files;
  void initialize(const Directories &dirs) {
    files = {
        {"chrome.log", dirs.data},     {"combined.log", dirs.data},
        {"daemon.db", dirs.data},      {"trapErrLogBackground.txt", dirs.data},
        {"trapErrLog.txt", dirs.data}, {"theRealPath.sh", dirs.terminal},
    };
  }
};

extern Directories &directories;
extern Files &files;
extern string socketPath;
extern std::ofstream g_logFile;
extern unsigned int shouldLog; // Global logging control bitmask
extern bool g_keyboardEnabled; // Global keyboard enable/disable flag

extern const CommandSignature COMMAND_REGISTRY[];
extern const size_t COMMAND_REGISTRY_SIZE;

#endif // GLOBALS_H
