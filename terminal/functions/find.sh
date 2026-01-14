findF() {
    _findF_show_help() {
    cat << EOF
Usage: findF -p PATTERN [-e EXCLUDE_DIRS] [-d SEARCH_PATH] [-h]

A robust function to find files using a regular expression while excluding directories.

OPTIONS:
   -h   Display this help text.
   -p   PATTERN   The POSIX-extended regular expression to match the file path (required).
   -e   DIRS      A comma-separated list of directory names to exclude.
   -d   PATH      The starting directory for the search. Defaults to '.'.

EXAMPLE:
  # Find all .json files, excluding 'node_modules' and 'dist'
  findF -p '.*\.json$' -e 'node_modules,dist'
EOF
    }
    local search_path="."
    local pattern=""
    local exclude_str=""
    local OPTIND
    while getopts "hp:e:d:" opt; do
        case "$opt" in
            h) _findF_show_help; return 0 ;;
            p) pattern="$OPTARG" ;;
            e) exclude_str="$OPTARG" ;;
            d) search_path="$OPTARG" ;;
            '?') _findF_show_help >&2; return 1 ;;
        esac
    done
    if [ -z "$pattern" ]; then
        echo "Error: The -p (pattern) argument is required." >&2
        _findF_show_help >&2
        return 1
    fi

    # If the pattern starts with '*', prepend a '.' to make it a valid regex search.
    # This allows for glob-like searches, e.g., "*.json"
    if [[ "${pattern:0:1}" == '*' ]]; then
        pattern=".$pattern"
    fi

    local name_prune_conditions=()
    if [ -n "$exclude_str" ]; then
        while IFS=',' read -ra exclude_dirs; do
            for dir in "${exclude_dirs[@]}"; do
                dir=$(echo "$dir" | awk '{$1=$1};1') 
                if [ -n "$dir" ]; then
                    if [ ${#name_prune_conditions[@]} -gt 0 ]; then
                        name_prune_conditions+=(-o)
                    fi
                    name_prune_conditions+=(-name "$dir")
                fi
            done
        done <<< "$exclude_str,"
    fi
    if [ ${#name_prune_conditions[@]} -gt 0 ]; then
        find_args+=(\( \( "${name_prune_conditions[@]}" \) -a -type d \) -prune -o)
    fi
    find_args+=(-type f -regextype posix-extended -regex "$pattern" -print)
    find "${find_args[@]}"
}

