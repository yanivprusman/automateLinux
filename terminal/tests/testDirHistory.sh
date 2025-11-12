
#!/bin/bash
# ls > "${AUTOMATE_LINUX_DIR_HISTORY_TESTS_DIR}output.txt"
exec > "${AUTOMATE_LINUX_DIR_HISTORY_TESTS_DIR}terminaloutput1.txt" 2>&1
set -x
rm "${AUTOMATE_LINUX_DIR_HISTORY_DIR}"* #2>/dev/null
cd ~/
cd coding/automateLinux/terminal/tests
cd ..
cd ..
${AUTOMATE_LINUX_DIR_HISTORY_TESTS_DIR}printDirHistory.sh 


bash <<'EOF'
exec > "${AUTOMATE_LINUX_DIR_HISTORY_TESTS_DIR}terminaloutput2.txt" 2>&1
set -x
rm "${AUTOMATE_LINUX_DIR_HISTORY_DIR}"* #2>/dev/null
cd ~/
cd coding/automateLinux/terminal/tests
cd ..
cd ..
${AUTOMATE_LINUX_DIR_HISTORY_TESTS_DIR}printDirHistory.sh 
EOF

# gnome-terminal --tab  