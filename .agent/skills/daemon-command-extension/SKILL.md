---
name: daemon-command-extension
description: Instructions for adding a new command to the AutomateLinux daemon.
---

# Adding a New Daemon Command

Follow these steps to extend the daemon's capabilities with a new command.

## 1. Define Constants
Open `daemon/include/Constants.h` and add definitions for your command and any unique arguments.

```cpp
#define COMMAND_MY_NEW_ACTION "myNewAction"
#define COMMAND_ARG_MY_PARAM "myParam"
```

## 2. Implement the Handler
In `daemon/src/mainCommand.cpp`, implement a handler function. The function must take a `const json &` and return a `CmdResult`.

```cpp
CmdResult handleMyNewAction(const json &command) {
    string param = command[COMMAND_ARG_MY_PARAM].get<string>();
    // ... implement logic ...
    return CmdResult(0, "Action successful: " + param + "\n");
}
```

## 3. Register the Command Signature
In `daemon/src/mainCommand.cpp`, locate the `COMMAND_REGISTRY` array and add your command's signature (name and required arguments).

```cpp
const CommandSignature COMMAND_REGISTRY[] = {
    // ...
    CommandSignature(COMMAND_MY_NEW_ACTION, {COMMAND_ARG_MY_PARAM}),
};
```

## 4. Register the Dispatcher Handler
In `daemon/src/mainCommand.cpp`, locate the `COMMAND_HANDLERS` array and map your command string to its handler function.

```cpp
static const CommandDispatch COMMAND_HANDLERS[] = {
    // ...
    {COMMAND_MY_NEW_ACTION, handleMyNewAction},
};
```

## 5. Build and Test
Rebuild the daemon and test your new command using the terminal `d` (or `daemon send`) command.

```bash
bd
d send myNewAction --myParam "hello"
```

> [!TIP]
> Use `CmdResult(0, ...)` for success and `CmdResult(1, ...)` for errors. The second argument is the response string sent back to the client.

## Alternative: Separate Command Files

For related groups of commands, you can create separate files (e.g., `cmdApp.cpp`, `cmdPeer.cpp`):

1. Create header `daemon/include/cmdMyFeature.h` with handler declarations
2. Create implementation `daemon/src/cmdMyFeature.cpp`
3. Include the header in `mainCommand.cpp`
4. Register commands in `COMMAND_REGISTRY` and `COMMAND_HANDLERS` as usual

Example structure:
```
daemon/
├── include/
│   ├── cmdApp.h          # App management declarations
│   └── cmdPeer.h         # Peer networking declarations
└── src/
    ├── cmdApp.cpp        # App lifecycle: start/stop/restart/build
    ├── cmdPeer.cpp       # Peer networking commands
    └── mainCommand.cpp   # Command dispatcher
```
