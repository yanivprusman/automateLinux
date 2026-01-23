#!/bin/bash
LOG_DIR="${AUTOMATE_LINUX_DATA_DIR:-/home/yaniv/coding/automateLinux/data}"
mkdir -p "$LOG_DIR"
exec 1>"$LOG_DIR/restart_loom_debug.log" 2>&1
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
if [ -z "$SERVER_PORT" ]; then SERVER_PORT=$([ "$MODE" = "prod" ] && echo 3500 || echo 3505); fi

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
# Set service names
SERVER_UNIT="loom-server-$MODE"
CLIENT_UNIT="loom-client-$MODE"

# Cleanup
echo "Stopping existing systemd units for $MODE..."
systemctl --user stop $SERVER_UNIT $CLIENT_UNIT 2>/dev/null || true
systemctl --user reset-failed $SERVER_UNIT $CLIENT_UNIT 2>/dev/null || true

# Kill any leftover processes by port
echo "Killing leftovers on port $SERVER_PORT and $CLIENT_PORT..."
fuser -k -9 $SERVER_PORT/tcp 2>/dev/null || true
fuser -k -9 $CLIENT_PORT/tcp 2>/dev/null || true

# Kill by name as backup
pkill -9 -f "loom-server $SERVER_PORT" || true
pkill -9 -f "vite --port $CLIENT_PORT" || true

# Wait for ports to clear
wait_for_port_release() {
    local port=$1
    local max_retries=20
    local retry=0
    
    echo "Waiting for port $port to clear..."
    while /usr/bin/lsof -i :$port >/dev/null 2>&1; do
        if [ $retry -ge $max_retries ]; then
            echo "ERROR: Port $port is still stuck after 10 seconds. Giving up."
            break
        fi
        
        # Aggressively kill after 2.5 seconds (5 retries)
        if [ $retry -ge 5 ]; then
             echo "Port $port still busy, attempting force kill..."
             fuser -k -9 $port/tcp 2>/dev/null || true
        fi
        
        sleep 0.5
        retry=$((retry+1))
    done
}

wait_for_port_release "$SERVER_PORT"
wait_for_port_release "$CLIENT_PORT"

# Create client .env (server reads LOOM_MODE from $ROOT_DIR/.env which is set manually)
echo "VITE_SERVER_PORT=$SERVER_PORT" > "$ROOT_DIR/client/.env"

# Start Server via systemd
echo "Starting Loom Server ($SERVER_UNIT)..."
systemctl --user restart "$SERVER_UNIT"

# Start Client via systemd
echo "Starting Loom Client ($CLIENT_UNIT)..."
systemctl --user restart "$CLIENT_UNIT"

echo "Sending wake-up nudge..."
/home/yaniv/coding/automateLinux/symlinks/daemon send simulateInput --type 1 --code 42 --value 1
sleep 0.1
/home/yaniv/coding/automateLinux/symlinks/daemon send simulateInput --type 1 --code 42 --value 0

echo "Done. Mode: $MODE, Server: $SERVER_PORT, Client: $CLIENT_PORT"
exit 0
