daemon send updateDirHistory --tty $AUTOMATE_LINUX_TTY_NUMBER --pwd "${PWD}/" &> /dev/null
# Small delay to let tee flush output before writing prompt marker (avoids race condition)
[ -n "$AUTOMATE_LINUX_TERMINAL_CAPTURE_FILE" ] && { sleep 0.01; echo "---PROMPT[timestamp:$(date +%s)]---" >> "$AUTOMATE_LINUX_TERMINAL_CAPTURE_FILE"; }
