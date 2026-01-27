# GNOME Extensions

A collection of GNOME Shell extensions that integrate the AutomateLinux suite directly into the desktop environment.

## Included Extensions

- **[clock@ya-niv.com](file:///opt/automateLinux/gnomeExtensions/clock@ya-niv.com)**: The primary UI extension. Adds a status menu to the top panel for toggling logging categories, managing macros, and monitoring daemon state.
- **[active-window-tracker@example.com](file:///opt/automateLinux/gnomeExtensions/active-window-tracker@example.com)**: Tracks the currently focused window and notifies the daemon of context changes (e.g., switching between Chrome and VS Code).

## Common Library

- **[lib](file:///opt/automateLinux/gnomeExtensions/lib)**: Shared logic for communicating with the daemon via Unix Domain Sockets from GJS (GNOME JavaScript).
