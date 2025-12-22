#include "main.h"
#include <vector>
#include <map>
#include <regex>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <csignal>
#include <cstdlib>
#include <array>
#include <fstream>
#include <iostream>
#include <cerrno>

using namespace std;

string socketPath;
Directories actualDirectories;
Directories& directories = actualDirectories;
Files actualFiles;
Files& files = actualFiles;
KVTable actualKvTable;
KVTable& kvTable = actualKvTable;
DirHistory actualDirHistory;
volatile int running = 1;
static int socket_fd = -1;
std::ofstream g_logFile;

struct ClientState {
    int fd;
    string buffer;
    struct ucred cred;
};

std::map<int, ClientState> clients;

// ----------------- Daemon Helpers -----------------

void signal_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        running = 0;
        if (socket_fd >= 0) shutdown(socket_fd, SHUT_RDWR);
        if (g_logFile.is_open()) g_logFile.close();
    }
}

string executeCommand(const char* cmd) {
    std::array<char, 256> buffer;
    string result;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);
    if (!result.empty() && result.back() == '\n') result.pop_back();
    return result;
}

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

int setup_socket() {
    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) return 1;

    socketPath = SOCKET_PATH;
    unlink(socketPath.c_str());

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

    if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
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
    if (client_fd < 0) return;

    struct ucred cred;
    socklen_t credLen = sizeof(cred);
    getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &cred, &credLen);

    clients[client_fd] = ClientState{client_fd, "", cred};
    cerr << "Client connected: FD=" << client_fd 
         << " PID=" << cred.pid << " UID=" << cred.uid << endl;
}

int handle_client_data(int client_fd) {
    ClientState& state = clients[client_fd];
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
        if (!command.empty() && command.back() == '\r') command.pop_back();

        ordered_json j;
        try { j = json::parse(command); }
        catch (...) {
            string result = "ERROR: Invalid JSON\n";
            write(client_fd, result.c_str(), result.length());
            continue;
        }
        mainCommand(j, client_fd);
    }
    return 0;
}

int initialize_daemon() {
    cerr << "Starting daemon..." << endl;
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    files.initialize(directories); // Initialize files (and thus directories.data) first
    g_logFile.open(directories.data + "daemon.log", std::ios::app);
    if (!g_logFile.is_open()) {
        cerr << "ERROR: Could not open log file: " << directories.data << "daemon.log" << endl;
    }
    initializeKeyboardPath();
    initializeMousePath();
    return setup_socket();
}

void daemon_loop() {
    while (running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(socket_fd, &read_fds);
        int max_fd = socket_fd;
        for (auto& pair : clients) {
            FD_SET(pair.first, &read_fds);
            if (pair.first > max_fd) max_fd = pair.first;
        }

        struct timeval timeout{1,0};
        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if (activity <= 0) continue;
        if (FD_ISSET(socket_fd, &read_fds)) accept_new_client();

        vector<int> fds;
        for (auto& pair : clients) fds.push_back(pair.first);
        for (int fd : fds) if (FD_ISSET(fd, &read_fds) && clients.find(fd) != clients.end())
            handle_client_data(fd);
    }

    for (auto& pair : clients) close(pair.first);
    close(socket_fd);
    unlink(socketPath.c_str());
}

// ----------------- Client Mode -----------------

int send_command_to_daemon(const ordered_json& jsonCmd) {
    int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd < 0) {
        cerr << "socket() failed: " << strerror(errno) << endl;
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(client_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "connect() failed: " << strerror(errno) << endl;
        close(client_fd);
        return 1;
    }

    string jsonCmdStr = jsonCmd.dump();
    write(client_fd, jsonCmdStr.c_str(), jsonCmdStr.length());
    write(client_fd, "\n", 1);

    char buffer[512];
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        cout << buffer << endl;
    }

    close(client_fd);
    return 0;
}

ordered_json parse_client_args(int argc, char* argv[], int start_index) {
    ordered_json j;
    if (argc <= start_index) {
        cerr << "Error: No command provided for client." << endl;
        j["error"] = "No command provided.";
        return j;
    }

    string commandName = argv[start_index];
    j[COMMAND_KEY] = commandName;

    const CommandSignature* foundCommand = nullptr;
    for (size_t i = 0; i < COMMAND_REGISTRY_SIZE; ++i) {
        if (COMMAND_REGISTRY[i].name == commandName) {
            foundCommand = &COMMAND_REGISTRY[i];
            break;
        }
    }

    if (!foundCommand) {
        cerr << "Error: Unknown command '" << commandName << "'." << endl;
        j["error"] = "Unknown command.";
        return j;
    }

    // Parse arguments
    for (int i = start_index + 1; i < argc; ++i) {
        string arg_key_str = argv[i];
        if (arg_key_str.rfind("--", 0) == 0) { // Starts with --
            string key = arg_key_str.substr(2);
            if (i + 1 < argc) {
                j[key] = argv[i+1];
                i++; // Consume the value
            } else {
                cerr << "Error: Missing value for argument '" << arg_key_str << "'." << endl;
                j["error"] = "Missing argument value.";
                return j;
            }
        } else {
            cerr << "Error: Unexpected argument format '" << arg_key_str << "'." << endl;
            j["error"] = "Invalid argument format.";
            return j;
        }
    }

    // Validate required arguments
    for (const string& required_arg : foundCommand->requiredArgs) {
        if (!j.contains(required_arg)) {
            cerr << "Error: Missing required argument '--" << required_arg << "' for command '" << commandName << "'." << endl;
            j["error"] = "Missing required argument.";
            return j;
        }
    }

    return j;
}

// ----------------- Main -----------------

int main(int argc, char* argv[]) {
    if (argc > 1) {
        string mode = argv[1];
        if (mode == "send") {
            ordered_json cmdJson = parse_client_args(argc, argv, 2); // Start parsing from argv[2]
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
            cerr << "Or: daemon send setKeyboard --keyboardName Code\n";
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



