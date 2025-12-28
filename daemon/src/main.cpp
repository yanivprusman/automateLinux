#include "main.h"
#include "ClientSender.h"
#include "DaemonServer.h"
#include "common.h"
#include <iostream>

using namespace std;

string socketPath;
Directories actualDirectories;
Directories &directories = actualDirectories;
Files actualFiles;
Files &files = actualFiles;
KVTable actualKvTable;
KVTable &kvTable = actualKvTable;
DirHistory actualDirHistory;
volatile int running = 1;
int g_keyboard_fd = -1;
std::ofstream g_logFile;

int main(int argc, char *argv[]) {
  if (argc > 1) {
    string mode = argv[1];
    if (mode == "send") {
      ordered_json cmdJson =
          parse_client_args(argc, argv, 2); // Start parsing from argv[2]
      if (cmdJson.contains("error")) {
        return 1; // An error occurred during parsing
      }
      return send_command_to_daemon(cmdJson);
    } else if (mode == "daemon") {
      if (initialize_daemon() != 0) {
        cerr << "Failed to initialize daemon." << endl;
        return 1;
      }
      daemon_loop();
      cerr << "Daemon shutting down." << endl;
    } else {
      cerr << "Unknown mode: expecting 'daemon send' or 'daemon daemon'\n";
      cerr << "Try: daemon send ping\n";
      cerr << "Or: daemon send enableKeyboard / disableKeyboard\n";
      return 1;
    }
  } else {
    cerr << "Usage: " << argv[0] << " <mode> [args...]\n";
    cerr << "Modes:\n";
    cerr << "  daemon            Run the daemon in server mode.\n";
    cerr << "  send <command>    Send a command to the daemon.\n";
    return 1;
  }
  return 0;
}
