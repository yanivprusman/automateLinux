firstTime(){
    trap ". $AUTOMATE_LINUX_TRAP_ERR_FILE" ERR
    trap ". $AUTOMATE_LINUX_TRAP_EXIT_FILE" EXIT
    trap ". $AUTOMATE_LINUX_TRAP_HUP_FILE" HUP
    # Get last directory from daemon with timeout, fallback to home
    local _ft_dir=$(timeout 2 daemon send openedTty --tty $AUTOMATE_LINUX_TTY_NUMBER 2>/dev/null)
    if [ -n "$_ft_dir" ] && [ -d "$_ft_dir" ]; then
        cd "$_ft_dir"
    fi
    if command -v mesg >/dev/null; then
        mesg y
    fi
    complete -F _to_completions to
    source "$AUTOMATE_LINUX_TERMINAL_FUNCTIONS_DIR/openFunctionCompletion.sh"
    complete -F _openFunction_completions openFunction
}
firstTime
unset firstTime