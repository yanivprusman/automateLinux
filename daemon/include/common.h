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

struct Directories {
    string base;
    string data;
    Directories() {
        base = canonical("/proc/self/exe").parent_path().parent_path().string() + "/";
        data = base + "data/";
    }
};
extern Directories& directories;
extern string socketPath;
class KVTable;
extern KVTable& kvTable;
class DirHistory;
extern DirHistory& dirHistory;


#endif // COMMON_H

