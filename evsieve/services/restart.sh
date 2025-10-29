#!/usr/bin/env bash 
sudo $(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/_sudoStop.sh")
sudo $(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/run.sh") "$@"

