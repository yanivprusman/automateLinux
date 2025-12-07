daemon() {
    if [ -z "$AUTOMATE_LINUX_DAEMON_FD_IN" ] || [ -z "$AUTOMATE_LINUX_DAEMON_FD_OUT" ]; then
        return 1
    fi
    local formatOutput="false"
    local formatOutputTrueFunctions=("" "--help" "showDB" "showTerminalInstance" "showAllTerminalInstances")
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
        printf '%s' "$reply" | sed 's/\\n/\n/g'
    else
        # printf %q "$reply" > /tmp/test.txt
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
