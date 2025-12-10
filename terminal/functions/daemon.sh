daemon() {
    if [ -z "$AUTOMATE_LINUX_DAEMON_FD_IN" ] || [ -z "$AUTOMATE_LINUX_DAEMON_FD_OUT" ]; then
        return 1
    fi
    if ! { true >&"$AUTOMATE_LINUX_DAEMON_FD_IN"; } 2>/dev/null || ! { true <&"$AUTOMATE_LINUX_DAEMON_FD_OUT"; } 2>/dev/null; then
        AUTOMATE_LINUX_DAEMON_PID=$(${AUTOMATE_LINUX_DAEMON_DIR}getDaemonPID.sh) && \
        coproc DAEMON_COPROC { socat - UNIX-CONNECT:"$AUTOMATE_LINUX_SOCKET_PATH" 2>/dev/null; } && \
        export AUTOMATE_LINUX_DAEMON_FD_IN=${DAEMON_COPROC[1]} && \
        export AUTOMATE_LINUX_DAEMON_FD_OUT=${DAEMON_COPROC[0]}
    fi
    local formatOutput="false"
    local formatOutputTrueFunctions=("" "--help" "showDB" "showTerminalInstance" "showAllTerminalInstances" "showEntriesByPrefix" "printDirHistory")
    if [[ " ${formatOutputTrueFunctions[*]} " == *" $1 "* ]]; then
        formatOutput="true"
    fi
    local args=()
    for arg in "$@"; do
        if [ "$arg" = "--json" ]; then
            formatOutput="true"
        else
            args+=("$arg")
        fi
    done
    local COMMAND="${args[0]}" json key value reply
    json="{\"command\":\"$COMMAND\""
    json+=",\"tty\":$AUTOMATE_LINUX_TTY_NUMBER"
    for ((i=1; i<${#args[@]}; i++)); do
        arg="${args[$i]}"
        key="${arg%%=*}"
        value="${arg#*=}"
        json+=",\"$key\":\"$value\""
    done
    json+="}"
    if [ "$COMMAND" = closedTty ]; then
        echo $json | nc -U -q 1 "$AUTOMATE_LINUX_SOCKET_PATH"
        return 0
    fi
    printf '%s\n' "$json" >&"$AUTOMATE_LINUX_DAEMON_FD_IN"
    read -t 2 -r reply <&"$AUTOMATE_LINUX_DAEMON_FD_OUT"
    if [ "$formatOutput" = "true" ]; then
        printf '%s' "$reply" | awk '
        {gsub(/\\n/,"\n"); printf "%s", $0} 
        END {if (substr($0,length,1)!="\n") printf "\n"}'
    else
        printf '%s' "$reply"
        if [[ "$reply" != "" ]]; then
            printf '\n'
        fi
    fi
}
export -f daemon

d(){
    daemon "$@"
}
export -f d
