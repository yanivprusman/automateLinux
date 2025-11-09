exit_code=$?
cmd="${BASH_COMMAND}"
line_no=${BASH_LINENO[0]}
func="${FUNCNAME[1]}"
# date_time
date_time=$(date '+%Y-%m-%d %H:%M:%S')
{
    echo "$date_time Exit code: $exit_code Command: $cmd Line: $line_no Function: ${func:-main} PWD: $PWD"
    echo "Script:"
    for f in "${BASH_SOURCE[@]}"; do
        echo "  $f"
    done
    echo "------------------------------------------"
} >> "$AUTOMATE_LINUX_TRAP_ERR_LOG_FILE"