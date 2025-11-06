deleteFunctions() {
    for f in $(declare -F | awk '{print $3}'); do
        unset -f "$f"
    done
}
deleteFunctions
AUTOMAT_LINUX_PATH_END=/ #or ''?
AUTOMAT_LINUX_DIR="/home/yaniv/coding/automateLinux$AUTOMAT_LINUX_PATH_END"
AUTOMAT_LINUX_BASH_RC_DIR="${AUTOMAT_LINUX_DIR}bashrc$AUTOMAT_LINUX_PATH_END"
AUTOMAT_LINUX_ENV_FILE="$AUTOMAT_LINUX_BASH_RC_DIR/env.sh"

