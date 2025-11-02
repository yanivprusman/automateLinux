#!/bin/bash
if [ -z "$BASH_VERSION" ]; then
    echo "This script requires bash" >&2
    return 1 2>/dev/null || exit 1
fi
theRealPath() {
    echo result:
    # realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/$1"
    realpath "$(dirname "$(realpath "${BASH_SOURCE[-1]}")")/$1"
    # echo calling script: 
    # echo "${BASH_SOURCE[0]}"
    sizeofBashSource=${#BASH_SOURCE[@]}
    echo size of BASH_SOURCE array: $sizeofBashSource
    for (( i=0; i<sizeofBashSource; i++ )); do
        echo BASH_SOURCE[$i]: ${BASH_SOURCE[$i]}
        
    done
}
# theRealPath "$@"
