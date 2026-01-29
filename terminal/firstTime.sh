firstTime(){
    trap ". $AUTOMATE_LINUX_TRAP_ERR_FILE" ERR
    trap ". $AUTOMATE_LINUX_TRAP_EXIT_FILE" EXIT
    trap ". $AUTOMATE_LINUX_TRAP_HUP_FILE" HUP
    cd $(daemon send openedTty --tty $AUTOMATE_LINUX_TTY_NUMBER )
    if command -v mesg >/dev/null; then
        mesg y
    fi
    complete -F _to_completions to
    source "$AUTOMATE_LINUX_TERMINAL_FUNCTIONS_DIR/openFunctionCompletion.sh"
    complete -F _openFunction_completions openFunction
    initTerminalCapture
}
firstTime
unset firstTime