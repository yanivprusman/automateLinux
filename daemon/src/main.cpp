#include "main.h"
#include "AutomationManager.h"
#include "ClientSender.h"
#include "DaemonServer.h"
#include "DatabaseTableManagers.h"
#include "KeyboardManager.h" // Added include
#include "MySQLManager.h"
#include "PeerManager.h"
#include "Version.h"
#include "common.h"
#include <iostream>
#include <nlohmann/json.hpp>
#include <unistd.h>

using namespace std;
using json = nlohmann::json;

// Forward declarations - implemented in DaemonServer.cpp
extern string getWgInterfaceIP();
extern string getPrimaryMacAddress();

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
  PeerManager::getInstance().stopReconnectLoop();
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
      if (cmdJson.contains("_help_shown")) {
        return 0; // Help was displayed, no need to send to daemon
      }
      return send_command_to_daemon(cmdJson);
    } else if (mode == "daemon") {
      // Signals handled in initialize_daemon()
      cerr << "automateLinux daemon v" << DAEMON_VERSION << endl;

      MySQLManager::initializeAndStartMySQL();

      // Load peer config and restore connections
      PeerManager &pm = PeerManager::getInstance();
      pm.loadConfig();
      if (pm.isLeader() && !pm.getPeerId().empty()) {
        // Leader self-registers in database
        string my_ip = getWgInterfaceIP();
        string my_mac = getPrimaryMacAddress();
        char hostname[256];
        string my_hostname = "";
        if (gethostname(hostname, sizeof(hostname)) == 0) {
          my_hostname = string(hostname);
        }
        PeerTable::upsertPeer(pm.getPeerId(), my_ip, my_mac, my_hostname, true);
        cerr << "Peer config restored: leader " << pm.getPeerId() << endl;
      } else if (!pm.getLeaderAddress().empty()) {
        // Worker connects to leader
        if (pm.connectToLeader()) {
          cerr << "Peer config restored: connected to leader at "
               << pm.getLeaderAddress() << endl;
        } else {
          cerr << "Peer config restored: will retry leader connection" << endl;
        }
        // Start background reconnect loop (handles disconnections and failed initial connects)
        pm.startReconnectLoop();
      }

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
      pm.stopReconnectLoop();
      KeyboardManager::mapper
          .stop(); // Explicitly stop mapper to ungrab devices
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
