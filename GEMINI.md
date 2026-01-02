# AutomateLinux

This repository contains a comprehensive suite of tools designed to enhance the Linux user experience through automation, input mapping, and cross-application state synchronization.

## Project Structure

- **[daemon](file:///home/yaniv/coding/automateLinux/daemon)**: The core C++ service that manages input events, MySQL state, and command dispatching. **Note**: When building the daemon, use the `bd` bash function instead of `./build.sh`.
- **[terminal](file:///home/yaniv/coding/automateLinux/terminal)**: A collection of Bash enhancements including shared history, smart directory navigation, and custom bindings.
- **[dashboard](file:///home/yaniv/coding/automateLinux/dashboard)**: A modern web interface (React/Vite) for monitoring logs and configuring system behaviors.
- **[chromeExtension](file:///home/yaniv/coding/automateLinux/chromeExtension)**: Synchronizes browser state (like active tabs) with the daemon.
- **[gnomeExtensions](file:///home/yaniv/coding/automateLinux/gnomeExtensions)**: Integrates the automation suite directly into the GNOME desktop environment.
- **[utilities](file:///home/yaniv/coding/automateLinux/utilities)**: Standalone tools for specific tasks like uinput simulation and log processing.

## Key Concepts

1. **Native Messaging**: High-performance communication between the browser and the local daemon.
2. **Input Mapping**: Intercepting and transforming hardware input events (keyboard/mouse) based on application context.
3. **Shared History**: A global directory navigation sequence shared across all terminal sessions.
