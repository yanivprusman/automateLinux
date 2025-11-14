readarray -t varsUntilMyBashrc <<< "$(compgen -v)"
myBashSourceFiles=(
    "${AUTOMATE_LINUX_TERMINAL_DIR}colors.sh"
    "${AUTOMATE_LINUX_TERMINAL_DIR}aliases.sh"
    "${AUTOMATE_LINUX_TERMINAL_FUNCTIONS_DIR}functions.sh"
    "${AUTOMATE_LINUX_BINDINGS_DIR}bindings.sh"
    "${AUTOMATE_LINUX_TERMINAL_DIR}exports.sh"
    "${AUTOMATE_LINUX_TERMINAL_DIR}gSettings.sh"
)
for file in "${myBashSourceFiles[@]}"; do
    if [ -f "$file" ]; then
        . "$file"
    else
        echo "No file found at $file"
    fi
done
touchDirectories
trap ". $AUTOMATE_LINUX_TRAP_ERR_FILE" ERR
set -E # "$AUTOMATE_LINUX_TRAP_GENERATOR_FILE"
if [[ ! -v "$AUTOMATE_LINUX_SUBSEQUENT_SOURCE" ]]; then :
    initializeDirHistory
    cdToPointer
fi
if [[ -v "$AUTOMATE_LINUX_SUBSEQUENT_SOURCE" ]]; then
    AUTOMATE_LINUX_SUBSEQUENT_SOURCE=true
fi
PS1='\[\e]0;\w\a\]\[\033[1;34m\]\w\[\033[0m\]\$ '
# runSingleton "$AUTOMATE_LINUX_SYMLINK_DIR/restartCorsairKeyBoardLogiMouseService.sh"
AUTOMATE_LINUX_SUBSEQUENT_SOURCE=true
PROMPT_COMMAND=". $AUTOMATE_LINUX_PROMPT_COMMAND_SCRIPT_FILE"
. "${AUTOMATE_LINUX_TERMINAL_DIR}unset.sh"
