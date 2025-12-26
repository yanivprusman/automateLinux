#!/bin/bash
# Safely stop the automateLinux daemon
# Finds the PID of the specific daemon binary and kills it

DAEMON_BIN="/home/yaniv/coding/automateLinux/daemon/daemon"
PIDS=$(pgrep -f "^$DAEMON_BIN")

if [ -z "$PIDS" ]; then
    echo "Daemon not running."
else
    echo "Stopping daemon (PIDs: $PIDS)..."
    kill $PIDS
    sleep 1
    # Check if still running and force kill if necessary
    PIDS=$(pgrep -f "^$DAEMON_BIN")
    if [ -n "$PIDS" ]; then
        echo "Force killing daemon..."
        kill -9 $PIDS
    fi
    echo "Daemon stopped."
fi
