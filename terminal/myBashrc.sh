if [ -f "${AUTOMATE_LINUX_TERMINAL_DIR}aliases.sh" ]; then
    . "${AUTOMATE_LINUX_TERMINAL_DIR}aliases.sh"
    else echo "No aliases file found at ${AUTOMATE_LINUX_TERMINAL_DIR}aliases.sh"
fi
if [ -f "${AUTOMATE_LINUX_TERMINAL_FUNCTIONS_DIR}functions.sh" ]; then
    . "${AUTOMATE_LINUX_TERMINAL_FUNCTIONS_DIR}functions.sh"
fi
if [ -f "${AUTOMATE_LINUX_BINDINGS_DIR}bindings.sh" ]; then
    . "${AUTOMATE_LINUX_BINDINGS_DIR}bindings.sh"
fi
if [ -f "${AUTOMATE_LINUX_TERMINAL_DIR}exports.sh" ]; then
    . "${AUTOMATE_LINUX_TERMINAL_DIR}exports.sh"
fi
trap ". $AUTOMATE_LINUX_TRAP_ERR_FILE" ERR
"$AUTOMATE_LINUX_TRAP_GENERATOR_FILE"
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
CDPATH=/home/yaniv/coding/automateLinux/