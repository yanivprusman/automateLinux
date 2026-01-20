#!/bin/bash
exec 1>/tmp/stop_loom_debug.log 2>&1
echo "Starting stop_loom.sh at $(date)"

# Parse arguments: --prod, --dev, or --all (default)
MODE="all"
for arg in "$@"; do
    case $arg in
        --dev) MODE="dev" ;;
        --prod) MODE="prod" ;;
        --all) MODE="all" ;;
    esac
done
echo "Mode: $MODE"

export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/1000/bus"
export XDG_RUNTIME_DIR="/run/user/1000"

# Stop systemd services based on mode
echo "Stopping systemd services..."
if [ "$MODE" = "prod" ] || [ "$MODE" = "all" ]; then
    systemctl --user stop loom-server-prod loom-client-prod 2>/dev/null || true
fi
if [ "$MODE" = "dev" ] || [ "$MODE" = "all" ]; then
    systemctl --user stop loom-server-dev loom-client-dev 2>/dev/null || true
fi

# Kill by process name (in case manual start)
if [ "$MODE" = "all" ]; then
    echo "Killing by process name..."
    killall loom-server 2>/dev/null || true
fi

# Kill by ports based on mode
echo "Killing processes on ports..."
if [ "$MODE" = "prod" ] || [ "$MODE" = "all" ]; then
    fuser -k 3004/tcp 3500/tcp 2>/dev/null || true
fi
if [ "$MODE" = "dev" ] || [ "$MODE" = "all" ]; then
    fuser -k 3005/tcp 3501/tcp 2>/dev/null || true
fi

echo "Done stopping loom ($MODE)."
exit 0
