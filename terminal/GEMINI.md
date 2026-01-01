# Terminal

A suite of Bash enhancements and automation scripts that integrate with the AutomateLinux daemon to provide a smarter, more efficient terminal experience.

## Key Features

- **Shared History**: Navigating directory history across all open TTYs using `Ctrl+Up` and `Ctrl+Down`.
- **Dynamic Prompt**: A custom `PS1` that provides context-aware information.
- **Smart Aliases**: Productivity-boosting aliases for common developer tasks.
- **Modular Functions**: A library of reusable shell functions for various automation needs.

## Key Files

- **[bashrc](file:///home/yaniv/coding/automateLinux/terminal/bashrc)**: The entry point for the terminal suite. Sources all other components.
- **[bindings.sh](file:///home/yaniv/coding/automateLinux/terminal/bindings.sh)**: Defines global key bindings for directory navigation and other macros.
- **[aliases.sh](file:///home/yaniv/coding/automateLinux/terminal/aliases.sh)**: Project-specific and general productivity aliases.
- **[ps1.sh](file:///home/yaniv/coding/automateLinux/terminal/ps1.sh)**: Implementation of the custom prompt.

## Directory Structure

- **[functions](file:///home/yaniv/coding/automateLinux/terminal/functions)**: Modular bash functions.
- **[completions](file:///home/yaniv/coding/automateLinux/terminal/completions)**: Tab-completion scripts for custom tools.
- **[tests](file:///home/yaniv/coding/automateLinux/terminal/tests)**: Automated tests for terminal functionality.
