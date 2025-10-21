#!/usr/bin/env bash
ORIG_DIR="$(pwd)"
cd "$(dirname "${BASH_SOURCE[0]}")"
../mappings/logiMouse.sh
../mappings/corsairKeyboard.sh
cd "$ORIG_DIR"
