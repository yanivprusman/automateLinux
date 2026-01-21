#include "PortalManager.h"
#include <cstring>
#include <iostream>
#include <pwd.h>
#include <string>
#include <unistd.h>
#include <vector>

void print_help() {
  std::cout << "Usage: portal-handler <command> [options]\n"
            << "Commands:\n"
            << "  request-session    Request a screen sharing session\n"
            << "Options:\n"
            << "  --app-id <id>      Application ID (default: "
               "com.automatelinux.portal-handler)\n"
            << "  --token-dir <path> Directory to store restore token "
               "(default: ~/.config/portal-handler)\n";
}

std::string expand_user(std::string path) {
  if (path.empty() || path[0] != '~')
    return path;
  const char *home = getenv("HOME");
  if (!home) {
    struct passwd *pw = getpwuid(getuid());
    if (pw)
      home = pw->pw_dir;
  }
  if (!home)
    return path;
  return std::string(home) + path.substr(1);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    print_help();
    return 1;
  }

  std::string command = argv[1];
  PortalManager::Config config;
  config.appId = "com.automatelinux.portal-handler";
  config.tokenDir = expand_user("~/.config/portal-handler");

  for (int i = 2; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--app-id" && i + 1 < argc) {
      config.appId = argv[++i];
    } else if (arg == "--token-dir" && i + 1 < argc) {
      config.tokenDir = expand_user(argv[++i]);
    } else {
      std::cerr << "Unknown argument: " << arg << std::endl;
      return 1;
    }
  }

  if (command == "request-session") {
    PortalManager manager(config);
    if (manager.init()) {
      std::cout << "NodeID: " << manager.getNodeId() << std::endl;
      std::cout << "FD: " << manager.getPipeWireFd() << std::endl;

      // Keep the process alive?
      // The FD might be closed if we exit?
      // Yes, checking the Loom implementation, it holds the FD.
      // If this is a one-shot CLI tool, handing off the FD is tricky via
      // stdout. Usually, this tool would be subprocessed and the parent would
      // inherit the FD or receive it via unix socket. BUT, for simple testing
      // or usage where the pipewire node ID is enough (if the other side can
      // open it?), wait... pipewire remote FD is needed to connect to the
      // session.

      // If we exit, the session might close.
      // The prompt said "Portal Handler Application", implying it might need to
      // stay running or pass the FD meaningfully.

      // For now, let's keep it running until a signal or input?
      // "write a dedicated XDG Desktop Portal handleing app"
      // Loom's PortalManager is inside the server which runs continuously.

      // If other apps use this, they will likely spawn it and read the output.
      // If this app exits, the FD is closed.
      // So we should wait for a signal to exit.

      std::cout << "Session active. Press Enter to exit..." << std::endl;
      std::cin.get();
    } else {
      std::cerr << "Failed to initialize session" << std::endl;
      return 1;
    }
  } else {
    print_help();
    return 1;
  }

  return 0;
}
