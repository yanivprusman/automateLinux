#!/bin/sh

# Resolve script location (needed for relative paths)
SCRIPT="$0"
while [ -h "$SCRIPT" ]; do
    DIR=$(dirname "$SCRIPT")
    LINK=$(readlink "$SCRIPT")
    case "$LINK" in
        /*) SCRIPT="$LINK" ;;
        *) SCRIPT="$DIR/$LINK" ;;
    esac
done
SCRIPT_DIR=$(cd -P "$(dirname "$SCRIPT")" && pwd)

TARGET="$1"
[ -z "$TARGET" ] && TARGET="."

# Handle no args - print current directory
[ -z "$TARGET" ] && cd -P "$PWD" && pwd -P && exit

# Convert to absolute path if relative
case "$TARGET" in
    /*) : ;;
    *) TARGET="$PWD/$TARGET" ;;
esac

# Let cd -P do all path resolution and handle the last component
cd -P "$(dirname "$TARGET")" 2>/dev/null || exit 1
BASE=$(basename "$TARGET")
if [ "$BASE" = "." ] || [ "$BASE" = ".." ]; then
    cd -P "$BASE" 2>/dev/null && pwd -P
elif [ -d "$(pwd -P)/$BASE" ]; then
    cd -P "$BASE" 2>/dev/null && pwd -P
else
    echo "$(pwd -P)/$BASE"
fi

if cd -P "$DIR_PART" 2>/dev/null; then
    RESOLVED_DIR=$(pwd -P)
    if [ -d "$RESOLVED_DIR/$BASE_PART" ]; then
        cd -P "$BASE_PART" 2>/dev/null && pwd -P
    else
        echo "$RESOLVED_DIR/$BASE_PART"
    fi
else
    echo "$TARGET"
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
