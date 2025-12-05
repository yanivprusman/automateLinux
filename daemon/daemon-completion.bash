_daemon_completion() {
    local cur prev opts commands
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    # Define available commands
    commands="openedTty updateDirHistory cdForward cdBackward showIndex deleteAllDirEntries listAllEntries"
    
    # Main options
    opts="-h --help"
    
    case "$prev" in
        daemon|d)
            # Complete with commands and options
            COMPREPLY=( $(compgen -W "$commands $opts" -- "$cur") )
            ;;
        *)
            # For other cases, check if we're looking at an option
            case "$cur" in
                -*)
                    # Complete with options
                    COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
                    ;;
                *)
                    # Default: suggest commands if at a position where they're expected
                    if [[ $COMP_CWORD -eq 1 ]]; then
                        COMPREPLY=( $(compgen -W "$commands $opts" -- "$cur") )
                    fi
                    ;;
            esac
            ;;
    esac
}

complete -F _daemon_completion daemon
complete -F _daemon_completion d

