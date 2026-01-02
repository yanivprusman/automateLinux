#include "main.h"
#include "AutomationManager.h"
#include "ClientSender.h"
#include "DaemonServer.h"
#include "MySQLManager.h"
#include "common.h"
#include <iostream>
#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

string socketPath;
Directories actualDirectories;
Directories &directories = actualDirectories;
Files actualFiles;
Files &files = actualFiles;
volatile int running = 1;
int g_keyboard_fd = -1;
std::ofstream g_logFile;

// Signal handler for clean shutdown
void signalHandler(int signum) {
  cout << "Interrupt signal (" << signum << ") received.\n";
  MySQLManager::stopMySQL();
  exit(signum);
}

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
      // ... in main ...
    } else if (mode == "daemon") {
      signal(SIGINT, signalHandler);
      signal(SIGTERM, signalHandler);

      MySQLManager::initializeAndStartMySQL();

      // Ensure socket directory exists
      std::filesystem::path socketDir =
          std::filesystem::path(SOCKET_PATH).parent_path();
      try {
        if (!std::filesystem::exists(socketDir)) {
          std::filesystem::create_directories(socketDir);
          // Set permissions to be accessible by user
          std::filesystem::permissions(socketDir,
                                       std::filesystem::perms::owner_all |
                                           std::filesystem::perms::group_read |
                                           std::filesystem::perms::group_exec,
                                       std::filesystem::perm_options::add);
        }
      } catch (const std::filesystem::filesystem_error &e) {
        std::cerr << "WARNING: Failed to create/set permissions on socket dir "
                  << socketDir << ": " << e.what() << std::endl;
        std::cerr << "Ensure the directory exists or run as a service with "
                     "RuntimeDirectory configured."
                  << std::endl;
        // Continue execution, bind() might fail later but that is handled
        // cleanly
      }
      if (initialize_daemon() != 0) {
        cerr << "Failed to initialize daemon." << endl;
        MySQLManager::stopMySQL();
        return 1;
      }

      daemon_loop();
      MySQLManager::stopMySQL();
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
