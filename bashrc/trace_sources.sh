
trace_sources() {
    local file="$1"
    local indent="${2:-}"

    # Absolute path
    if [[ ! "$file" = /* ]]; then
        file="$PWD/$file"
    fi

    # Prevent infinite recursion
    [[ -f "$file" ]] || { echo "${indent}[Not found] $file"; return; }

    echo "${indent}--- $file ---"

    while IFS= read -r line; do
        # Print the line
        echo "${indent}$line"

        # Check for source or . command
        if [[ "$line" =~ ^[[:space:]]*(source|\.)[[:space:]]+(.+)$ ]]; then
            # Extract the sourced file (strip quotes)
            src="${BASH_REMATCH[2]}"
            src="${src%\"}"; src="${src#\"}"
            src="${src%\'}"; src="${src#\'}"

            # Resolve relative paths
            if [[ ! "$src" = /* ]]; then
                src="$(dirname "$file")/$src"
            fi

            # Recurse
            trace_sources "$src" "  $indent"
        fi
    done < "$file"
}
