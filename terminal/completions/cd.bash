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
        local IFS=$'\n'
        local completions=($(compgen -d -- "$cur"))
        local i=0
        for item in "${completions[@]}"; do
            COMPREPLY[i]="${item}/"
            ((i++))
        done
    fi
}
complete -F _cd -o nospace cd