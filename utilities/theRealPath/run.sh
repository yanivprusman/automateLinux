GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'
testFileSymlink=`which restartCorsairKeyBoardLogiMouseService.sh`
testFile=`readlink -f $testFileSymlink`
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
