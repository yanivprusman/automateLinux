#!/bin/bash
# Simple wrapper to save clock position to daemon
# Usage: ./saveClockPosition.sh X Y

X=$1
Y=$2

if [ -z "$X" ] || [ -z "$Y" ]; then
    echo "Usage: $0 X Y" >&2
    exit 1
fi

# SOCKET_PATH="/run/automatelinux/automatelinux-daemon.sock"
# echo "{\"command\":\"upsertEntry\", \"key\":\"clockPositionX\", \"value\":\"$X\"}" | nc -U -q 1 "$SOCKET_PATH"
# echo "{\"command\":\"upsertEntry\", \"key\":\"clockPositionY\", \"value\":\"$Y\"}" | nc -U -q 1 "$SOCKET_PATH"
# AUTOMATE_LINUX_SOCKET_PATH="/run/automatelinux/automatelinux-daemon.sock"
echo "{\"command\":\"upsertEntry\", \"key\":\"clockPositionX\", \"value\":\"$X\"}" | nc -U -q 1 "$AUTOMATE_LINUX_SOCKET_PATH"
echo "{\"command\":\"upsertEntry\", \"key\":\"clockPositionY\", \"value\":\"$Y\"}" | nc -U -q 1 "$AUTOMATE_LINUX_SOCKET_PATH"
