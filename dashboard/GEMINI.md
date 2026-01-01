# Dashboard

A modern web application built with React and Vite that provides a user-friendly interface for managing the AutomateLinux suite.

## Features

- **Live Logs**: Real-time visualization of system and input events.
- **Log Filtering**: Granular control over which event categories are displayed.
- **Macro Builder**: (In development) Visual interface for creating and managing input macros.
- **System Configuration**: Toggling various daemon features like keyboard/mouse grabbing.

## Tech Stack

- **Framework**: [React](https://react.dev/)
- **Build Tool**: [Vite](https://vitejs.dev/)
- **Language**: [TypeScript](https://www.typescriptlang.org/)
- **State Management**: React Hooks.

## Key Files

- **[src/components](file:///home/yaniv/coding/automateLinux/dashboard/src/components)**: Contains all the UI components, categorized by feature area.
- **bridge.cjs**: A helper script (likely for development or IPC bridge).
- **vite.config.ts**: Configuration for the Vite build system.
