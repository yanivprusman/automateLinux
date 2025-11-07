# echo asdf
if [ -f "${AUTOMAT_LINUX_BASH_RC_DIR}aliases.sh" ]; then
    . "${AUTOMAT_LINUX_BASH_RC_DIR}aliases.sh"
fi
if [ -f "${AUTOMAT_LINUX_BASH_RC_DIR}functions.sh" ]; then
    . "${AUTOMAT_LINUX_BASH_RC_DIR}functions.sh"
fi
setDirHistoryPointerToLast
goToDirPointer
PROMPT_COMMAND="$AUTOMAT_LINUX_PROMPT_COMMAND_SCRIPT_FILE"
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
[ -f "$AUTOMAT_LINUX_ENV_FILE" ] && source "$AUTOMAT_LINUX_ENV_FILE"
SCRIPT="$AUTOMAT_LINUX_SYMLINK_DIR/restartCorsairKeyBoardLogiMouseService.sh"
if [ -f "$SCRIPT" ] && ! pgrep -f "$SCRIPT" > /dev/null; then
    sudo -n "$SCRIPT" > /dev/null 2>/dev/null
fi
# pst=$PS1
# if [ -f "$AUTOMAT_LINUX_BASH_RC_DIR./theRealPath" ]; then
#     . "$AUTOMAT_LINUX_BASH_RC_DIR./theRealPath"
# fi
