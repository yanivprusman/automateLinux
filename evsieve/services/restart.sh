#!/usr/bin/env bash 
sudo $(getRealPath _sudoStop.sh)
sudo $(getRealPath run.sh) "$@"

