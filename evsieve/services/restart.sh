#!/usr/bin/env bash 
sudo $(runScriptRelative _sudoStop.sh)
sudo $(runScriptRelative run.sh) "$@"

