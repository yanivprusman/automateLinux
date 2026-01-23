_openFunction_completions() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local funcs=$(listMyFunctions)
    COMPREPLY=($(compgen -W "$funcs" -- "$cur"))
}
