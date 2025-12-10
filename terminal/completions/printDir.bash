_printDir_completion() {
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    opts="--help -h -d -f --no-color --copy -c"

    case "$prev" in
        -d)
            # directories with trailing slash
            COMPREPLY=( $(compgen -d -- "$cur") )
            for i in "${!COMPREPLY[@]}"; do
                [[ -d "${COMPREPLY[$i]}" ]] && COMPREPLY[$i]+='/'
            done
            return 0
            ;;
        -f)
            # files and directories, add trailing slash for dirs
            COMPREPLY=( $(compgen -f -- "$cur") )
            for i in "${!COMPREPLY[@]}"; do
                [[ -d "${COMPREPLY[$i]}" ]] && COMPREPLY[$i]+='/'
            done
            return 0
            ;;
    esac

    # Suggest options even without leading -
    COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
}

complete -o bashdefault -o default -o nospace -F _printDir_completion print
