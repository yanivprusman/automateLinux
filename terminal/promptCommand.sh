daemon send updateDirHistory --tty $AUTOMATE_LINUX_TTY_NUMBER --pwd "${PWD}/" &> /dev/null
[ -n "$AUTOMATE_LINUX_TERMINAL_CAPTURE_FILE" ] && echo "---PROMPT[timestamp:$(date +%s)]---" >> "$AUTOMATE_LINUX_TERMINAL_CAPTURE_FILE"
