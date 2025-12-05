#!/bin/bash
pid=$(pgrep -f "/home/yaniv/coding/automateLinux/daemon/daemon")
if [ -z "$pid" ]; then
    echo "Daemon is not running"
    exit 1
fi
# echo -n $pid > /tmp/daemon.pid
echo $pid
