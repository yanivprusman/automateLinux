_prepareBuild_completion() {
    local cur prev
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    
    # First argument is the language option
    if [[ ${COMP_CWORD} -eq 1 ]]; then
        local opts="-c -cpp"
        COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
    # After language option, complete directory names
    elif [[ ${COMP_CWORD} -gt 1 ]]; then
        COMPREPLY=( $(compgen -d -- "$cur") )
    fi
}

complete -F _prepareBuild_completion prepareBuild
