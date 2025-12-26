#!/bin/bash

# Emergency Restore Script for automateLinux
# Run this if your keyboard or mouse becomes unresponsive.

echo "Initiating emergency restore..."

# 1. Stop the systemd service
echo "Stopping automateLinux daemon service..."
systemctl --user stop daemon.service 2>/dev/null

# 2. Kill any lingering daemon or evsieve processes
echo "Killing lingering processes..."
pkill -9 -f "daemon daemon"
pkill -9 -f "evsieve"

# 3. Small delay to allow kernel to release devices
sleep 0.5

# 4. Final check
if pgrep -f "daemon daemon" > /dev/null || pgrep -f "evsieve" > /dev/null; then
    echo "Warning: Some processes could not be killed. You might need sudo."
else
    echo "Success! Control should be restored."
fi

echo "Tip: If things are still frozen, try Ctrl+Alt+F1 then Ctrl+Alt+F2 to reset input focus."
