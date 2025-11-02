#!/bin/bash
if [ -z "$BASH_VERSION" ]; then
    echo "This script requires bash" >&2
    return 1 2>/dev/null || exit 1
fi
# prove that if not sourced the call in this sctipt executes: print debug
# if theRealPath has been sourced already will call the function theRealPath
# if is b can call theRealPath in this script or in the calling script
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
# can be sourced or subprocessed,
echo B4 sourcing or executing theRealPath.sh 
theRealPath "$@"
echo after sourcing or executing theRealPath.sh 

