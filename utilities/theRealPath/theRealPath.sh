#!/bin/bash
if [ -z "$BASH_VERSION" ]; then
    echo "This script requires bash" >&2
    return 1 2>/dev/null || exit 1
fi
theRealPath() {
    echo result:
    realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/$1"
    echo calling script: 
    echo "${BASH_SOURCE[0]}"
}
theRealPath "$@"
