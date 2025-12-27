#!/bin/bash
# Wrapper script for native host to debug startup and ensure environment
LOG=/tmp/chrome-native-wrapper.log
echo "[$(date)] Wrapper started" >> $LOG
echo "PWD: $PWD" >> $LOG
echo "PATH: $PATH" >> $LOG

# Use absolute path to python3
PYTHON_EXEC=/usr/bin/python3
SCRIPT=/home/yaniv/coding/automateLinux/chromeExtension/native-host.py

if [ -x "$PYTHON_EXEC" ]; then
    echo "Executing python..." >> $LOG
    exec "$PYTHON_EXEC" "$SCRIPT" "$@"
else
    echo "Python executable not found at $PYTHON_EXEC" >> $LOG
    exit 1
fi
