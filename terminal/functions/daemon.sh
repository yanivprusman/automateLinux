daemon() {
    if [ -z "$AUTOMATE_LINUX_DAEMON_FD_IN" ] || [ -z "$AUTOMATE_LINUX_DAEMON_FD_OUT" ]; then
        return 1
    fi
    
    local formatOutput="false"
    local args=()
    
    # Process all arguments, extract --json flag if present
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
    
    # Add remaining args as key=value pairs
    for ((i=1; i<${#args[@]}; i++)); do
        arg="${args[$i]}"
        key="${arg%%=*}"
        value="${arg#*=}"
        json+=",\"$key\":\"$value\""
    done
    json+="}"
    
    printf '%s\n' "$json" >&"$AUTOMATE_LINUX_DAEMON_FD_IN"
    read -t 2 -r reply <&"$AUTOMATE_LINUX_DAEMON_FD_OUT"
    
    if [ "$formatOutput" = "true" ]; then
        # Parse the response and format it nicely
        # Convert literal \n to actual newlines
        printf '%s' "$reply" | sed 's/\\n/\n/g'
    else
        # Output raw response as-is
        printf '%s\n' "$reply"
    fi
}
export -f daemon

d(){
    daemon "$@"
}
export -f d
