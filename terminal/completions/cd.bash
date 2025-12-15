_cd() {
    local cur="${COMP_WORDS[COMP_CWORD]}"

    if [[ "$cur" =~ ^\.{2,}$ ]]; then
        # Convert ... into ../.. style for completion
        local levels=$(( ${#cur} - 1 ))
        local prefix=
        for ((i=0; i<levels; i++)); do
            prefix+="../"
        done
        COMPREPLY=( $(compgen -d -- "$prefix") )
    else
        COMPREPLY=( $(compgen -d -- "$cur") )
    fi
}
complete -F _cd cd