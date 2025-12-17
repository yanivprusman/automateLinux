# _changeDevice_completion() {
#     local cur prev
#     COMPREPLY=()
#     cur="${COMP_WORDS[COMP_CWORD]}"
#     prev="${COMP_WORDS[COMP_CWORD-1]}"

#     if [[ ${COMP_CWORD} -eq 1 ]]; then
#         COMPREPLY=( $(compgen -W "terminal chrome code default" -- "$cur") )
#     fi
# }
# complete -F _changeDevice_completion changeDevices.sh

_changeDevices_completion() {
    local cur prev
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    commands="terminal chrome code default"

    if [[ ${COMP_CWORD} -eq 1 ]]; then
        COMPREPLY=( $(compgen -W "$commands" -- "$cur") )
    fi
}
complete -F _changeDevices_completion changeDevices.sh
