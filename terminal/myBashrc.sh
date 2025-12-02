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
if [[ ! -v AUTOMATE_LINUX_SUBSEQUENT_SOURCE ]]; then
    
    AUTOMATE_LINUX_RELAY_DIR="/tmp/automatelinux-relay-$$"
    mkdir -p "$AUTOMATE_LINUX_RELAY_DIR"
    AUTOMATE_LINUX_DAEMON_IN="$AUTOMATE_LINUX_RELAY_DIR/in"
    AUTOMATE_LINUX_DAEMON_OUT="$AUTOMATE_LINUX_RELAY_DIR/out"
    rm -f "$AUTOMATE_LINUX_DAEMON_IN" "$AUTOMATE_LINUX_DAEMON_OUT" 2>/dev/null
    mkfifo "$AUTOMATE_LINUX_DAEMON_IN" "$AUTOMATE_LINUX_DAEMON_OUT" 2>/dev/null
    (
        stdbuf -o0 -i0 cat "$AUTOMATE_LINUX_DAEMON_IN" | \
        stdbuf -o0 -i0 nc -U "$AUTOMATE_LINUX_SOCKET_PATH" 2>/dev/null > "$AUTOMATE_LINUX_DAEMON_OUT"
    ) &
    AUTOMATE_LINUX_RELAY_PID=$!
    sleep 0.2
    exec {AUTOMATE_LINUX_DAEMON_IN_FD}>"$AUTOMATE_LINUX_DAEMON_IN"
    exec {AUTOMATE_LINUX_DAEMON_OUT_FD}<"$AUTOMATE_LINUX_DAEMON_OUT"
    export AUTOMATE_LINUX_DAEMON_IN_FD AUTOMATE_LINUX_DAEMON_OUT_FD
    export AUTOMATE_LINUX_RELAY_PID AUTOMATE_LINUX_RELAY_DIR
    trap "rm -rf '$AUTOMATE_LINUX_RELAY_DIR' 2>/dev/null; kill $AUTOMATE_LINUX_RELAY_PID 2>/dev/null" EXIT
fi
AUTOMATE_LINUX_SUBSEQUENT_SOURCE=true
PS1='\[\e]0;'$AUTOMATE_LINUX_TTY_NUMBER'\w\a\]'"${_yellow}\w${_nc}\$ "
cp ~/coding/automateLinux/desktop/*.desktop ~/Desktop/
# runSingleton "$AUTOMATE_LINUX_SYMLINK_DIR/restartCorsairKeyBoardLogiMouseService.sh"
PROMPT_COMMAND=". $AUTOMATE_LINUX_PROMPT_COMMAND_SCRIPT_FILE"
. "${AUTOMATE_LINUX_TERMINAL_DIR}unset.sh"

