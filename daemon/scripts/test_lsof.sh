#!/bin/bash
/usr/bin/lsof -i :"$1" 2>&1 > "${AUTOMATE_LINUX_DIR:-/opt/automateLinux}/daemon/scripts/lsof_output.log"