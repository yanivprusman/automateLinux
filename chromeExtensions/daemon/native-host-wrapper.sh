#!/bin/bash
# Wrapper for native-host.py to debug startup issues
# Logs stdout/stderr to a file

LOG_FILE="/tmp/com.automatelinux.native-host-wrapper.log"
exec 2>> "$LOG_FILE"
echo "[$(date)] Starting native-host-wrapper.sh" >> "$LOG_FILE"

# Make sure we're in the right directory
cd "$(dirname "$0")"

# Run the python script and pass through stdin/stdout
# Note: we use -u for unbuffered output to avoid proxying issues
exec python3 -u ./native-host.py "$@"
