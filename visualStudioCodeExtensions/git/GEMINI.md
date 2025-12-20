# Git VS Code Extension - Project Overview

This document provides an overview of the `git` VS Code extension project. This extension aims to enhance Git integration within VS Code by providing functionality related to file-specific commit history and simplified file checkout from previous commits.

## Project Overview

The `git` VS Code extension is a TypeScript-based project that integrates with Git to provide advanced features for managing individual file histories.

*   **Core Functionality**:
    *   **Active File Commits View**: Displays the commit history for the currently active file in a dedicated sidebar view.
    *   **Checkout File from Commit**: Allows users to checkout a specific version of the active file from a chosen commit.
    *   **Checkout File from Next Commit**: Provides quick navigation to checkout the active file from the next chronological commit after the currently checked out version.
*   **Technology Stack**:
    *   **TypeScript**: Primary language for development.
    *   **Node.js**: Runtime environment.
    *   **VS Code API**: Utilized for extension development, including tree views, commands, and text editor interactions.
    *   **Git CLI**: Executes Git commands via `child_process.exec` to interact with the repository.

## Building and Running

The extension uses `npm` for package management and `tsc` for TypeScript compilation.

### Prerequisites

*   Node.js and npm installed.
*   Git installed and configured.
*   VS Code with Extension Test Runner (for testing).

### Build

To compile the TypeScript source code into JavaScript:

```bash
npm run compile
```

This command will output the compiled JavaScript files to the `out/` directory.

### Run (Development Mode)

1.  Press `F5` in VS Code to open a new VS Code window with the extension loaded.
2.  Open the Command Palette (`Ctrl+Shift+P` or `Cmd+Shift+P` on Mac).
3.  The extension's commands will be available, and the "Git Extension" sidebar view will appear.

### Run Tests

The project includes tests for the extension's functionality.

1.  Ensure the "watch" task is running:
    ```bash
    npm run watch
    ```
2.  Open the Testing view in VS Code (from the activity bar).
3.  Click the "Run Test" button or use the hotkey `Ctrl/Cmd + ; A`.
    *   Alternatively, run the test script directly:
        ```bash
        npm test
        ```

## Development Conventions

*   **Language**: TypeScript is used for all source code.
*   **Linting**: ESLint is configured with `typescript-eslint` to enforce coding style and identify potential issues.
    *   To run linting: `npm run lint`
*   **Type Checking**: Strict type checking is enabled in `tsconfig.json`.
*   **Module System**: Node16 module resolution.
*   **Output Directory**: Compiled JavaScript files are output to the `out/` directory.
*   **Git Interaction**: Git commands are executed programmatically using Node.js's `child_process.exec`. Error handling and user feedback are provided for Git operations.
*   **VS Code API Usage**: Adheres to VS Code extension development best practices, using `vscode.commands.registerCommand`, `vscode.window.createTreeView`, `vscode.TreeDataProvider`, etc.
