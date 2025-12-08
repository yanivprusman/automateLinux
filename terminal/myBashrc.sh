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
set -E 
if [[ ! -v AUTOMATE_LINUX_SUBSEQUENT_SOURCE ]]; then
    . $(theRealPath firstTime.sh)
fi
AUTOMATE_LINUX_SUBSEQUENT_SOURCE=true
PS1='\[\e]0;'$AUTOMATE_LINUX_TTY_NUMBER'\w\a\]'"${_yellow}\w${_nc}\$ "
cp ~/coding/automateLinux/desktop/*.desktop ~/Desktop/
# runSingleton "$AUTOMATE_LINUX_SYMLINK_DIR/restartCorsairKeyBoardLogiMouseService.sh"
PROMPT_COMMAND=". $AUTOMATE_LINUX_PROMPT_COMMAND_SCRIPT_FILE"
. "${AUTOMATE_LINUX_TERMINAL_DIR}unset.sh"

