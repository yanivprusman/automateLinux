_printDir_completion() {
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    opts="--help -h -d -f --no-color -copy"
    if [[ "$cur" == --* ]]; then
        COMPREPLY=( $(compgen -W "--no-color --help" -- "$cur") )
        return 0
    fi
    if [[ "$cur" == -* ]]; then
        COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
        return 0
    fi
    
    # # If previous word is -f, suggest files
    # if [[ "$prev" == "-f" ]]; then
    #     COMPREPLY=( $(compgen -f -- "$cur") )
    #     return 0
    # fi
    
    # # If previous word is -d or we're collecting directories, suggest directories
    # if [[ "$prev" == "-d" ]] || [[ "$prev" != "-f" && "$prev" != "-"* ]]; then
    #     COMPREPLY=( $(compgen -d -- "$cur") )
    #     return 0
    # fi
}

complete -o bashdefault -o default -o nospace -F _printDir_completion printDir
