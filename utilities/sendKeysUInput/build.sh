SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
SOURCE="$SCRIPT_DIR/sendKeysUInput.c"

gcc -o "$SCRIPT_DIR/sendKeysUInput" "$SOURCE"
if [ $? -ne 0 ]; then
	echo "build failed"
	exit 1
fi

# If caller passed -d or --daemon, start the program in daemon mode (background)
if [ "$1" = "-d" ] || [ "$1" = "--daemon" ]; then
	echo "Starting sendKeysUInput in daemon mode..."
	sudo "$SCRIPT_DIR/sendKeysUInput" -d &
	sleep 0.1
	echo "done"
else
	echo "Compiled: $SCRIPT_DIR/sendKeysUInput"
	echo "Run with: sudo $SCRIPT_DIR/sendKeysUInput -d   to run as daemon"
fi
