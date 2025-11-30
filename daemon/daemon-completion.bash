_daemon_completion() {
    local cur prev opts commands
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    # Define available commands
    commands="initialize DIR ttyOpened update cdBackward cdForward print reset status resetWithDefaultDir getEscapedTty testIfProper resetDirHistoryToBeginningStateIfError resetDirHistoryToBeginningState initializeDirHistory cdToPointer insertDir insertDirAtIndex insertDirAfterIndex setDirHistoryPointer getDirHistoryPointer getDirFromHistory updateDirHistory navigateBack navigateForward"
    
    # Main options
    opts="-h --help -v --verbose --version --complete"
    
    case "$prev" in
        daemon)
            # Complete with commands and options
            COMPREPLY=( $(compgen -W "$commands $opts" -- "$cur") )
            ;;
        update|resetWithDefaultDir)
            # These commands expect file/directory arguments
            COMPREPLY=( $(compgen -f -- "$cur") )
            ;;
        getEscapedTty|testIfProper|resetDirHistoryToBeginningStateIfError|getDirHistoryPointer|getDirFromHistory)
            # These commands expect file arguments
            COMPREPLY=( $(compgen -f -- "$cur") )
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
