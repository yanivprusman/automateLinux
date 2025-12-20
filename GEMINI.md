# GEMINI.md - Project Overview

This document provides an overview of the `automateLinux` project, a suite of custom automations designed to enhance and customize a Linux desktop environment. The project integrates various components, including a C++ daemon for system-level operations, a comprehensive set of shell scripts for terminal customization, and `evsieve` configurations for input event remapping.

## Project Overview

The `automateLinux` project aims to provide a highly personalized and automated Linux experience. Its core components work in conjunction to manage system resources, streamline workflows, and enable advanced input device control.

*   **C++ Daemon (`daemon/`)**: A central background service responsible for system-level tasks. It discovers and manages input device paths (keyboard, mouse), maintains a key-value store (`KVTable`), handles directory history, and facilitates inter-process communication via UNIX domain sockets. It exposes a command interface for clients.
*   **Terminal Environment Customization (`terminal/`)**: A modular collection of Bash scripts that extensively customize the user's shell environment. This includes setting environment variables, defining aliases and functions, configuring key bindings, managing the prompt (`PS1`, `PROMPT_COMMAND`), and enabling dynamic behaviors within the terminal.
*   **Input Event Handling (`evsieve/`)**: Utilizes the `evsieve` tool to intercept, remap, and act upon input events from various devices. This allows for complex key rebindings, execution of shell commands based on input patterns, and other advanced input-related automations.
*   **GNOME Shell Extensions (`gnomeExtensions/`)**: A collection of extensions that provide desktop integration. These extensions leverage a shared library (`gnomeExtensions/lib/`) for common functionality:
    *   `logging.js`: Provides a standardized logging class for consistent log output across all extensions.
    *   `daemon.js`: Provides a `DaemonConnector` class that centralizes the logic for communicating with the C++ daemon via its UNIX domain socket.
    *   `shellCommand.js`: Provides a `ShellCommandExecutor` class to asynchronously execute shell commands directly from extensions, used for system-level actions like shutdown.

    The `clock@ya-niv.com` extension now features:
    *   **Position Persistence**: The clock's position is saved via the C++ daemon (`KVTable`) and reloaded upon extension enablement, ensuring it retains its last-known position across sessions.
    *   **Right-Click Menu**: A right-click context menu has been added to the clock label, offering a "Shut Down" option that directly executes a system power-off command (`/usr/bin/systemctl poweroff`).
*   **Desktop Integration (`applications/`, `autostart/`, `desktop/`)**: Contains `.desktop` files for application launchers, autostart configurations, and custom Gnome Shell extensions to integrate automations directly into the graphical desktop environment.

## Building and Running

### C++ Daemon

The daemon is a C++ application built using CMake and Make. It can operate as a server or accept commands as a client.

*   **Build**:
    The daemon can be built using the `build.sh` script located in the `daemon/` directory. This script handles CMake configuration, compilation, and post-build steps, including restarting the systemd service.
    ```bash
    # To build and deploy the daemon:
    bd
    ```
*   **Running the Daemon (Server Mode)**:
    The daemon is designed to run as a systemd service (`daemon.service`). The `build.sh` script typically restarts this service upon successful compilation.
*   **Sending Commands to the Daemon (Client Mode)**:
    The `daemon` executable is available in the system's PATH and can act as a client to send JSON commands to a running daemon instance via its UNIX socket.
    ```bash
    # Example: Send a 'ping' command
    daemon send ping

    # Example: Send a command with arguments
    daemon send setKeyboard --keyboardName Code
    ```
    Commands are parsed from command-line arguments using a `--key value` format.

*   **Discovering Available Commands**:
    Before adding new functionality, it's crucial to check for existing daemon commands to avoid errors like `Unknown command`. The C++ source code is the definitive source of truth.

    *   **Primary Source**: The `COMMAND_REGISTRY` array in `daemon/src/mainCommand.cpp` provides a complete list of all registered commands and their required arguments. For example:
        ```cpp
        const CommandSignature COMMAND_REGISTRY[] = {
            // ...
            CommandSignature(COMMAND_UPSERT_ENTRY, {COMMAND_ARG_KEY, COMMAND_ARG_VALUE}),
            CommandSignature(COMMAND_GET_ENTRY, {COMMAND_ARG_KEY}),
            // ...
        };
        ```
    *   **Command Definitions**: The string literals for commands (e.g., `COMMAND_UPSERT_ENTRY`) are defined as macros in `daemon/include/common.h`. This file is useful for cross-referencing macro names with their string values.

    By consulting these files, you can construct valid JSON commands to send to the daemon. For instance, to use `upsertEntry`, you know you must provide a `key` and a `value`.

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
*   **Shared Extension Modules**: For GNOME Shell extensions, common functionality (like logging and daemon communication) is centralized in shared modules within `gnomeExtensions/lib/` to promote code reuse and simplify maintenance.
*   **Environment Variables**: Extensive use of `AUTOMATE_LINUX_` prefixed environment variables for managing paths, configurations, and inter-script communication.
*   **Daemon-Client Architecture**: A clear client-server model for system services, leveraging UNIX sockets for efficient and secure inter-process communication.
*   **Input Handling**: A significant focus on intercepting and manipulating input events, utilizing both the C++ daemon's input device discovery and `evsieve` for event remapping.
*   **Systemd Integration**: The C++ daemon is managed as a `systemd` service, ensuring it starts automatically and is properly managed by the operating system.
*   **External Dependencies**: The C++ daemon relies on `SQLite3` for data storage and `systemd` for service management. `evsieve` is an external tool used for input event processing.


in case we want to add a command to daemon instructions on how to add a command are provided in /home/yaniv/coding/automateLinux/daemon/AGENTS.md