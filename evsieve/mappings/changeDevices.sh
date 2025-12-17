#!/bin/bash

# Fetch keyboard path from daemon
KEYBOARD_BY_ID=$(daemon send getKeyboardPath)

# Check if KEYBOARD_BY_ID was successfully retrieved
if [ -z "$KEYBOARD_BY_ID" ]; then
    echo "Error: Could not retrieve keyboard path from daemon. Is the daemon running?" >&2
    exit 1
fi

export keyboardPath="$KEYBOARD_BY_ID"

case "$1" in
    terminal)
        script_to_source="corsairKeyBoardLogiMousegnome-terminal-server.sh"
        ;;
    chrome)
        script_to_source="corsairKeyBoardLogiMousegoogle-chrome.sh"
        ;;
    code)
        script_to_source="corsairKeyBoardLogiMouseCode.sh"
        ;;
    default)
        script_to_source="corsairKeyBoardLogiMouseDefaultKeyboard.sh"
        ;;
    *)
        echo "Usage: $0 {terminal|chrome|code|default}" >&2
        exit 1
        ;;
esac

script_path="/home/yaniv/coding/automateLinux/evsieve/mappings/$script_to_source"

if [ -f "$script_path" ]; then
    echo "Sourcing: $script_path with keyboardPath=$keyboardPath"
    source "$script_path"
else
    echo "Error: Script not found: $script_path" >&2
    exit 1
fi
