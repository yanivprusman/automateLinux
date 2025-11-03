GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'
testFileSymlink=`which restartCorsairKeyBoardLogiMouseService.sh`
testFile=`readlink -f $testFileSymlink`
echo -e "${GREEN}testing theRealPath"
echo -e "contents of theRealPath.sh:${NC}"
# echo '''#!/bin/bash
# if [ -z "$BASH_VERSION" ]; then
#     echo "This script requires bash" >&2
#     return 1 2>/dev/null || exit 1
# fi
# # prove that if not sourced the call in this sctipt executes: print debug
# # if theRealPath has been sourced already will call the function theRealPath
# # if is b can call theRealPath in this script or in the calling script
# theRealPath() {
#     local GREEN='\033[0;32m'
#     local NC='\033[0m' # No Color
#     echo -e "${GREEN}result:${NC}" "\$1 is: $1"
#     realpath "$(dirname "$(realpath "${BASH_SOURCE[-1]}")")/$1"
#     sizeofBashSource=${#BASH_SOURCE[@]}
#     echo -e "${GREEN}size of BASH_SOURCE array:${NC} $sizeofBashSource"
#     for (( i=0; i<sizeofBashSource; i++ )); do
#         echo -e "${GREEN}BASH_SOURCE[$i]:${NC} ${BASH_SOURCE[$i]}"
#     done
# }
# # can be sourced or subprocessed,
# ORANGE='\033[1m'
# # ORANGE='\033[38;5;208m' 
# # ORANGE='\033[1;38;5;208;48;5;234m'
# echo -e "${ORANGE}B4 sourcing or executing theRealPath.sh \$1 is: $1"
# theRealPath "$@"
# echo -e "${ORANGE}after sourcing or executing theRealPath.sh ${NC}"
# ''' | sed $'s/^/\t/'
echo -e "${GREEN}testing sourcing theRealPath and calling theRealPath:"
echo -e "the symlink to test file: ${NC}${testFileSymlink}${GREEN}"
echo -e "the test file: ${NC}${testFile}"
echo -e "${GREEN}contents of the test file:${NC}"
echo '''. theRealPath asdf
theRealPath ./asdfg
theRealPath ../../asdfgh ''' | tee $testFile | sed $'s/^/\t/'
echo -e "${GREEN}calling: the symlink${NC}"
restartCorsairKeyBoardLogiMouseService.sh | sed $'s/^/\t/'
echo -e "${YELLOW}----------------------------------------${NC}"
echo -e "${GREEN}testing calling theRealPath directly without sourcing:${NC}"
echo -e "${GREEN}contents of the test file:${NC}"
echo '''theRealPath asdf
theRealPath ./asdfg
theRealPath ../../asdfgh ''' | tee $testFile | sed $'s/^/\t/'
echo -e "${GREEN}calling: the symlink${NC}"
restartCorsairKeyBoardLogiMouseService.sh | sed $'s/^/\t/'
