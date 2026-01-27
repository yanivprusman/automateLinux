# Utilities

A collection of standalone tools and helper scripts that support the AutomateLinux ecosystem. These are designed to be used independently or called by the daemon and extensions.

## Key Utilities

- **[termcontrol](file:///opt/automateLinux/utilities/termcontrol)**: A C++ utility for controlling terminal window properties and querying state.
- **[sendKeysUInput](file:///opt/automateLinux/utilities/sendKeysUInput)**: Simulates keyboard input by writing directly to `/dev/uinput`. Used for triggering macros from scripts.
- **[lastChanged](file:///opt/automateLinux/utilities/lastChanged)**: Efficiently monitors file changes across the filesystem.
- **[tailWindow](file:///opt/automateLinux/utilities/tailWindow)**: Monitors active window transitions and logs them to the console.
- **[cleanBetween](file:///opt/automateLinux/utilities/cleanBetween)**: Text processing utility to extract or remove content between specific patterns.

## Other Scripts

- `emergencyRestore.sh`: A safety script to ungrab devices and restore keyboard/mouse control if the daemon crashes or hangs.
