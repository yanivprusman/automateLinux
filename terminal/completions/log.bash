_log_completion() {
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    opts="evsieve/dev/pts/"
    COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
}

complete -o bashdefault -o default -o nospace -F _log_completion log
