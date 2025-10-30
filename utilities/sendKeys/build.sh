SCRIPT_PATH="$(realpath "${BASH_SOURCE[0]}")"
SCRIPT_DIR="$(dirname "$SCRIPT_PATH")"
SOURCE="$SCRIPT_DIR/sendKeys.c"
gcc -o $SCRIPT_DIR/sendKeys $SOURCE && sudo $SCRIPT_DIR/sendKeys
