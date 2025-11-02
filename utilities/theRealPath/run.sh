GREEN='\033[0;32m'
NC='\033[0m'
testFileSymlink=`which restartCorsairKeyBoardLogiMouseService.sh`
testFile=`readlink -f $testFileSymlink`
echo -e "${GREEN}testing sourcing theRealPath and calling theRealPath:"
echo -e the test file symlink: ${NC}${testFileSymlink}${GREEN}
echo -e the test file: ${NC}${testFile}
echo '''. theRealPath
theRealPath ./
theRealPath ../../asdf ''' > $testFile
cat $testFile
echo -e "${GREEN}calling: ${NC}$testFile"
restartCorsairKeyBoardLogiMouseService.sh | sed $'s/^/\t/'