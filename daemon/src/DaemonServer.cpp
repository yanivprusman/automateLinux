#include "DaemonServer.h"
#include "KeyboardManager.h"
#include "Utils.h" // For executeCommand
#include "common.h"
#include "main.h" // For KVTable, DirHistory, etc. declarations
#include "mainCommand.h"
#include "sendKeys.h"
#include <array>
#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;

static int socket_fd = -1;
extern int g_keyboard_fd;       // Defined in main.cpp, used here
extern volatile int running;    // Defined in main.cpp
extern std::ofstream g_logFile; // Defined in Globals.h/main.cpp
extern bool g_keyboardEnabled;  // Defined in mainCommand.cpp

struct ClientState {
  int fd;
  string buffer;
  struct ucred cred;
};

static std::map<int, ClientState> clients;

void initializeKeyboardPath() {
  string deviceName = executeCommand(KEYBOARD_DISCOVERY_CMD);
  if (!deviceName.empty()) {
    string fullPath = KEYBOARD_INPUT_PATH + deviceName;
    kvTable.upsert(KEYBOARD_PATH_KEY, fullPath);
    cerr << "Keyboard path initialized: " << fullPath << endl;
  }
}

void initializeMousePath() {
  string eventNum = executeCommand(MOUSE_DISCOVERY_CMD);
  if (!eventNum.empty()) {
    string fullPath = MOUSE_INPUT_PATH + eventNum;
    kvTable.upsert(MOUSE_PATH_KEY, fullPath);
    cerr << "Mouse path initialized: " << fullPath << endl;
  }
}

void openKeyboardDevice() {
  string keyboard_path = kvTable.get(KEYBOARD_PATH_KEY);
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
  socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
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

  chmod(socketPath.c_str(), 0660);
  chown(socketPath.c_str(), getuid(), getgid());
  cerr << "Socket listening on: " << socketPath << endl;
  return 0;
}

void accept_new_client() {
  int client_fd = accept(socket_fd, nullptr, nullptr);
  if (client_fd < 0)
    return;

  struct ucred cred;
  socklen_t credLen = sizeof(cred);
  getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &cred, &credLen);

  clients[client_fd] = ClientState{client_fd, "", cred};
  cerr << "Client connected: FD=" << client_fd << " PID=" << cred.pid
       << " UID=" << cred.uid << endl;
}

int handle_client_data(int client_fd) {
  ClientState &state = clients[client_fd];
  char buffer[512];
  ssize_t bytesRead = read(client_fd, buffer, sizeof(buffer) - 1);
  if (bytesRead <= 0) {
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
    mainCommand(j, client_fd);
  }
  return 0;
}

pid_t g_httpBridgePid = -1;

void launchHttpBridge() {
  pid_t pid = fork();
  if (pid == 0) {
    // Child process
    // Redirect stdout/stderr to log file or /dev/null
    int devNull = open("/dev/null", O_WRONLY);
    dup2(devNull, STDOUT_FILENO);
    dup2(devNull, STDERR_FILENO);
    close(devNull);

    // Execute the script
    // Assuming the script is in the same directory as the daemon executable or
    // known path We'll use the repository path we know
    execl("/usr/bin/python3", "python3",
          "/home/yaniv/coding/automateLinux/daemon/http-bridge.py", NULL);

    // If execl returns, it failed
    exit(1);
  } else if (pid > 0) {
    g_httpBridgePid = pid;
    cerr << "Started HTTP bridge with PID: " << pid << endl;
  } else {
    cerr << "Failed to fork HTTP bridge: " << strerror(errno) << endl;
  }
}

void signal_handler(int sig) {
  if (sig == SIGTERM || sig == SIGINT) {
    running = 0;
    if (g_httpBridgePid > 0) {
      kill(g_httpBridgePid, SIGTERM);
      waitpid(g_httpBridgePid, nullptr, 0); // Wait for it to exit
    }
    if (socket_fd >= 0)
      shutdown(socket_fd, SHUT_RDWR);
    if (g_keyboard_fd >= 0)
      close(g_keyboard_fd);
    if (g_logFile.is_open())
      g_logFile.close();
  }
}

int initialize_daemon() {
  cerr << "Starting daemon..." << endl;
  signal(SIGTERM, signal_handler);
  signal(SIGINT, signal_handler);
  signal(SIGPIPE, SIG_IGN);

  launchHttpBridge();

  files.initialize(
      directories); // Initialize files (and thus directories.data) first
  g_logFile.open(directories.data + "combined.log", std::ios::app);
  if (!g_logFile.is_open()) {
    cerr << "ERROR: Could not open log file: " << directories.data
         << "combined.log" << endl;
  }
  initializeKeyboardPath();
  initializeMousePath();
  openKeyboardDevice();
  int rc = setup_socket();
  if (rc != 0) {
    if (g_keyboard_fd >= 0) {
      close(g_keyboard_fd);
      g_keyboard_fd = -1;
    }
    return rc;
  }

  // Initialize keyboard to default enabled state on daemon startup
  if (KeyboardManager::setKeyboard(g_keyboardEnabled).status != 0) {
    cerr << "ERROR: Failed to initialize keyboard mapping" << endl;
    // Don't return error here, let the daemon loop run so we can debug via
    // socket
  }
  // Restore logging state
  string savedLogState = kvTable.get("shouldLogState");
  if (!savedLogState.empty()) {
    try {
      shouldLog = std::stoul(savedLogState);
      logToFile("Restored logging state: " + std::to_string(shouldLog),
                LOG_CORE);
    } catch (...) {
      shouldLog = LOG_INPUT; // Default fallback
    }
  } else {
    shouldLog = LOG_INPUT; // Default if nothing saved
  }

  return 0;
}

void daemon_loop() {
  while (running) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(socket_fd, &read_fds);
    int max_fd = socket_fd;
    for (auto &pair : clients) {
      FD_SET(pair.first, &read_fds);
      if (pair.first > max_fd)
        max_fd = pair.first;
    }

    struct timeval timeout{1, 0};
    int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
    if (activity <= 0)
      continue;
    if (FD_ISSET(socket_fd, &read_fds))
      accept_new_client();

    vector<int> fds;
    for (auto &pair : clients)
      fds.push_back(pair.first);
    for (int fd : fds)
      if (FD_ISSET(fd, &read_fds) && clients.find(fd) != clients.end())
        handle_client_data(fd);
  }

  for (auto &pair : clients)
    close(pair.first);
  close(socket_fd);
  unlink(socketPath.c_str());
}
