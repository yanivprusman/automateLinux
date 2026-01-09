_tus_completion() {
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    opts="-a --all"

    if [[ ${cur} == -* ]] ; then
        COMPREPLY=( $(compgen -W "${opts}" -- "${cur}") )
        return 0
    fi
}

_user_management_completion() {
    local cur prev
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    
    # Complete with existing usernames
    local users
    users=$(awk -F: '$3 >= 1000 && $3 < 60000 {print $1}' /etc/passwd)
    COMPREPLY=( $(compgen -W "${users}" -- "${cur}") )
}

complete -F _tus_completion _tus
complete -F _user_management_completion _tuk _tur _theUserSetToBash _theUserAddToCoding __theUserReplicateGnome _setGidSameAsUid _theUserGroups _theUserSwitch
