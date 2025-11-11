exec > "${AUTOMATE_LINUX_DIR_HISTORY_TESTS_DIR}terminaloutput.txt" 2>&1
gnome-terminal --tab  
rm "${AUTOMATE_LINUX_DIR_HISTORY_DIR}"* #2>/dev/null
cd ~/
cd coding/automateLinux/terminal/tests
cd ..
cd ..
${AUTOMATE_LINUX_DIR_HISTORY_TESTS_DIR}printDirHistory.sh > "${AUTOMATE_LINUX_DIR_HISTORY_TESTS_DIR}output.txt"


