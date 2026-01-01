#include "ClientSender.h"
#include "common.h"
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using namespace std;

int send_command_to_daemon(const ordered_json &jsonCmd) {
  int client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (client_fd < 0) {
    cerr << "socket() failed: " << strerror(errno) << endl;
    return 1;
  }

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

  if (connect(client_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    cerr << "connect() failed: " << strerror(errno) << endl;
    close(client_fd);
    return 1;
  }

  string jsonCmdStr = jsonCmd.dump();
  write(client_fd, jsonCmdStr.c_str(), jsonCmdStr.length());
  write(client_fd, "\n", 1);

  char buffer[4096];
  string response;
  while (true) {
    ssize_t n = read(client_fd, buffer, sizeof(buffer) - 1);
    if (n <= 0)
      break;
    buffer[n] = '\0';
    response += buffer;
  }
  if (!response.empty()) {
    cout << response;
  }

  close(client_fd);
  return 0;
}

ordered_json parse_client_args(int argc, char *argv[], int start_index) {
  ordered_json j;
  if (argc <= start_index) {
    cerr << "Error: No command provided for client." << endl;
    j["error"] = "No command provided.";
    return j;
  }

  string commandName = argv[start_index];
  j[COMMAND_KEY] = commandName;

  const CommandSignature *foundCommand = nullptr;
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
        string val = argv[i + 1];
        try {
          size_t pos;
          long long num = stoll(val, &pos);
          if (pos == val.length()) { // Entire string is a number
            j[key] = num;
          } else {
            j[key] = val;
          }
        } catch (...) {
          j[key] = val;
        }
        i++; // Consume the value
      } else {
        cerr << "Error: Missing value for argument '" << arg_key_str << "'."
             << endl;
        j["error"] = "Missing argument value.";
        return j;
      }
    } else {
      cerr << "Error: Unexpected argument format '" << arg_key_str << "'."
           << endl;
      j["error"] = "Invalid argument format.";
      return j;
    }
  }

  // Validate required arguments
  for (const string &required_arg : foundCommand->requiredArgs) {
    if (!j.contains(required_arg)) {
      cerr << "Error: Missing required argument '--" << required_arg
           << "' for command '" << commandName << "'." << endl;
      j["error"] = "Missing required argument.";
      return j;
    }
  }

  return j;
}
