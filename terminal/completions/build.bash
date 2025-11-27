_build_completion() {
    local cur prev
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    local opts="-rebuild"
    COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
}

complete -F _build_completion b
