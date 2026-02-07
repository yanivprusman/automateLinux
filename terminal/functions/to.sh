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
    local dev_dir="/opt/dev"
    local prod_dir="/opt/prod"

    # Handle automateLinux specially
    if [[ "$target" == "automateLinux" ]]; then
        cd "$base"
        return 0
    fi

    # Check if target is a valid app in dev
    if [[ -n "$target" && -d "${dev_dir}/${target}" ]]; then
        if $prod; then
            cd "${prod_dir}/${target}"
        else
            cd "${dev_dir}/${target}"
        fi
        return 0
    fi

    return 1
}

_to_completions() {
    local cur="${COMP_WORDS[COMP_CWORD]}"
    local dev_dir="/opt/dev"

    # Build list of completions
    local completions="-prod automateLinux"
    if [[ -d "$dev_dir" ]]; then
        for app in "${dev_dir}"/*/; do
            if [[ -d "$app" ]]; then
                app_name=$(basename "$app")
                completions="$completions $app_name"
            fi
        done
    fi

    COMPREPLY=($(compgen -W "$completions" -- "$cur"))
}
