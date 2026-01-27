#!/bin/bash

# Configuration
DEV_EXTENSION_DIR="/opt/automateLinux/visualStudioCodeExtensions/git"
VSCODE_EXTENSIONS_DIR="/home/yaniv/.vscode/extensions"
EXTENSION_ID="undefined_publisher.git-0.0.1"
TARGET_DIR="${VSCODE_EXTENSIONS_DIR}/${EXTENSION_ID}"

echo "[git-ext] Starting extension synchronization..."

# 1. Compile the current code
echo "[git-ext] Compiling development version..."
cd "$DEV_EXTENSION_DIR"
npm run compile

if [ $? -ne 0 ]; then
    echo "[error] Compilation failed. Aborting."
    exit 1
fi

# 2. Check if the target extension directory exists
if [ -d "$TARGET_DIR" ]; then
    if [ -L "$TARGET_DIR" ]; then
        echo "[git-ext] Symlink already exists at ${TARGET_DIR}. Updating content..."
    else
        echo "[git-ext] Removing existing static installation at ${TARGET_DIR}..."
        rm -rf "$TARGET_DIR"
    fi
fi

# 3. Create the symlink
if [ ! -d "$TARGET_DIR" ]; then
    echo "[git-ext] Creating symlink: ${TARGET_DIR} -> ${DEV_EXTENSION_DIR}"
    ln -s "$DEV_EXTENSION_DIR" "$TARGET_DIR"
fi

echo "[git-ext] Synchronization complete!"
echo "[git-ext] IMPORTANT: Please completely restart VS Code for changes to take effect."
