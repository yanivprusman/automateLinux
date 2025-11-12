green='\e[32m'
yellow='\e[33m'
NC='\e[0m' # No Color
echo -e "${green}testDirHistory.sh:${NC}"
cat testDirHistory.sh | awk 'NR >= 3'
heredoc testDirHistory.sh
./testDirHistory.hereDoc 
echo -e "${yellow}$AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR${NC}"
cat "${AUTOMATE_LINUX_DATA_DIR}terminaloutput1.txt"
