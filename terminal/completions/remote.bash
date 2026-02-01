_printRemoteFiles_completion() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local opts="retrieve -c"
    local available_opts=""

    # Get available options
    for opt in $opts; do
        local found=false
        for word in "${COMP_WORDS[@]}"; do
            if [[ "$word" == "$opt" ]]; then
                found=true
                break
            fi
        done
        if ! $found; then
            available_opts="$available_opts $opt"
        fi
    done
    
    COMPREPLY=( $(compgen -W "$available_opts" -- "$cur") )
}
complete -F _printRemoteFiles_completion printRemoteFiles

_rdp_completion() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local peers="desktop laptop vps local"
    COMPREPLY=( $(compgen -W "$peers" -- "$cur") )
}
complete -F _rdp_completion rdp