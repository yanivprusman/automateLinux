#!/usr/bin/env bash

_sendkeys_completion() {
    local cur prev opts commands
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    # List of options and commands
    opts="-h --help -k --keyboard"
    commands="keyADown keyAUp keyA numlock Code gnome-terminal-server google-chrome syn"

    # Handle option arguments
    case "${prev}" in
        -k|--keyboard)
            # Complete with device paths from /dev/input/by-id/
            COMPREPLY=( $(compgen -W "$(ls /dev/input/by-id/)" -- ${cur}) )
            return 0
            ;;
    esac

    # If current word starts with -, complete from opts
    if [[ ${cur} == -* ]] ; then
        COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
        return 0
    fi

    # Otherwise complete from commands
    COMPREPLY=( $(compgen -W "${commands}" -- ${cur}) )
    return 0
}

# Complete both the command name and the full path
complete -F _sendkeys_completion sendKeys
complete -F _sendkeys_completion ./toggle/sendKeys