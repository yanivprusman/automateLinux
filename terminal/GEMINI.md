# Terminal

A suite of Bash enhancements and automation scripts that integrate with the AutomateLinux daemon to provide a smarter, more efficient terminal experience.

## Key Features

- **Shared History**: Navigating directory history across all open TTYs using `Ctrl+Up` and `Ctrl+Down`.
- **Dynamic Prompt**: A custom `PS1` that provides context-aware information.
- **Smart Aliases**: Productivity-boosting aliases for common developer tasks.
- **Modular Functions**: A library of reusable shell functions for various automation needs.

## Key Files

- **[bashrc](file:///opt/automateLinux/terminal/bashrc)**: The entry point for the terminal suite. Sources all other components.
- **[bindings.sh](file:///opt/automateLinux/terminal/bindings.sh)**: Defines global key bindings for directory navigation and other macros.
- **[aliases.sh](file:///opt/automateLinux/terminal/aliases.sh)**: Project-specific and general productivity aliases.
- **[ps1.sh](file:///opt/automateLinux/terminal/ps1.sh)**: Implementation of the custom prompt.

## Directory Structure

- **[functions](file:///opt/automateLinux/terminal/functions)**: Modular bash functions.
- **[completions](file:///opt/automateLinux/terminal/completions)**: Tab-completion scripts for custom tools.
- **[tests](file:///opt/automateLinux/terminal/tests)**: Automated tests for terminal functionality.
