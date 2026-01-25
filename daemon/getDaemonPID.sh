#!/bin/bash

# 1. Try to get PID from systemd service if active
if systemctl is-active --quiet daemon.service; then
    pid=$(systemctl show --property MainPID --value daemon.service)
    if [ "$pid" != "0" ] && [ -n "$pid" ]; then
        echo "$pid"
        exit 0
    fi
fi

# 2. Fallback to pgrep (return only the most recent)
# Use AUTOMATE_LINUX_DIR environment variable if set, otherwise try /opt/
SEARCH_PATH="${AUTOMATE_LINUX_DIR:-/opt/automateLinux}/daemon/daemon"
pid=$(pgrep -f "$SEARCH_PATH" | tail -n 1)

if [ -z "$pid" ]; then
    # Do not echo text if we want to avoid "unable to parse" errors in VS Code
    # But if called manually, helpful to know. 
    # Let's echo to stderr instead if missing.
    >&2 echo "Daemon is not running"
    exit 1
fi

echo "$pid"
