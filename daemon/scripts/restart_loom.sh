#!/bin/bash
exec 1>/tmp/restart_loom_debug.log 2>&1
echo "Starting restart_loom.sh at $(date)"

# Fix Environment for Loom Server (Wayland/Portal/DBus)
export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/1000/bus"
export XDG_RUNTIME_DIR="/run/user/1000"
export WAYLAND_DISPLAY="wayland-0"
export XDG_SESSION_TYPE="wayland"

# Cleanup
echo "Killing existing processes..."
killall loom-server 2>/dev/null || true
fuser -k 4000/tcp 4001/tcp 4002/tcp 4003/tcp 4004/tcp 4005/tcp 4100/tcp 2>/dev/null || true

# Start Server
echo "Starting server..."
cd /home/yaniv/coding/loom/server/build || { echo "Server dir not found"; exit 1; }
nohup ./loom-server 4100 > /tmp/loom-server.log 2>&1 &
SERVER_PID=$!
echo "Server started with PID $SERVER_PID"

# Wait a bit
sleep 1

# Start Client
echo "Starting client..."
cd /home/yaniv/coding/loom/client || { echo "Client dir not found"; exit 1; }
nohup npm run dev > /tmp/loom-client.log 2>&1 &
CLIENT_PID=$!
echo "Client started with PID $CLIENT_PID"

echo "Done."
exit 0
