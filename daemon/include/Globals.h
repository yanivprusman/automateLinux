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
  string data;
  string mappings;
  string symlinks;
  string terminal;
  Directories() {
    std::filesystem::path p = canonical("/proc/self/exe").parent_path();
    // Go up until we find a directory that contains 'data', 'evsieve', and
    // 'daemon'
    while (p != p.root_path()) {
      if (std::filesystem::exists(p / "data") &&
          std::filesystem::exists(p / "evsieve") &&
          std::filesystem::exists(p / "daemon")) {
        break;
      }
      p = p.parent_path();
    }
    base = p.string() + "/";
    data = base + "data/";
    mappings = base + "evsieve/mappings/";
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
        {"chrome.log", dirs.data},
        {"combined.log", dirs.data},
        {"daemon.db", dirs.data},
        {"evsieveErr.log", dirs.data},
        {"evsieveOutput.log", dirs.data},
        {"trapErrLogBackground.txt", dirs.data},
        {"trapErrLog.txt", dirs.data},
        {"corsairKeyBoardLogiMouseCode.sh", dirs.mappings},
        {"corsairKeyBoardLogiMouseDefaultKeyboard.sh", dirs.mappings},
        {"corsairKeyBoardLogiMousegnome-terminal-server.sh", dirs.mappings},
        {"corsairKeyBoardLogiMouseAll.sh", dirs.mappings},
        {"corsairKeyBoardLogiMousegoogle-chrome.sh", dirs.mappings},
        {"theRealPath.sh", dirs.terminal},
    };
  }
};

extern Directories &directories;
extern Files &files;
extern string socketPath;
class KVTable;
extern KVTable &kvTable;
class DirHistory;
extern DirHistory &dirHistory;
extern std::ofstream g_logFile;
extern unsigned int shouldLog; // Global logging control bitmask
extern bool g_keyboardEnabled; // Global keyboard enable/disable flag

extern const CommandSignature COMMAND_REGISTRY[];
extern const size_t COMMAND_REGISTRY_SIZE;

#endif // GLOBALS_H
