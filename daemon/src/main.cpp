#include "main.h"
#include <vector>
#include <map>
#include <regex>
#include <sys/select.h>
#include <cstdlib>
#include <array>

string socketPath;
Directories actualDirectories;
Directories& directories = actualDirectories;
Files actualFiles;
Files& files = actualFiles;
KVTable actualKvTable;
KVTable& kvTable = actualKvTable;
DirHistory actualDirHistory;
DirHistory& dirHistory = actualDirHistory;

static volatile int running = 1;
std::ofstream g_logFile;
static int socket_fd = -1;

struct ClientState {
    int fd;
    string buffer;  
    struct ucred cred;
};

std::map<int, ClientState> clients;

void signal_handler(int sig) {
    if (sig == SIGTERM || sig == SIGINT) {
        running = 0;
        if (socket_fd >= 0) {
            shutdown(socket_fd, SHUT_RDWR);
        }
        if (g_logFile.is_open()) {
            g_logFile.close();
        }
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
    if (!result.empty() && result.back() == '\n') {
        result.pop_back();
    }
    return result;
}
void initializeKeyboardPath() {
    string cmd = KEYBOARD_DISCOVERY_CMD;
    string deviceName = executeCommand(cmd.c_str());
    if (!deviceName.empty()) {
        string fullPath = KEYBOARD_INPUT_PATH + deviceName;
        kvTable.upsert(KEYBOARD_PATH_KEY, fullPath);
        cerr << "Keyboard path initialized: " << fullPath << endl;
    } else {
        cerr << "Warning: Could not find Corsair keyboard device" << endl;
    }
}
void initializeMousePath() {
    string cmd = MOUSE_DISCOVERY_CMD;
    string eventNum = executeCommand(cmd.c_str());
    if (!eventNum.empty()) {
        string fullPath = MOUSE_INPUT_PATH + eventNum;
        kvTable.upsert(MOUSE_PATH_KEY, fullPath);
        cerr << "Mouse path initialized: " << fullPath << endl;
    } else {
        cerr << "Warning: Could not find Logitech mouse device" << endl;
    }
}
int setup_socket(){
    socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        cerr << "socket() failed: " << strerror(errno) << endl;
        return 1;
    }
    socketPath = SOCKET_PATH;
    unlink(socketPath.c_str());  
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path)-1);
    if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        cerr << "bind() failed: " << strerror(errno) << endl;
        close(socket_fd);
        return 1;
    }
    if (listen(socket_fd, 10) < 0) {
        cerr << "listen() failed: " << strerror(errno) << endl;
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
    if (client_fd < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            cerr << "Accept failed: " << strerror(errno) << endl;
        }
        return;
    }
    struct ucred cred;
    socklen_t credLen = sizeof(cred);
    if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &cred, &credLen) < 0) {
        cerr << "Failed to get peer credentials" << endl;
        close(client_fd);
        return;
    }
    ClientState state;
    state.fd = client_fd;
    state.cred = cred;
    clients[client_fd] = state;
    cerr << "Client connected: FD=" << client_fd 
         << " PID=" << cred.pid << " UID=" << cred.uid << endl;
}

int handle_client_data(int client_fd) {
    ClientState& state = clients[client_fd];
    char buffer[512];
    ssize_t bytesRead = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0) {
        if (bytesRead < 0) {
            cerr << "Read error on FD " << client_fd << ": " << strerror(errno) << endl;
        } else {
            cerr << "Client disconnected: FD=" << client_fd << endl;
        }
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
        if (!command.empty()) {
            if (!command.empty() && command.back() == '\r') {
                command.pop_back();
            }
            ordered_json j;
            string result;
            try {
                j = json::parse(command);
            } catch (...) {
                result = "ERROR: Invalid JSON\n";
                write(client_fd, result.c_str(), result.length());
                cerr << "Invalid JSON command received." << endl;
                continue;
            }
            string prettyJson;
            prettyJson += "{\n  ";
            for (size_t i = 1; i < command.length() - 1; i++) {
                if (command[i] == ',') {
                    prettyJson += ",\n  ";
                } else {
                    prettyJson += command[i];
                }
            }
            prettyJson += "\n}";
            cerr << "Processing command from FD " << client_fd
                 << ": " << prettyJson << endl;
            mainCommand(j, client_fd);
        }
    }
    return 0;
}

int initialize(){
    cerr << "Starting daemon (single-process mode)" << endl;
    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGPIPE, SIG_IGN);
    g_logFile.open(directories.data + "daemon.log", std::ios::app);
    if (!g_logFile.is_open()) {
        cerr << "Warning: Could not open log file at " << directories.data << "daemon.log" << endl;
    }
    if (setup_socket() != 0) {
        cerr << "Failed to set up socket, exiting." << endl;
        return 1;
    }
    files.initialize(directories);
    initializeKeyboardPath();
    initializeMousePath();
    return 0;
}

void loop(){
    while (running) {
        fd_set read_fds;
        FD_ZERO(&read_fds);
        FD_SET(socket_fd, &read_fds);
        int max_fd = socket_fd;
        for (const auto& pair : clients) {
            int fd = pair.first;
            FD_SET(fd, &read_fds);
            if (fd > max_fd) {
                max_fd = fd;
            }
        }
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        int activity = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout);
        if (activity < 0) {
            if (errno == EINTR) continue;
            cerr << "Select error: " << strerror(errno) << endl;
            break;
        }
        
        if (activity == 0) {
            continue;
        }
        if (FD_ISSET(socket_fd, &read_fds)) {
            accept_new_client();
        }
        std::vector<int> client_fds;
        for (const auto& pair : clients) {
            client_fds.push_back(pair.first);
        }
        for (int fd : client_fds) {
            if (FD_ISSET(fd, &read_fds)) {
                if (clients.find(fd) != clients.end()) {
                    handle_client_data(fd);
                }
            }
        }
    }
    cerr << "Closing " << clients.size() << " client connections..." << endl;
    for (auto& pair : clients) {
        close(pair.first);
    }
    clients.clear();
    close(socket_fd);
    unlink(socketPath.c_str());
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    if (initialize() != 0) {
        cerr << "Initialization failed, exiting." << endl;
        return 1;
    }
    loop();
    cerr << "Daemon shutting down." << endl;
    return 0;
}



