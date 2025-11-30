readarray -t varsUntilMyBashrc <<< "$(compgen -v)"
myBashSourceFiles=(
    "${AUTOMATE_LINUX_TERMINAL_DIR}colors.sh"
    "${AUTOMATE_LINUX_TERMINAL_DIR}aliases.sh"
    "${AUTOMATE_LINUX_TERMINAL_FUNCTIONS_DIR}functions.sh"
    "${AUTOMATE_LINUX_TERMINAL_DIR}bindings.sh"
    "${AUTOMATE_LINUX_TERMINAL_DIR}exports.sh"
    "${AUTOMATE_LINUX_TERMINAL_DIR}gSettings.sh"
    "${AUTOMATE_LINUX_TERMINAL_DIR}/completions/completion.sh"
)
for file in "${myBashSourceFiles[@]}"; do
    . "$file"
done
touchDirectories
trap ". $AUTOMATE_LINUX_TRAP_ERR_FILE" ERR
set -E 
# if [[ ! -v AUTOMATE_LINUX_SUBSEQUENT_SOURCE ]]; then :
#     # daemon ttyOpened
#     # daemon cdToPointer
#     AUTOMATE_LINUX_SUBSEQUENT_SOURCE=true
#     # initCoproc
#     # AUTOMATE_LINUX_FD=$(firstFreeFd)
#     # exec {AUTOMATE_LINUX_FD}<> >(socat UNIX-CONNECT:"$AUTOMATE_LINUX_SOCKET_PATH" -)
#     exec {AUTOMATE_LINUX_FD}<>"$AUTOMATE_LINUX_SOCKET_PATH" 
#     echo in bashrc: AUTOMATE_LINUX_FD=$AUTOMATE_LINUX_FD
# fi
if [[ ! -v AUTOMATE_LINUX_SUBSEQUENT_SOURCE ]]; then
    AUTOMATE_LINUX_SUBSEQUENT_SOURCE=true
    
    # Create persistent named pipes for daemon communication
    # Use a unique ID based on the current shell's PID
    AUTOMATE_LINUX_SHELL_PID=$$
    AUTOMATE_LINUX_FIFO_DIR="/tmp/automatelinux-fifos-$AUTOMATE_LINUX_SHELL_PID"
    mkdir -p "$AUTOMATE_LINUX_FIFO_DIR"
    AUTOMATE_LINUX_FIFO_IN="$AUTOMATE_LINUX_FIFO_DIR/in"
    AUTOMATE_LINUX_FIFO_OUT="$AUTOMATE_LINUX_FIFO_DIR/out"
    
    # Remove old FIFOs if they exist
    rm -f "$AUTOMATE_LINUX_FIFO_IN" "$AUTOMATE_LINUX_FIFO_OUT" 2>/dev/null
    
    mkfifo "$AUTOMATE_LINUX_FIFO_IN" "$AUTOMATE_LINUX_FIFO_OUT" 2>/dev/null
    
    # Start relay process
    (
        while true; do
            cat "$AUTOMATE_LINUX_FIFO_IN" | \
            timeout 3 nc -U "$AUTOMATE_LINUX_SOCKET_PATH" 2>/dev/null | \
            tee "$AUTOMATE_LINUX_FIFO_OUT" >/dev/null || break
        done
    ) &
    AUTOMATE_LINUX_RELAY_PID=$!
    
    # Give relay time to start
    sleep 0.1
    
    # Export all FIFO variables
    export AUTOMATE_LINUX_FIFO_IN AUTOMATE_LINUX_FIFO_OUT AUTOMATE_LINUX_RELAY_PID AUTOMATE_LINUX_FIFO_DIR
    
    trap "rm -rf '$AUTOMATE_LINUX_FIFO_DIR' 2>/dev/null; kill $AUTOMATE_LINUX_RELAY_PID 2>/dev/null" EXIT
fi
PS1='\[\e]0;'$AUTOMATE_LINUX_TTY_NUMBER'\w\a\]'"${_yellow}\w${_nc}\$ "
cp ~/coding/automateLinux/desktop/*.desktop ~/Desktop/
# runSingleton "$AUTOMATE_LINUX_SYMLINK_DIR/restartCorsairKeyBoardLogiMouseService.sh"
AUTOMATE_LINUX_SUBSEQUENT_SOURCE=true
PROMPT_COMMAND=". $AUTOMATE_LINUX_PROMPT_COMMAND_SCRIPT_FILE"
. "${AUTOMATE_LINUX_TERMINAL_DIR}unset.sh"

