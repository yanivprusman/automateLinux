SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"
MAPPINGS_DIR="$SCRIPT_DIR/mappings"
chmod +x "$0"
declare -A script_pids

is_script_running() {
    local pid=$1
    if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
        return 0  # Script is running
    else
        return 1  # Script is not running
    fi
}

start_scripts() {
    for script in "$MAPPINGS_DIR"/*.sh; do
        if [ -f "$script" ]; then
            script_name=$(basename "$script")
            if ! is_script_running "${script_pids[$script_name]}"; then
                echo "Starting script: $script"
                chmod +x "$script"  # Ensure the script is executable
                "$script" &
                script_pids[$script_name]=$!
                echo "Started $script_name with PID ${script_pids[$script_name]}"
            fi
        fi
    done
}

# Function to monitor and restart scripts if they die
# monitor_scripts() {
#     for script in "$MAPPINGS_DIR"/*.sh; do
#         if [ -f "$script" ]; then
#             script_name=$(basename "$script")
#             if ! is_script_running "${script_pids[$script_name]}"; then
#                 echo "Script $script_name died, restarting..."
#                 "$script" &
#                 script_pids[$script_name]=$!
#                 echo "Restarted $script_name with PID ${script_pids[$script_name]}"
#             fi
#         fi
#     done
# }

# # Cleanup function
# cleanup() {
#     echo "Cleaning up..."
#     for pid in "${script_pids[@]}"; do
#         if is_script_running "$pid"; then
#             kill "$pid" 2>/dev/null
#         fi
#     done
#     exit 0
# }

# # Set up trap for cleanup
# trap cleanup SIGTERM SIGINT

# Initial start of all scripts
start_scripts

# # Main loop - monitor and restart scripts if needed
# while true; do
#     monitor_scripts
#     sleep 10  # Check every 10 seconds
# done