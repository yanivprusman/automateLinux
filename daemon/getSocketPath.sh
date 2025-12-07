#!/bin/bash

# # Return the socket path for the automateLinux daemon
# # SOCKET_PATH="/run/automatelinux/automatelinux-daemon.sock"
# # SOCKET_PATH="/run/automatelinux/automatelinux-daemon.sock"

# # Verify daemon is running
# if ! pgrep -f "/home/yaniv/coding/automateLinux/daemon/daemon" > /dev/null 2>&1; then
#     echo "Daemon is not running" >&2
#     exit 1
# fi

# # Verify socket exists
# if [ ! -S "$AUTOMATE_LINUX_SOCKET_PATH" ]; then
#     echo "Socket does not exist at $AUTOMATE_LINUX_SOCKET_PATH" >&2
#     exit 1
# fi

# echo "$AUTOMATE_LINUX_SOCKET_PATH"
echo /run/automatelinux/automatelinux-daemon.sock