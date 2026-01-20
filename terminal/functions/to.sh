to() {
    local prod=false
    local target=""

    # Parse arguments
    while [[ $# -gt 0 ]]; do
        case "$1" in
            -prod)
                prod=true
                shift
                ;;
            *)
                target="$1"
                shift
                ;;
        esac
    done

    local base=$(d getDir --dirName base)
    local extra_apps_dir="${base}extraApps"
    local prod_dir="$HOME/coding/prod"

    # Handle automateLinux specially
    if [[ "$target" == "automateLinux" ]]; then
        cd "$base"
        return 0
    fi

    # Check if target is a valid app in extraApps
    if [[ -n "$target" && -d "${extra_apps_dir}/${target}" ]]; then
        if $prod; then
            cd "${prod_dir}/${target}"
        else
            cd "${extra_apps_dir}/${target}"
        fi
        return 0
    fi

    return 1
}

_to_completions() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local extra_apps_dir="${AUTOMATE_LINUX_DIR}/extraApps"

    # Build list of completions
    local completions="-prod automateLinux"
    if [[ -d "$extra_apps_dir" ]]; then
        completions="$completions $(ls -1 "$extra_apps_dir" 2>/dev/null | tr '\n' ' ')"
    fi

    COMPREPLY=($(compgen -W "$completions" -- "$cur"))
}

complete -F _to_completions to
