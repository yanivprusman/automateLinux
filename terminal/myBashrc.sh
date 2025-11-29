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
    # if [ -f "$file" ]; then
        . "$file"
    # else
    #     echo "No file found at $file"
    # fi
done
touchDirectories
trap ". $AUTOMATE_LINUX_TRAP_ERR_FILE" ERR
set -E 
if [[ ! -v AUTOMATE_LINUX_SUBSEQUENT_SOURCE ]]; then :
# daemon ttyOpened
# daemon cdToPointer
AUTOMATE_LINUX_SUBSEQUENT_SOURCE=true
fi
PS1='\[\e]0;'$AUTOMATE_LINUX_TTY_NUMBER'\w\a\]'"${_yellow}\w${_nc}\$ "
cp ~/coding/automateLinux/desktop/*.desktop ~/Desktop/
# nc -U "$AUTOMATE_LINUX_SOCKET_PATH" >/dev/null &
# nc -U "$AUTOMATE_LINUX_SOCKET_PATH" </dev/null >/dev/null 2>&1 &
# PIPE=/tmp/daemon_pipe
# rm -f "$PIPE"
# mkfifo "$PIPE"
# nc -U "$AUTOMATE_LINUX_SOCKET_PATH" < "$PIPE" >/dev/null 2>&1 &
# nc -U "$AUTOMATE_LINUX_SOCKET_PATH" >/dev/null 2>&1 &
# NC_PID=$!
# PIPE=/tmp/daemon_pipe
# rm -f "$PIPE"
# mkfifo "$PIPE"
# nc -U "$AUTOMATE_LINUX_SOCKET_PATH" < "$PIPE" &
# NC_PID=$!
# coproc MYNC { nc -U "$AUTOMATE_LINUX_SOCKET_PATH"; }
# exec 3<>$AUTOMATE_LINUX_SOCKET_PATH
initCoproc

# runSingleton "$AUTOMATE_LINUX_SYMLINK_DIR/restartCorsairKeyBoardLogiMouseService.sh"
AUTOMATE_LINUX_SUBSEQUENT_SOURCE=true
PROMPT_COMMAND=". $AUTOMATE_LINUX_PROMPT_COMMAND_SCRIPT_FILE"
. "${AUTOMATE_LINUX_TERMINAL_DIR}unset.sh"

