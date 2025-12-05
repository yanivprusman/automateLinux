daemon() {
    if [ -z "$AUTOMATE_LINUX_DAEMON_FD_IN" ] || [ -z "$AUTOMATE_LINUX_DAEMON_FD_OUT" ]; then
        return 1
    fi
    local jsonOutput="false"
    for arg in "$@"; do
        case "$arg" in
            jsonOutput=true)
                jsonOutput="true"
                ;;
        esac
    done
    local COMMAND="$1" json key value reply
    shift
    json="{\"command\":\"$COMMAND\""
    json+=",\"tty\":$AUTOMATE_LINUX_TTY_NUMBER"
    for arg in "$@"; do
        key="${arg%%=*}"
        value="${arg#*=}"
        json+=",\"$key\":\"$value\""
    done
    json+="}"
    # echo "in daemon function, sending json: $json" >&2
    printf '%s\n' "$json" >&"$AUTOMATE_LINUX_DAEMON_FD_IN"
    read -t 2 -r reply <&"$AUTOMATE_LINUX_DAEMON_FD_OUT"
    # if [ "$jsonOutput" != "true" ]; then
    #     reply=$(echo "$reply" | jq -r '.message')
    # fi
    # if [[ ! "$reply" == "\n" ]] ; then 
    #     printf '%s\n' "$reply"
    # fi
    printf '%q\n' "$reply"

}
export -f daemon

d(){
    daemon "$@"
}
export -f d