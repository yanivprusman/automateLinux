#!/bin/bash
echo "[$(date)] Wrapper executed with args: $@" >> /tmp/wrapper.log
exec /usr/bin/python3.12 /home/yaniv/coding/automateLinux/chromeExtension/native-host.py "$@"
