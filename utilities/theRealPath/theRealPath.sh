#!/bin/bash
if [ -z "$BASH_VERSION" ]; then
    echo "This script requires bash" >&2
    return 1 2>/dev/null || exit 1
fi
# can be sourced or subprocessed or run as a function?
# prove that if not sourced the call in this sctipt executes: print debug
# if theRealPath has been sourced already will call the function theRealPath
# if is b can call theRealPath in this script or in the calling script
theRealPath() {
    local GREEN='\033[0;32m'
    local NC='\033[0m' # No Color
    echo -e "${GREEN}result:${NC}" "\$1 is: $1"
    realpath "$(dirname "$(realpath "${BASH_SOURCE[-1]}")")/$1"
    sizeofBashSource=${#BASH_SOURCE[@]}
    echo -e "${GREEN}size of BASH_SOURCE array:${NC} $sizeofBashSource"
    for (( i=0; i<sizeofBashSource; i++ )); do
        echo -e "${GREEN}BASH_SOURCE[$i]:${NC} ${BASH_SOURCE[$i]}"
    done
}
STYLE='\033[4;32m'
echo -e "${STYLE}sourced or subprocessed${NC}"
theRealPath "$@"
# echo -e "${ORANGE}after sourcing or executing theRealPath.sh ${NC}"

