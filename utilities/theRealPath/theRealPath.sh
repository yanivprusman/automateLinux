#!/bin/bash
if [ -z "$BASH_VERSION" ]; then
    echo "This script requires bash" >&2
    return 1 2>/dev/null || exit 1
fi
theRealPath() {
    if [ "${BASH_SOURCE[1]}" ]; then
        realpath "$(dirname "$(realpath "${BASH_SOURCE[1]}")")/$1"
    else
        realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/$1"
    fi
}
theRealPath "$@"
