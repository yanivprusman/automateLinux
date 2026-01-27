
exit_code=$?
touch "$AUTOMATE_LINUX_TRAP_ERR_LOG_FILE"
cmd="${BASH_COMMAND}"
line_no=${BASH_LINENO[0]}
func="${FUNCNAME[1]}"
date_time=$(date '+%Y-%m-%d %H:%M:%S')
error_msg=$(2>&1 timeout 1 bash -c "$cmd" </dev/null >/dev/null || true)
{
    echo "$date_time Exit code: $exit_code Command: $cmd Line: $line_no Function: ${func:-main} PWD: $PWD"
    if [ -n "$error_msg" ]; then
        echo "Error message: $error_msg"
    fi
    echo "Stack:"
    for f in "${BASH_SOURCE[@]}"; do
        echo "  $f"
    done
    echo "------------------------------------------"
} >> "$AUTOMATE_LINUX_TRAP_ERR_LOG_FILE"

