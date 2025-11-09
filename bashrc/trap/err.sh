exit_code=$?
cmd="${BASH_COMMAND}"
line_no=${BASH_LINENO[0]}
func="${FUNCNAME[1]}"
# date_time
date_time=$(date '+%Y-%m-%d %H:%M:%S')

{
    echo "[$date_time] ERROR DETECTED"
    echo "  Exit code : $exit_code"
    echo "  Command   : $cmd"
    echo "  Line      : $line_no"
    echo "  Function  : ${func:-main}"
    echo "  PWD       : $PWD"
    echo "  Script    : ${BASH_SOURCE[1]}"
    echo "------------------------------------------"
} >> "$AUTOMATE_LINUX_TRAP_ERR_LOG_FILE"