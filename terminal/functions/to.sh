to() {
    local target="$1"
    local base=$(d getDir base)

    case "$target" in
        cad)
            cd "$base/../cad"
            ;;
        automateLinux)
            cd "$base/"
            ;;
        *)
            # echo "Target '$target' not configured."
            return 1
            ;;
    esac
}