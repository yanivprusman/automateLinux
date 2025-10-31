#!/usr/bin/env bash

_tailwindow_completion() {
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    # List of options
    opts="-h --help -s --size"

    # Handle option arguments
    case "${prev}" in
        -s|--size)
            # No completion for size (numeric value)
            return 0
            ;;
        *)
            # If current word starts with -, complete from opts
            if [[ ${cur} == -* ]] ; then
                COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
                return 0
            fi
            # Otherwise complete with file paths
            COMPREPLY=( $(compgen -f -- ${cur}) )
            return 0
            ;;
    esac
}

# Complete both the command name and the full path
complete -F _tailwindow_completion tailWindow
complete -F _tailwindow_completion ./tailWindow