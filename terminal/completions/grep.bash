# _grep_completion() {
#     local cur prev opts
#     COMPREPLY=()
#     cur="${COMP_WORDS[COMP_CWORD]}"
#     prev="${COMP_WORDS[COMP_CWORD-1]}"
#     opts="--exclude-dir={"
#     # COMPREPLY=( $(compgen -W "$opts" -- "$cur") )
#     COMPREPLY=( $(compgen -W "--exclude-dirchrome,.git}" -- "$cur") )

# }

# complete -o bashdefault -o default -o nospace -F _grep_completion grep
