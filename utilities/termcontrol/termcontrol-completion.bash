_termcontrol_completion() {
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    # Main options
    opts="--buffer --char --char-range --set --set-range --cursor --raw --reset --help"
    
    case "$prev" in
        termcontrol)
            COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
            ;;
        --raw)
            COMPREPLY=( $(compgen -W "on off" -- "$cur") )
            ;;
        --cursor)
            COMPREPLY=( $(compgen -W "save restore" -- "$cur") )
            ;;
        --color)
            COMPREPLY=( $(compgen -W "0 1 2 3 4 5 6 7" -- "$cur") )
            ;;
        --attr)
            COMPREPLY=( $(compgen -W "bold underline reverse blink" -- "$cur") )
            ;;
        *)
            # For options that take multiple arguments, don't complete
            case "${COMP_WORDS[1]}" in
                --char|--char-range|--set|--set-range)
                    return 0
                    ;;
            esac
            # Complete with options for commands that support them
            case "$cur" in
                -*)
                    local subopts="--color --attr"
                    COMPREPLY=( $(compgen -W "$subopts" -- "$cur") )
                    ;;
            esac
            ;;
    esac
}

complete -F _termcontrol_completion termcontrol