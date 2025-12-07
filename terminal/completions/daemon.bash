_daemon_completion() {
    local cur prev opts commands words
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    commands="openedTty closedTty updateDirHistory cdForward cdBackward showTerminalInstance showAllTerminalInstances deleteEntry showEntriesByPrefix deleteEntriesByPrefix showDB "
    opts="--help"
    
    case "$prev" in
        daemon|d)
            COMPREPLY=( $(compgen -W "$commands $opts" -- "$cur") )
            ;;
        --json)
            COMPREPLY=( $(compgen -W "$commands" -- "$cur") )
            ;;
        openedTty|updateDirHistory|cdForward|cdBackward|showTerminalInstance|deleteAllDirEntries|listAllEntries)
            COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
            ;;
        *)
        case "$cur" in
            -*)
                COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
                ;;
            *)
                if [[ $COMP_CWORD -eq 1 || ( $COMP_CWORD -eq 2 && "${COMP_WORDS[1]}" == "--json" ) ]]; then
                    COMPREPLY=( $(compgen -W "$commands" -- "$cur") )
                else
                    COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
                fi
                ;;
        esac
        ;;
    esac
}

complete -F _daemon_completion daemon
complete -F _daemon_completion d

