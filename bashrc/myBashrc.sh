# echo asdf
if [ -f "${AUTOMATE_LINUX_BASH_RC_DIR}aliases.sh" ]; then
    . "${AUTOMATE_LINUX_BASH_RC_DIR}aliases.sh"
fi
if [ -f "${AUTOMATE_LINUX_BASH_RC_DIR}functions.sh" ]; then
    . "${AUTOMATE_LINUX_BASH_RC_DIR}functions.sh"
fi
initializeDirHistoryFileTty
goToDirPointer
##########################
export PATH="/home/yaniv/coding/flatBuffers/execute:$PATH"
export PATH=$PATH:/usr/local/go/bin
generatePassword() { python3 ~/generatePassword.py; }
export PATH="$PATH:$HOME/coding/scripts"
h() {
    history | grep "$@"
}
export FREECAD_MACROS_DIR="/home/yaniv/coding/freeCad/Macros/"
source ~/coding/automateLinux/utilities/sendKeys/sendkeys-completion.bash
EVSIEVE_LOG_FILE=$(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/../evsieve/log/log.txt")
[ -f "$AUTOMATE_LINUX_ENV_FILE" ] && source "$AUTOMATE_LINUX_ENV_FILE"
SCRIPT="$AUTOMATE_LINUX_SYMLINK_DIR/restartCorsairKeyBoardLogiMouseService.sh"
if [ -f "$SCRIPT" ] && ! pgrep -f "$SCRIPT" > /dev/null; then
    sudo -n "$SCRIPT" > /dev/null 2>/dev/null
fi
PROMPT_COMMAND=". $AUTOMATE_LINUX_PROMPT_COMMAND_SCRIPT_FILE"
# pst=$PS1
# if [ -f "$AUTOMATE_LINUX_BASH_RC_DIR./theRealPath" ]; then
#     . "$AUTOMATE_LINUX_BASH_RC_DIR./theRealPath"
# fi
