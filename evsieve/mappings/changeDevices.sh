#!/bin/bash
export keyboardPath=$(daemon send getKeyboardPath)
export mousePath=$(daemon send getMousePath)
if [ -z "$keyboardPath" ]; then
    echo "Error: Could not retrieve keyboard path from daemon. Is the daemon running?" >&2
    exit 1
fi
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
