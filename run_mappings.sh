sudo pkill -9 evsieve &>/dev/null
sleep 0.3
SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
MAPPINGS_DIR="$SCRIPT_DIR/mappings"
for script in "$MAPPINGS_DIR"/*.sh; do
    if [ -f "$script" ]; then
        "$script" &>/dev/null &
    fi
done