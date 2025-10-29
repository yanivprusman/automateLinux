#!/usr/bin/env bash

SCRIPT_DIR="$(dirname "$(realpath "${BASH_SOURCE[0]}")")"

sudo "$(realpath "${SCRIPT_DIR}/_sudoStop.sh")"
sudo "$(realpath "${SCRIPT_DIR}/run.sh")" "$@"

