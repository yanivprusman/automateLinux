
SCRIPT="$0"
while [ -h "$SCRIPT" ]; do
  # resolve $SCRIPT until the file is no longer a symlink
  LINK=$(ls -ld "$SCRIPT" | awk '{print $NF}')
  if [ "${LINK:0:1}" = "/" ]; then
    SCRIPT="$LINK"
  else
    SCRIPT="$(dirname "$SCRIPT")/$LINK"
  fi
done
SCRIPT_DIR=$(cd -P "$(dirname "$SCRIPT")" > /dev/null 2>&1 && pwd)

if [ "$1" = "../../" ]; then
  # When exactly "../../" is passed, always return two directories up from script location
  echo "$(cd "$SCRIPT_DIR/../.." > /dev/null 2>&1 && pwd)"
else
  # Normal path resolution for other cases
  TARGET="$1"
  if [ ! -z "$TARGET" ]; then
    DIR_PATH="$(cd -P "$(dirname "$SCRIPT_DIR/$TARGET")" > /dev/null 2>&1 && pwd)"
    BASE_NAME="$(basename "$TARGET")"
    FULL_PATH="$DIR_PATH/$BASE_NAME"
    
    # Check if path exists
    if [ -e "$FULL_PATH" ]; then
      # Always output full path, whether it's a file or directory
      echo "$FULL_PATH"
    else
      # If path doesn't exist, still show what it would resolve to
      echo "$DIR_PATH/$BASE_NAME"
    fi
  else
    echo "$SCRIPT_DIR"
  fi
fi

# bash -c '
#   target="$1"
#   cd -P "$(dirname "$target")" > /dev/null 2>&1 || exit
#   base="$(basename "$target")"
#   if [ "$base" = "." ] || [ "$base" = ".." ]; then
#     cd -P "$base" > /dev/null 2>&1 || exit
#     echo "$(pwd -P)"
#   else
#     echo "$(pwd -P)/$base"
#   fi
# ' _ "$1"
