readarray -t varsUntilMyBashrc <<< "$(compgen -v)"
myBashSourceFiles=(
    "${AUTOMATE_LINUX_TERMINAL_DIR}colors.sh"
    "${AUTOMATE_LINUX_TERMINAL_DIR}aliases.sh"
    "${AUTOMATE_LINUX_TERMINAL_FUNCTIONS_DIR}functions.sh"
    "${AUTOMATE_LINUX_TERMINAL_DIR}bindings.sh"
    "${AUTOMATE_LINUX_TERMINAL_DIR}exports.sh"
    "${AUTOMATE_LINUX_TERMINAL_DIR}gSettings.sh"
    "${AUTOMATE_LINUX_TERMINAL_DIR}/completions/completion.sh"
    "${AUTOMATE_LINUX_TERMINAL_DIR}geminiSessions.sh"
)
for file in "${myBashSourceFiles[@]}"; do
    . "$file"
done
touchDirectories
set -E 
if [[ ! -v AUTOMATE_LINUX_SUBSEQUENT_SOURCE ]]; then
    . $(theRealPath firstTime.sh)
fi
AUTOMATE_LINUX_SUBSEQUENT_SOURCE=true
export AUTOMATE_LINUX_PS1='\[\e]0;\W '$AUTOMATE_LINUX_TTY_NUMBER'\a\]'"${_yellow}\w${_nc}\$ "
PS1=$AUTOMATE_LINUX_PS1
cp ~/coding/automateLinux/desktop/*.desktop ~/Desktop/
# runSingleton "$AUTOMATE_LINUX_SYMLINK_DIR/restartCorsairKeyBoardLogiMouseService.sh"
redirectSession
PROMPT_COMMAND=". $AUTOMATE_LINUX_PROMPT_COMMAND_SCRIPT_FILE"
. "${AUTOMATE_LINUX_TERMINAL_DIR}unset.sh"

