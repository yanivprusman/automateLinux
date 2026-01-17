#!/bin/bash
exec 1>/tmp/restart_loom_debug.log 2>&1
echo "Starting restart_loom.sh at $(date)"

# Fix Environment for Loom Server (Wayland/Portal/DBus)
export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/1000/bus"
export XDG_RUNTIME_DIR="/run/user/1000"
export WAYLAND_DISPLAY="wayland-0"
export XDG_SESSION_TYPE="wayland"
export XDG_CURRENT_DESKTOP="Unity"
export XDG_SESSION_DESKTOP="ubuntu"
export XDG_MENU_PREFIX="gnome-"

# Cleanup
echo "Stopping existing systemd units..."
systemctl --user stop loom-server loom-client-dev loom-autoselect 2>/dev/null || true
systemctl --user reset-failed loom-server loom-client-dev loom-autoselect 2>/dev/null || true

# Kill any leftover processes not managed by systemd (legacy)
echo "Killing leftovers..."
killall -9 loom-server 2>/dev/null || true
fuser -k -9 4100/tcp 2>/dev/null || true

# Wait for ports to clear
echo "Waiting for ports to clear..."
sleep 3

echo "Debug: Checking for processes holding port 4100:"
ss -tulpn | grep :4100 || echo "Port 4100 is free."
ps aux | grep loom-server | grep -v grep || echo "No loom-server process found."

# Reload daemon to pick up any service file changes
systemctl --user daemon-reload

# Force fresh session by deleting restore token
# This ensures the "Share Screen" popup appears, which the automation script handles.
rm -f /home/yaniv/.config/loom/restore_token

# Start Server
echo "Starting server unit..."
systemctl --user start loom-server
echo "Server started."

# Launch Automation Script (to handle screen share popup)
# Must run in user session to interact with windows.
echo "Launching automation script..."
systemd-run --user --unit=loom-autoselect \
    --setenv=DISPLAY=":0" \
    --setenv=XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
    /usr/bin/python3 /home/yaniv/coding/automateLinux/utilities/autoSelectLoomScreen.py > /tmp/loom_autoselect.log 2>&1

# Start Client
echo "Starting client unit..."
systemctl --user start loom-client-dev
echo "Client started."

echo "Done."
exit 0
