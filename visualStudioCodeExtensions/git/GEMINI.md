# GEMINI.md - Project Overview

This document provides an overview of the `git` VS Code extension project.

## Project Overview

The `git` VS Code extension aims to enhance Git integration within the Visual Studio Code editor. Its core functionality revolves around providing an intuitive way to view and interact with the commit history of the currently active file.

Key features include:
*   **Active File Commit View:** Displays the commit history for the file open in the active editor within a dedicated sidebar view.
*   **Checkout File from Commit:** Allows users to restore a specific version of the active file by selecting a commit from its history.
*   **Navigate Commit History:** Provides commands to easily move forward and backward through the commit history of the active file, facilitating quick inspection of changes over time. done with key bindings.

## Development Conventions

*   **Language:** TypeScript is used for all development, ensuring type safety and leveraging modern JavaScript features.
*   **VS Code API Usage:** The extension extensively uses the Visual Studio Code API for creating UI elements (views, commands) and integrating with editor events.
*   **Git CLI Interaction:** Git commands are executed directly via `child_process.exec` to interact with the system's Git installation.
*   **Project Structure:**
    *   `src/extension.ts`: Contains the main activation logic and the implementation of the extension's features.
    *   `package.json`: 

new goal:
lets refactor the extension. i want another mode of operatoin. one keep as is. the second mode has two commits, a fromCommit and a toCommit. the file will show both in the following way:
lines that are going to be removed from the fromCommit will have a comment at the end //remove. 
lines that will be added in the toCommit will have a comment at the end //add and will be commented out.
this way the file logic will be the same as the from commit and we will know from the comments what will be removed and what will be added.