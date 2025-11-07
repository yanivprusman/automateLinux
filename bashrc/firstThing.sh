deleteFunctions() {
    for f in $(declare -F | awk '{print $3}'); do
        unset -f "$f"
    done
}
deleteFunctions
export AUTOMAT_LINUX_PATH_END=/ #or ''?
export AUTOMAT_LINUX_DIR="/home/yaniv/coding/automateLinux$AUTOMAT_LINUX_PATH_END"
export AUTOMAT_LINUX_BASH_RC_DIR="${AUTOMAT_LINUX_DIR}bashrc$AUTOMAT_LINUX_PATH_END"
export AUTOMAT_LINUX_ENV_FILE="$AUTOMAT_LINUX_BASH_RC_DIR/env.sh"
export AUTOMAT_LINUX_SYMLINK_DIR="$AUTOMAT_LINUX_DIR/symlinks"
export AUTOMAT_LINUX_DIR_HISTORY_FILE="${AUTOMAT_LINUX_BASH_RC_DIR}dirHistory.sh"
export AUTOMAT_LINUX_DIR_HISTORY_POINTER
export AUTOMAT_LINUX_PROMPT_COMMAND_SCRIPT_FILE="${AUTOMAT_LINUX_BASH_RC_DIR}promptCommand.sh"
export AUTOMAT_LINUX_BASH_RC_LOG_FILE="${AUTOMAT_LINUX_BASH_RC_DIR}bashrcLog.txt"
export AUTOMAT_LINUX_VERBOSE=true
export AUTOMAT_LINUX_VERBOSE=false
if [ "$AUTOMAT_LINUX_VERBOSE" = true ]; then
    set -x
    exec > >(tee "$AUTOMAT_LINUX_BASH_RC_LOG_FILE" >/dev/null) 2>&1
fi
