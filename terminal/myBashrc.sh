if [ -f "${AUTOMATE_LINUX_TERMINAL_DIR}aliases.sh" ]; then
    . "${AUTOMATE_LINUX_TERMINAL_DIR}aliases.sh"
    else echo "No aliases file found at ${AUTOMATE_LINUX_TERMINAL_DIR}aliases.sh"
fi
if [ -f "${AUTOMATE_LINUX_TERMINAL_DIR}functions.sh" ]; then
    . "${AUTOMATE_LINUX_TERMINAL_DIR}functions.sh"
fi
if [ -f "${AUTOMATE_LINUX_BINDINGS_DIR}bindings.sh" ]; then
    . "${AUTOMATE_LINUX_BINDINGS_DIR}bindings.sh"
fi
if [ -f "${AUTOMATE_LINUX_TERMINAL_DIR}exports.sh" ]; then
    . "${AUTOMATE_LINUX_TERMINAL_DIR}exports.sh"
fi
cd $AUTOMATE_LINUX_TERMINAL_DIR
trap ". $AUTOMATE_LINUX_TRAP_ERR_FILE" ERR
initializeDirHistoryFileTty
goToDirPointer
PS1='\[\e]0;\w\a\]\[\033[1;34m\]\w\[\033[0m\]\$ '
[ -f "$AUTOMATE_LINUX_ENV_FILE" ] && source "$AUTOMATE_LINUX_ENV_FILE"
SCRIPT="$AUTOMATE_LINUX_SYMLINK_DIR/restartCorsairKeyBoardLogiMouseService.sh"
# if [ -f "$SCRIPT" ] && ! pgrep -f "$SCRIPT" > /dev/null; then
#     sudo -n "$SCRIPT" > /dev/null 2>/dev/null
# fi
PROMPT_COMMAND=". $AUTOMATE_LINUX_PROMPT_COMMAND_SCRIPT_FILE"
cat $AUTOMATE_LINUX_TRAP_ERR_LOG_FILE
