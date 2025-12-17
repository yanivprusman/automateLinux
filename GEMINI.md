# GEMINI.md - Project Overview

This document provides an overview of the `automateLinux` project, a suite of custom automations designed to enhance and customize a Linux desktop environment. The project integrates various components, including a C++ daemon for system-level operations, a comprehensive set of shell scripts for terminal customization, and `evsieve` configurations for input event remapping.

## Project Overview

The `automateLinux` project aims to provide a highly personalized and automated Linux experience. Its core components work in conjunction to manage system resources, streamline workflows, and enable advanced input device control.

*   **C++ Daemon (`daemon/`)**: A central background service responsible for system-level tasks. It discovers and manages input device paths (keyboard, mouse), maintains a key-value store (`KVTable`), handles directory history, and facilitates inter-process communication via UNIX domain sockets. It exposes a command interface for clients.
*   **Terminal Environment Customization (`terminal/`)**: A modular collection of Bash scripts that extensively customize the user's shell environment. This includes setting environment variables, defining aliases and functions, configuring key bindings, managing the prompt (`PS1`, `PROMPT_COMMAND`), and enabling dynamic behaviors within the terminal.
*   **Input Event Handling (`evsieve/`)**: Utilizes the `evsieve` tool to intercept, remap, and act upon input events from various devices. This allows for complex key rebindings, execution of shell commands based on input patterns, and other advanced input-related automations.
*   **Desktop Integration (`applications/`, `autostart/`, `desktop/`, `gnomeExtensions/`)**: Contains `.desktop` files for application launchers, autostart configurations, and custom Gnome Shell extensions to integrate automations directly into the graphical desktop environment.

## Building and Running

### C++ Daemon

The daemon is a C++ application built using CMake and Make. It can operate as a server or accept commands as a client.

*   **Build**:
    The daemon can be built using the `build.sh` script located in the `daemon/` directory. This script handles CMake configuration, compilation, and post-build steps, including restarting the systemd service.
    ```bash
    # To build and deploy the daemon:
    source ./daemon/build.sh
    ```
*   **Running the Daemon (Server Mode)**:
    The daemon is designed to run as a systemd service (`daemon.service`). The `build.sh` script typically restarts this service upon successful compilation.
*   **Sending Commands to the Daemon (Client Mode)**:
    The daemon executable can act as a client to send JSON commands to a running daemon instance via its UNIX socket.
    ```bash
    # Example: Send a 'ping' command
    ./daemon/main send ping

    # Example: Send a command with arguments
    ./daemon/main send setKeyboard --keyboardName Code
    ```
    Commands are parsed from command-line arguments using a `--key value` format.

### Terminal Environment

The terminal customizations are applied by sourcing several Bash scripts.

*   **Integration**:
    To integrate these customizations into your shell, ensure your `~/.bashrc` sources the project's main `terminal/bashrc` file. This file, in turn, sources other modular scripts.
    ```bash
    # In ~/.bashrc:
    if [ -f "/path/to/automateLinux/terminal/bashrc" ]; then
        . "/path/to/automateLinux/terminal/bashrc"
    fi
    ```
*   **Key Scripts**:
    *   `terminal/firstThing.sh`: Initializes core `AUTOMATE_LINUX_` environment variables, including paths to the daemon's socket and data directories.
    *   `terminal/myBashrc.sh`: Sources additional configuration scripts for aliases, functions, bindings, and manages the shell prompt (`PS1`, `PROMPT_COMMAND`).

### `evsieve` Configurations

`evsieve` is used for advanced input event remapping. Specific configurations are found in the `evsieve/mappings/` directory.

*   **Usage**:
    The `evsieve` instructions provide examples for setting up input hooks, remapping keys, and executing shell commands based on input. These configurations are typically run as separate processes to apply the desired event transformations.
    ```bash
    # Example from evsieve/instructions.txt:
    # Print direct input events:
    sudo evsieve --input /dev/input/event* --print format=direct

    # Remap Ctrl+S to Ctrl+N (illustrative, actual implementation involves more parameters)
    # This involves setting up hooks and send-key commands as described in evsieve documentation.

## Development Conventions

*   **Modularity**: The project emphasizes modularity, particularly within the `terminal/` scripts, where concerns like aliases, functions, and environment variables are separated into distinct files.
*   **Environment Variables**: Extensive use of `AUTOMATE_LINUX_` prefixed environment variables for managing paths, configurations, and inter-script communication.
*   **Daemon-Client Architecture**: A clear client-server model for system services, leveraging UNIX sockets for efficient and secure inter-process communication.
*   **Input Handling**: A significant focus on intercepting and manipulating input events, utilizing both the C++ daemon's input device discovery and `evsieve` for event remapping.
*   **Systemd Integration**: The C++ daemon is managed as a `systemd` service, ensuring it starts automatically and is properly managed by the operating system.
*   **External Dependencies**: The C++ daemon relies on `SQLite3` for data storage and `systemd` for service management. `evsieve` is an external tool used for input event processing.
