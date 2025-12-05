_build_completion() {
    local cur prev
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    local opts="-rebuild -source"
    filtered=""
    for opt in $opts; do
        skip=false
        for used in "${COMP_WORDS[@]:1}"; do   # skip command itself
            [[ "$used" == "$opt" ]] && skip=true
        done
        [[ $skip == false ]] && filtered="$filtered $opt"
    done
    COMPREPLY=( $(compgen -W "$filtered" -- "$cur") )
}

complete -F _build_completion b
