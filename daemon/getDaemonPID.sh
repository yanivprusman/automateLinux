#!/bin/bash

# 1. Try to get PID from systemd service if active
if systemctl is-active --quiet automateLinux.service; then
    pid=$(systemctl show --property MainPID --value automateLinux.service)
    if [ "$pid" != "0" ] && [ -n "$pid" ]; then
        echo "$pid"
        exit 0
    fi
fi

# 2. Fallback to pgrep (return only the most recent)
pid=$(pgrep -f "/home/yaniv/coding/automateLinux/daemon/daemon" | tail -n 1)

if [ -z "$pid" ]; then
    # Do not echo text if we want to avoid "unable to parse" errors in VS Code
    # But if called manually, helpful to know. 
    # Let's echo to stderr instead if missing.
    >&2 echo "Daemon is not running"
    exit 1
fi

echo "$pid"
