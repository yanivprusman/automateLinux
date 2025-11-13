exec > "${AUTOMATE_LINUX_DATA_DIR}terminaloutput1.txt" 2>&1
green='\e[32m' yellow='\e[33m' NC='\e[0m' # No Color
set -x
rm "${AUTOMATE_LINUX_DIR_HISTORY_DIR}"* #2>/dev/null
cd ~/
cd coding/automateLinux/terminal/tests
cd ..
cd ..
# set +x
echo -e "${yellow}$AUTOMATE_LINUX_PRINT_BLOCK_SEPARATOR${NC}"
echo -e "${green}dir history folder:${NC}"
${AUTOMATE_LINUX_DIR_HISTORY_TESTS_DIR}printDirHistory.sh 
# echo -e "${green}executing promptCommand.sh:${NC}"
# . /home/yaniv/coding/automateLinux/terminal/promptCommand.sh
# echo -e "${green}dir history folder after promptCommand.sh:${NC}"
# ${AUTOMATE_LINUX_DIR_HISTORY_TESTS_DIR}printDirHistory.sh 

