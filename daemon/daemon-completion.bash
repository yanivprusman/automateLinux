_daemon_completion() {
    local cur prev opts commands words
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    # Define available commands
    commands="openedTty updateDirHistory cdForward cdBackward showIndex deleteAllDirEntries listAllEntries"
    
    # Main options
    opts="--json -h --help"
    
    case "$prev" in
        daemon|d)
            # After daemon/d, suggest commands and options
            COMPREPLY=( $(compgen -W "$commands $opts" -- "$cur") )
            ;;
        --json)
            # After --json flag, complete with commands
            COMPREPLY=( $(compgen -W "$commands" -- "$cur") )
            ;;
        openedTty|updateDirHistory|cdForward|cdBackward|showIndex|deleteAllDirEntries|listAllEntries)
            # After a command, suggest --json flag
            COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
            ;;
        *)
            # For other cases, check if we're looking at an option
            case "$cur" in
                -*)
                    # Complete with options
                    COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
                    ;;
                *)
                    # Default: check context
                    if [[ $COMP_CWORD -eq 1 || ( $COMP_CWORD -eq 2 && "${COMP_WORDS[1]}" == "--json" ) ]]; then
                        COMPREPLY=( $(compgen -W "$commands" -- "$cur") )
                    else
                        # Suggest options/flags for any position after command
                        COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
                    fi
                    ;;
            esac
            ;;
    esac
}

complete -F _daemon_completion daemon
complete -F _daemon_completion d

