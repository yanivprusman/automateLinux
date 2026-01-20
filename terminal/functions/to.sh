to() {
    local target="$1"
    local base=$(d getDir --dirName base)

    case "$target" in
        cad)
            cd "${base}extraApps/cad"
            ;;
        automateLinux)
            cd "$base/"
            ;;
        loom)
            cd "${base}extraApps/loom"
            ;;
        *)
            # echo "Target '$target' not configured."
            return 1
            ;;
    esac
}