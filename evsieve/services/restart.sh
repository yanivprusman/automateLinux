#!/usr/bin/env bash 
sudo "$(dirname "${BASH_SOURCE[0]}")/_sudoStop.sh"
sudo "$(dirname "${BASH_SOURCE[0]}")/run.sh" "$@"

