#include "DaemonServer.h"
#include "DatabaseTableManagers.h"
#include "KeyboardManager.h"
#include "PeerManager.h"
#include "Utils.h"
#include "common.h"
#include "main.h"
#include "mainCommand.h"
#include "sendKeys.h"
#include "using.h"
#include <arpa/inet.h>
#include <array>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <ifaddrs.h>
#include <iostream>
#include <map>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

static int socket_fd = -1;
static int peer_socket_fd = -1;  // TCP socket for peer-to-peer communication
extern int g_keyboard_fd;        // Defined in main.cpp, used here
extern volatile int running;     // Defined in main.cpp
extern std::ofstream g_logFile;  // Defined in Globals.h/main.cpp
extern bool g_keyboardEnabled;   // Defined in mainCommand.cpp

struct ClientState {
  int fd;
  string buffer;
  struct ucred cred;
};

struct PeerClientState {
  int fd;
  string buffer;
  string peer_ip;
  bool authenticated;
};

static std::map<int, ClientState> clients;
static std::map<int, PeerClientState> peer_clients;

void emitDaemonReadySignal() {
  logToFile("Emitting daemon ready DBus signal", LOG_CORE);
  int rc = system(DBUS_SIGNAL_COMMAND);
  if (rc != 0) {
    logToFile("ERROR: Failed to emit DBus signal, rc=" + std::to_string(rc),
              LOG_CORE);
  }
}

void initializeKeyboardPath() {
  string deviceName = executeCommand(KEYBOARD_DISCOVERY_CMD);
  if (!deviceName.empty()) {
    string fullPath = KEYBOARD_INPUT_PATH + deviceName;
    DeviceTable::setDevicePath("keyboard", fullPath);
    cerr << "Keyboard path initialized: " << fullPath << endl;
  }
}

void initializeMousePath() {
  string eventNum = executeCommand(MOUSE_DISCOVERY_CMD);
  if (!eventNum.empty()) {
    string fullPath = MOUSE_INPUT_PATH + eventNum;
    DeviceTable::setDevicePath("mouse", fullPath);
    cerr << "Mouse path initialized: " << fullPath << endl;
  }
}

void openKeyboardDevice() {
  string keyboard_path = DeviceTable::getDevicePath("keyboard");
  if (keyboard_path.empty()) {
    cerr << "ERROR: Keyboard path not set, cannot open device" << endl;
    return;
  }

  g_keyboard_fd = open(keyboard_path.c_str(), O_WRONLY);
  if (g_keyboard_fd < 0) {
    cerr << "ERROR: Could not open keyboard device: " << keyboard_path << endl;
  } else {
    cerr << "Keyboard device opened: " << keyboard_path
         << " (fd=" << g_keyboard_fd << ")" << endl;
  }
}

int setup_socket() {
  // Use SOCK_CLOEXEC to prevent child processes (like loom-server) from
  // inheriting this FD
  socket_fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (socket_fd < 0) {
    cerr << "ERROR: socket() failed: " << strerror(errno) << endl;
    return 1;
  }

  socketPath = SOCKET_PATH;

  // Check if another daemon is already running
  int temp_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (temp_fd >= 0) {
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
    if (connect(temp_fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
      cerr << "ERROR: Another daemon is already running on " << socketPath
           << endl;
      close(temp_fd);
      close(socket_fd);
      return 1;
    }
    close(temp_fd);
  }

  unlink(socketPath.c_str());

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

  if (bind(socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    cerr << "ERROR: bind() failed for " << socketPath << ": " << strerror(errno)
         << endl;
    if (errno == ENOENT) {
      cerr << "TIP: Ensure the parent directory of the socket exists." << endl;
    }
    close(socket_fd);
    return 1;
  }
  if (listen(socket_fd, 10) < 0) {
    close(socket_fd);
    return 1;
  }

  chmod(socketPath.c_str(), 0666);
  chown(socketPath.c_str(), getuid(), getgid());
  cerr << "Socket listening on: " << socketPath << endl;
  return 0;
}

// Get the IP address of the wg0 interface
string getWgInterfaceIP() {
  struct ifaddrs *ifaddr, *ifa;
  char ip[INET_ADDRSTRLEN];
  string result = "";

  if (getifaddrs(&ifaddr) == -1) {
    cerr << "ERROR: getifaddrs() failed: " << strerror(errno) << endl;
    return "";
  }

  for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == nullptr)
      continue;

    if (ifa->ifa_addr->sa_family == AF_INET) {
      string ifname = ifa->ifa_name;
      if (ifname == "wg0") {
        struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
        inet_ntop(AF_INET, &(addr->sin_addr), ip, INET_ADDRSTRLEN);
        result = ip;
        break;
      }
    }
  }

  freeifaddrs(ifaddr);
  return result;
}

int setup_peer_socket() {
  string wg_ip = getWgInterfaceIP();
  if (wg_ip.empty()) {
    cerr << "INFO: wg0 interface not found, peer networking disabled" << endl;
    return 0;  // Not an error, just no WireGuard available
  }

  peer_socket_fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
  if (peer_socket_fd < 0) {
    cerr << "ERROR: peer socket() failed: " << strerror(errno) << endl;
    return 1;
  }

  // Allow reuse of address
  int opt = 1;
  setsockopt(peer_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PEER_TCP_PORT);
  inet_pton(AF_INET, wg_ip.c_str(), &addr.sin_addr);

  if (bind(peer_socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    cerr << "ERROR: peer bind() failed for " << wg_ip << ":" << PEER_TCP_PORT
         << ": " << strerror(errno) << endl;
    close(peer_socket_fd);
    peer_socket_fd = -1;
    return 1;
  }

  if (listen(peer_socket_fd, 10) < 0) {
    cerr << "ERROR: peer listen() failed: " << strerror(errno) << endl;
    close(peer_socket_fd);
    peer_socket_fd = -1;
    return 1;
  }

  cerr << "Peer socket listening on: " << wg_ip << ":" << PEER_TCP_PORT << endl;
  logToFile("Peer networking enabled on " + wg_ip + ":" +
                to_string(PEER_TCP_PORT),
            LOG_CORE);
  return 0;
}

void accept_new_client() {
  int client_fd = accept(socket_fd, nullptr, nullptr);
  if (client_fd < 0)
    return;

  // Set FD_CLOEXEC logic to prevent child processes from inheriting this FD.
  // This is crucial for commands like restartLoom that spawn long-running
  // children.
  fcntl(client_fd, F_SETFD, FD_CLOEXEC);

  struct ucred cred;
  socklen_t credLen = sizeof(cred);
  getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &cred, &credLen);

  clients[client_fd] = ClientState{client_fd, "", cred};
  cerr << "Client connected: FD=" << client_fd << " PID=" << cred.pid
       << " UID=" << cred.uid << endl;
}

void accept_new_peer() {
  struct sockaddr_in peer_addr;
  socklen_t peer_len = sizeof(peer_addr);
  int peer_fd = accept(peer_socket_fd, (struct sockaddr *)&peer_addr, &peer_len);
  if (peer_fd < 0)
    return;

  fcntl(peer_fd, F_SETFD, FD_CLOEXEC);

  char ip_str[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(peer_addr.sin_addr), ip_str, INET_ADDRSTRLEN);

  peer_clients[peer_fd] = PeerClientState{peer_fd, "", ip_str, false};
  cerr << "Peer connected: FD=" << peer_fd << " IP=" << ip_str << endl;
  logToFile("Peer connected from " + string(ip_str), LOG_CORE);
}

int handle_peer_data(int peer_fd) {
  PeerClientState &state = peer_clients[peer_fd];
  char buffer[4096];
  ssize_t bytesRead = read(peer_fd, buffer, sizeof(buffer) - 1);
  if (bytesRead <= 0) {
    cerr << "Peer disconnected: FD=" << peer_fd << " IP=" << state.peer_ip
         << endl;
    logToFile("Peer disconnected: " + state.peer_ip, LOG_CORE);
    close(peer_fd);
    peer_clients.erase(peer_fd);
    return 0;
  }

  buffer[bytesRead] = '\0';
  state.buffer += buffer;

  size_t pos;
  while ((pos = state.buffer.find('\n')) != string::npos) {
    string message = state.buffer.substr(0, pos);
    state.buffer.erase(0, pos + 1);
    if (!message.empty() && message.back() == '\r')
      message.pop_back();

    ordered_json j;
    try {
      j = json::parse(message);
    } catch (...) {
      string result = "ERROR: Invalid JSON\n";
      write(peer_fd, result.c_str(), result.length());
      continue;
    }

    // Handle peer messages
    // Peer connections are persistent - don't close based on mainCommand return
    logToFile("Peer message from " + state.peer_ip + ": " + message, LOG_CORE);

    // Just process the command - peer connections stay open
    mainCommand(j, peer_fd);
  }
  return 0;
}

// Handle commands from leader (for workers)
static string leader_buffer;

int handle_leader_data() {
  PeerManager &pm = PeerManager::getInstance();
  int leader_fd = pm.getLeaderSocket();
  if (leader_fd < 0)
    return 0;

  char buffer[4096];
  ssize_t bytesRead = read(leader_fd, buffer, sizeof(buffer) - 1);
  if (bytesRead <= 0) {
    logToFile("Lost connection to leader", LOG_CORE);
    pm.disconnectFromLeader();
    leader_buffer.clear();
    return 0;
  }

  buffer[bytesRead] = '\0';
  leader_buffer += buffer;

  size_t pos;
  while ((pos = leader_buffer.find('\n')) != string::npos) {
    string message = leader_buffer.substr(0, pos);
    leader_buffer.erase(0, pos + 1);
    if (!message.empty() && message.back() == '\r')
      message.pop_back();

    ordered_json j;
    try {
      j = json::parse(message);
    } catch (...) {
      logToFile("Invalid JSON from leader: " + message, LOG_CORE);
      continue;
    }

    logToFile("Command from leader: " + message, LOG_CORE);
    mainCommand(j, leader_fd);
  }
  return 0;
}

int handle_client_data(int client_fd) {
  ClientState &state = clients[client_fd];
  char buffer[4096];
  ssize_t bytesRead = read(client_fd, buffer, sizeof(buffer) - 1);
  if (bytesRead <= 0) {
    unregisterLogSubscriber(client_fd);
    close(client_fd);
    clients.erase(client_fd);
    return 0;
  }

  buffer[bytesRead] = '\0';
  state.buffer += buffer;

  size_t pos;
  while ((pos = state.buffer.find('\n')) != string::npos) {
    string command = state.buffer.substr(0, pos);
    state.buffer.erase(0, pos + 1);
    if (!command.empty() && command.back() == '\r')
      command.pop_back();

    ordered_json j;
    try {
      j = json::parse(command);
    } catch (...) {
      string result = "ERROR: Invalid JSON\n";
      write(client_fd, result.c_str(), result.length());
      continue;
    }
    if (mainCommand(j, client_fd) == 1) {
      // If mainCommand returns 1, it means we should close the connection.
      // We don't need to call close() here as mainCommand already might have or
      // we will. Actually mainCommand shouldn't close it, handle_client_data
      // should.
      unregisterLogSubscriber(client_fd);
      close(client_fd);
      clients.erase(client_fd);
      return 0;
    }
  }
  return 0;
}

// Signal handler for clean shutdown
void signal_handler(int sig) {
  if (sig == SIGTERM || sig == SIGINT) {
    running = 0;
  }
}

int initialize_daemon() {
  cerr << "Starting daemon..." << endl;
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGPIPE, SIG_IGN);

  files.initialize(directories);

  // Restore logging state EARLY
  string savedLogState = SettingsTable::getSetting("shouldLogState");
  if (!savedLogState.empty()) {
    try {
      shouldLog = std::stoul(savedLogState);
    } catch (...) {
      shouldLog = LOG_CORE;
    }
  } else {
    shouldLog = LOG_CORE;
  }

  g_logFile.open(directories.data + "combined.log", std::ios::app);
  if (!g_logFile.is_open()) {
    cerr << "ERROR: Could not open log file: " << directories.data
         << "combined.log" << endl;
  }

  logToFile("Logging mask restored/initialized to: " +
                std::to_string(shouldLog),
            LOG_CORE);

  initializeKeyboardPath();
  initializeMousePath();
  KeyboardManager::mapper.loadPersistence();
  openKeyboardDevice();
  int rc = setup_socket();
  if (rc != 0) {
    if (g_keyboard_fd >= 0) {
      close(g_keyboard_fd);
      g_keyboard_fd = -1;
    }
    return rc;
  }

  // Setup peer-to-peer socket (optional - only if wg0 exists)
  rc = setup_peer_socket();
  if (rc != 0) {
    logToFile("WARNING: Peer socket setup failed, continuing without peer networking", LOG_CORE);
    // Don't fail daemon startup if peer socket fails
  }

  // DISABLED: Keyboard grab feature removed - using separate keybinding project
  // if (KeyboardManager::setKeyboard(g_keyboardEnabled).status != 0) {
  //   logToFile("ERROR: Failed to initialize keyboard mapping", LOG_CORE);
  // }

  // Start Loom on daemon startup (both prod and dev)
  string restartLoomScript =
      directories.base + "daemon/scripts/restart_loom.sh";
  // Export DBUS_SESSION_BUS_ADDRESS so it can talk to the user bus.
  string cmdBase =
      "export DBUS_SESSION_BUS_ADDRESS=unix:path=/run/user/1000/bus; " +
      restartLoomScript;

  string cmdProd = cmdBase + " --prod > /dev/null 2>&1 &";
  string cmdDev = cmdBase + " --dev > /dev/null 2>&1 &";

  int prodRc = system(cmdProd.c_str());
  int devRc = system(cmdDev.c_str());

  if (prodRc != 0 || devRc != 0) {
    logToFile(
        "WARNING: Failed to start Loom (Prod RC=" + std::to_string(prodRc) +
            ", Dev RC=" + std::to_string(devRc) + ")",
        LOG_CORE);
  } else {
    logToFile("Loom startup scripts (prod & dev) executed as user yaniv",
              LOG_CORE);
  }

  // Start Dashboard
  string startDashboardScript =
      directories.base + "daemon/scripts/start_dashboard.sh";
  string dashboardCmd = startDashboardScript + " > /dev/null 2>&1 &";
  int dashboardRc = system(dashboardCmd.c_str());
  if (dashboardRc != 0) {
    logToFile("WARNING: Failed to start Dashboard, rc=" +
                  std::to_string(dashboardRc),
              LOG_CORE);
  } else {
    logToFile("Dashboard startup script executed", LOG_CORE);
  }

  emitDaemonReadySignal();

  return 0;
}

void daemon_loop() {
  while (running) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &read_fds);
    int max_fd = socket_fd;

    // Add peer socket if available
    if (peer_socket_fd >= 0) {
      FD_SET(peer_socket_fd, &read_fds);
      if (peer_socket_fd > max_fd)
        max_fd = peer_socket_fd;
    }

    // Add local clients
    for (auto &pair : clients) {
      FD_SET(pair.first, &read_fds);
      if (pair.first > max_fd)
        max_fd = pair.first;
    }

    // Add peer clients
    for (auto &pair : peer_clients) {
      FD_SET(pair.first, &read_fds);
      if (pair.first > max_fd)
        max_fd = pair.first;
    }

    // For workers: add leader socket to receive commands from leader
    PeerManager &pm = PeerManager::getInstance();
    int leader_fd = pm.isConnectedToLeader() ? pm.getLeaderSocket() : -1;
    if (leader_fd >= 0) {
      FD_SET(leader_fd, &read_fds);
      if (leader_fd > max_fd)
        max_fd = leader_fd;
    }

    struct timeval timeout{
        0, 200000}; // 200ms timeout for faster shutdown response
    int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
    if (activity <= 0)
      continue;

    // Accept new local clients
    if (FD_ISSET(socket_fd, &read_fds))
      accept_new_client();

    // Accept new peer connections
    if (peer_socket_fd >= 0 && FD_ISSET(peer_socket_fd, &read_fds))
      accept_new_peer();

    // Handle local client data
    vector<int> fds;
    for (auto &pair : clients)
      fds.push_back(pair.first);
    for (int fd : fds)
      if (FD_ISSET(fd, &read_fds) && clients.find(fd) != clients.end())
        handle_client_data(fd);

    // Handle peer data
    vector<int> peer_fds;
    for (auto &pair : peer_clients)
      peer_fds.push_back(pair.first);
    for (int fd : peer_fds)
      if (FD_ISSET(fd, &read_fds) && peer_clients.find(fd) != peer_clients.end())
        handle_peer_data(fd);

    // Handle data from leader (for workers)
    if (leader_fd >= 0 && FD_ISSET(leader_fd, &read_fds))
      handle_leader_data();
  }

  // Cleanup local clients
  for (auto &pair : clients)
    close(pair.first);
  close(socket_fd);
  unlink(socketPath.c_str());

  // Cleanup peer clients and socket
  for (auto &pair : peer_clients)
    close(pair.first);
  if (peer_socket_fd >= 0) {
    close(peer_socket_fd);
    peer_socket_fd = -1;
  }

  if (g_keyboard_fd >= 0) {
    close(g_keyboard_fd);
    g_keyboard_fd = -1;
  }
  if (g_logFile.is_open()) {
    g_logFile.close();
  }
}
