#!/bin/bash
# Simple wrapper to load clock position from daemon
# Outputs X and Y on separate lines

SOCKET_PATH="/run/automatelinux/automatelinux-daemon.sock"

X=$(echo "{\"command\":\"getEntry\", \"key\":\"clockPositionX\"}" | nc -U -q 1 "$SOCKET_PATH" 2>/dev/null | head -1)
Y=$(echo "{\"command\":\"getEntry\", \"key\":\"clockPositionY\"}" | nc -U -q 1 "$SOCKET_PATH" 2>/dev/null | head -1)

echo "$X"
echo "$Y"
