# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"

# Source the completion script using the script directory as base
source "$(theRealPath "$SCRIPT_DIR/../utilities/termcontrol/termcontrol-completion.bash")"
