#!/bin/bash

# This script must be sourced from bash
if [ -z "$BASH_VERSION" ]; then
    echo "This script requires bash" >&2
    return 1 2>/dev/null || exit 1
fi

# Function to resolve path relative to calling script
theRealPath() {
    if [ "${BASH_SOURCE[1]}" ]; then
        # Called from another script
        realpath "$(dirname "$(realpath "${BASH_SOURCE[1]}")")/$1"
    else
        # Called from current directory
        realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/$1"
    fi
}
