#!/bin/bash
exec 1>/tmp/stop_loom_debug.log 2>&1
echo "Starting stop_loom.sh at $(date)"

# 1. Stop systemd services
echo "Stopping systemd services..."
export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/1000/bus"
export XDG_RUNTIME_DIR="/run/user/1000"
systemctl --user stop loom-server loom-client-dev loom-client-prod 2>/dev/null || true

# 2. Kill by process name (in case manual start)
echo "Killing by process name..."
killall loom-server 2>/dev/null || true

# 3. Kill by ports (force cleanup)
echo "Killing processes on ports..."
fuser -k 3004/tcp 3005/tcp 4000/tcp 4001/tcp 4002/tcp 4003/tcp 4004/tcp 4005/tcp 4100/tcp 2>/dev/null || true

echo "Done stopping loom."
exit 0
