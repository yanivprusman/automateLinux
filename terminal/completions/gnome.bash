#!/usr/bin/env bash

_gnomeExtensionRestart() {
    local cur
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    
    local commands="active-window-tracker@example.com clock@ya-niv.com"
    
    COMPREPLY=($(compgen -W "$commands" -- "$cur"))
    return 0  # <-- This is what you were missing!
}

complete -F _gnomeExtensionRestart gnomeExtensionRestart