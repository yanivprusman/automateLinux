myBashSourceFiles=(
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
trap ". $AUTOMATE_LINUX_TRAP_ERR_FILE" ERR
set -E
# "$AUTOMATE_LINUX_TRAP_GENERATOR_FILE"
initializeDirHistoryFileTty
goToDirPointer
PS1='\[\e]0;\w\a\]\[\033[1;34m\]\w\[\033[0m\]\$ '
# runSingleton "$AUTOMATE_LINUX_SYMLINK_DIR/restartCorsairKeyBoardLogiMouseService.sh"
PROMPT_COMMAND=". $AUTOMATE_LINUX_PROMPT_COMMAND_SCRIPT_FILE"
# if [ $(wc -l < "$AUTOMATE_LINUX_TRAP_ERR_LOG_FILE") -gt 1 ]; then
#     cat $AUTOMATE_LINUX_TRAP_ERR_LOG_FILE
# fi
# if [ $(wc -l < "$AUTOMATE_LINUX_TRAP_ERR_LOG_FILE_BACKGROUND") -gt 1 ]; then
#     sleep 1
#     cat $AUTOMATE_LINUX_TRAP_ERR_LOG_FILE_BACKGROUND
# fi
# CDPATH=/home/yaniv/coding/automateLinux/