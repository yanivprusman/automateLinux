#ifndef COMMON_H
#define COMMON_H

#include "using.h"
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <systemd/sd-daemon.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <sys/stat.h>
#include <sstream>
#include <cerrno>
#include <sqlite3.h>

#define OUTPUT_FILE "/home/yaniv/coding/automateLinux/daemon/output.txt"
#define SOCKET_PATH "/run/automatelinux/automatelinux-daemon.sock"
#define DIR_HISTORY_POINTER_PREFIX "pointerDevPts"
#define IDNEX_OF_LAST_TOUCHED_DIR_KEY "indexOfLastTouchedDir"
#define TTY_KEY "tty"
#define DIR_HISTORY_PREFIX "dirHistory"
#define DIR_HISTORY_DEFAULT_DIR "/home/yaniv/coding/"
#define COMMAND_KEY "command"
#define COMMAND_OPENED_TTY "openedTty"

struct Directories {
    string base;
    string data;
    Directories() {
        base = canonical("/proc/self/exe").parent_path().parent_path().string() + "/";
        data = base + "data/";
    }
};

struct CmdResult {
    int status;         
    std::string message;
    CmdResult(int s = 0, const std::string& msg = "") 
        : status(s), message(msg) {}
};

extern Directories& directories;
extern string socketPath;
class KVTable;
extern KVTable& kvTable;
class DirHistory;
extern DirHistory& dirHistory;


#endif // COMMON_H

