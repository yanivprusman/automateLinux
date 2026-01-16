---
name: daemon-port-management
description: Instructions for managing application ports with the AutomateLinux daemon.
---

# Port Management

The AutomateLinux daemon acts as a central registry for application ports, allowing cross-component service discovery.

## CLI Usage

Use the high-level `d` command (which aliases `daemon send`) to manage ports.

### Setting a Port
Registers a port for a specific shorthand key (e.g., `pt` for Public Transportation).

```bash
d setPort --key pt --value 3001
```

### Getting a Port
Retrieves the currently registered port for a specific key.

```bash
d getPort --key pt
```

### Listing All Ports
Displays a list of all registered applications and their assigned ports.

```bash
d listPorts
```

## How it Works

### Persistence
The daemon stores these ports in the `settings` table of its MySQL database. The keys are automatically prefixed with `port_` (e.g., `port_pt`).

### Integration
Component logic (like in `mainCommand.cpp` or `InputMapper.cpp`) can retrieve these values using the `SettingsTable` manager:

```cpp
string port = SettingsTable::getSetting("port_pt");
if (!port.empty()) {
    string url = "http://localhost:" + port;
}
```

## Common Ports
| Key | Default Port | Description |
| :--- | :--- | :--- |
| `pt` | 3001 | Public Transportation Dashboard |
| `dash` | 9223 | AutomateLinux Core Dashboard |

> [!NOTE]
> Port management is useful for macros that need to launch browsers or proxy services to specific local addresses without hardcoding them in the binary.
