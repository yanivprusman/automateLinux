#!/bin/bash
exec 1>/tmp/restart_loom_debug.log 2>&1
echo "Starting restart_loom.sh at $(date)"

# Parse arguments: --prod (default) or --dev
MODE="prod"
for arg in "$@"; do
    case $arg in
        --dev) MODE="dev" ;;
        --prod) MODE="prod" ;;
    esac
done
echo "Mode: $MODE"

# Define Root Directory based on mode
if [ "$MODE" = "prod" ]; then
    ROOT_DIR="/home/yaniv/coding/prod/loom"
    PORT_KEY_CLIENT="loom-prod"
    PORT_KEY_SERVER="loom-server"
else
    ROOT_DIR="/home/yaniv/coding/automateLinux/extraApps/loom"
    PORT_KEY_CLIENT="loom-dev"
    PORT_KEY_SERVER="loom-server-dev"
fi
echo "Root Directory: $ROOT_DIR"

# Query Daemon for Ports
CLIENT_PORT=$(daemon send getPort --key "$PORT_KEY_CLIENT" | tr -d '\n\r ')
SERVER_PORT=$(daemon send getPort --key "$PORT_KEY_SERVER" | tr -d '\n\r ')

# Fallbacks if daemon query fails
if [ -z "$CLIENT_PORT" ]; then CLIENT_PORT=$([ "$MODE" = "prod" ] && echo 3004 || echo 3005); fi
if [ -z "$SERVER_PORT" ]; then SERVER_PORT=$([ "$MODE" = "prod" ] && echo 3500 || echo 3501); fi

echo "Using Client Port: $CLIENT_PORT, Server Port: $SERVER_PORT"

# Fix Environment for Loom Server (Wayland/Portal/DBus)
export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/1000/bus"
export XDG_RUNTIME_DIR="/run/user/1000"
export WAYLAND_DISPLAY="wayland-0"
export XDG_SESSION_TYPE="wayland"
export XDG_CURRENT_DESKTOP="Unity"
export XDG_SESSION_DESKTOP="ubuntu"
export XDG_MENU_PREFIX="gnome-"

# Set service names
SERVER_UNIT="loom-server-$MODE"
CLIENT_UNIT="loom-client-$MODE"
AUTO_UNIT="loom-autoselect-$MODE"

# Cleanup
echo "Stopping existing systemd units for $MODE..."
systemctl --user stop $SERVER_UNIT $CLIENT_UNIT $AUTO_UNIT 2>/dev/null || true
systemctl --user reset-failed $SERVER_UNIT $CLIENT_UNIT $AUTO_UNIT 2>/dev/null || true

# Kill any leftover processes by port
echo "Killing leftovers on port $SERVER_PORT and $CLIENT_PORT..."
fuser -k -9 $SERVER_PORT/tcp 2>/dev/null || true
fuser -k -9 $CLIENT_PORT/tcp 2>/dev/null || true

# Kill by name as backup
pkill -9 -f "loom-server $SERVER_PORT" || true
pkill -9 -f "vite --port $CLIENT_PORT" || true

# Wait for ports to clear
sleep 1

# Create .env files
echo "VITE_SERVER_PORT=$SERVER_PORT" > "$ROOT_DIR/client/.env"
echo "PORT=$SERVER_PORT" > "$ROOT_DIR/server/.env"

# Force fresh session by deleting restore token
# rm -f /home/yaniv/.config/loom/restore_token

# Start Server via systemd
echo "Starting Loom Server ($SERVER_UNIT)..."
systemctl --user restart "$SERVER_UNIT"

# Launch Automation Script (to handle screen share popup)
echo "Launching automation script ($AUTO_UNIT)..."
systemd-run --user --unit="$AUTO_UNIT" \
    --setenv=DISPLAY=":0" \
    --setenv=WAYLAND_DISPLAY="$WAYLAND_DISPLAY" \
    --setenv=XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" \
    --setenv=DBUS_SESSION_BUS_ADDRESS="$DBUS_SESSION_BUS_ADDRESS" \
    --setenv=XDG_SESSION_TYPE="$XDG_SESSION_TYPE" \
    --setenv=XDG_CURRENT_DESKTOP="$XDG_CURRENT_DESKTOP" \
    /usr/bin/python3 /home/yaniv/coding/automateLinux/utilities/autoSelectLoomScreen.py

# Start Client via systemd
echo "Starting Loom Client ($CLIENT_UNIT)..."
systemctl --user restart "$CLIENT_UNIT"

echo "Done. Mode: $MODE, Server: $SERVER_PORT, Client: $CLIENT_PORT"
exit 0
