#ifndef COMMON_H
#define COMMON_H

#include "Constants.h"
#include "Globals.h"
#include "Types.h"
#include "Utils.h"
#include "using.h"

// Include standard libraries that were in common.h to maintain backward
// compatibility during this refactor step, though files should ideally include
// what they use.
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <signal.h>
#include <sqlite3.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <systemd/sd-daemon.h>
#include <unistd.h>
#include <utility>
#include <vector>

#endif // COMMON_H
