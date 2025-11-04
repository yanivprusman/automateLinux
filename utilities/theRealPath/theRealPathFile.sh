#!/bin/bash
if [ -z "$BASH_VERSION" ]; then
    echo "This script requires bash" >&2
    return 1 2>/dev/null || exit 1
fi
theRealPathFunction() {
    local GREEN='\033[0;32m'
    local NC='\033[0m' # No Color
    echo -e "${GREEN}result:${NC}" "\$1 is: $1"
    realpath "$1"
    sizeofBashSource=${#BASH_SOURCE[@]}
    echo -e "${GREEN}size of BASH_SOURCE array:${NC} $sizeofBashSource"
    for (( i=0; i<sizeofBashSource; i++ )); do
        echo -e "${GREEN}BASH_SOURCE[$i]:${NC} ${BASH_SOURCE[$i]}"
    done
}
