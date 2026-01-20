#!/bin/bash
/usr/bin/lsof -i :"$1" 2>&1 > /home/yaniv/coding/automateLinux/daemon/scripts/lsof_output.log