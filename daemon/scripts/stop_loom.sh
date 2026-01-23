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

# Check for Primary User Configuration if running as root
PRIMARY_USER_FILE="${AUTOMATE_LINUX_DIR:-/opt/automateLinux}/config/primary_user.env"
if [ "$(id -u)" -eq 0 ] && [ -f "$PRIMARY_USER_FILE" ]; then
    TARGET_USER=$(cat "$PRIMARY_USER_FILE")
    TARGET_UID=$(id -u "$TARGET_USER")
    export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/$TARGET_UID/bus"
    export XDG_RUNTIME_DIR="/run/user/$TARGET_UID"
    echo "Running as root. Switching context to user: $TARGET_USER"
    run_as_user() { runuser -u "$TARGET_USER" -- "$@"; }
else
    TARGET_USER=$(whoami)
    echo "Running as user: $TARGET_USER"
    run_as_user() { "$@"; }
fi

# Stop systemd services based on mode
echo "Stopping systemd services..."
if [ "$MODE" = "prod" ] || [ "$MODE" = "all" ]; then
    run_as_user systemctl --user stop loom-server-prod loom-client-prod loom-autoselect-prod 2>/dev/null || true
    run_as_user systemctl --user reset-failed loom-server-prod loom-client-prod loom-autoselect-prod 2>/dev/null || true
fi
if [ "$MODE" = "dev" ] || [ "$MODE" = "all" ]; then
    run_as_user systemctl --user stop loom-server-dev loom-client-dev loom-autoselect-dev 2>/dev/null || true
    run_as_user systemctl --user reset-failed loom-server-dev loom-client-dev loom-autoselect-dev 2>/dev/null || true
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
