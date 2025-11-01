#!/bin/sh

# Handle no args - print current directory
[ -z "$1" ] && cd -P "$PWD" && pwd -P && exit 0

# Save original directory and target
ORIG_PWD="$PWD"
TARGET="$1"

# Handle relative paths
case "$TARGET" in
    /*) : ;;
    */|.|./*|..|..|../*) TARGET="$ORIG_PWD/$TARGET" ;;
    *) TARGET="$ORIG_PWD/$TARGET" ;;
esac

# Clean up path by resolving one directory at a time
cd -P / || exit 1
set -- $(echo "$TARGET" | tr '/' ' ')
for d; do
    [ -z "$d" ] && continue
    [ "$d" = "." ] && continue
    [ "$d" = ".." ] && cd .. && continue
    [ -d "$d" ] && cd "$d" || {
        [ "$d" = "$(basename "$TARGET")" ] && echo "$(pwd -P)/$d"
        cd "$ORIG_PWD"
        exit 0
    }
done
pwd -P

# Handle special cases and directories
if [ "$BASE" = "." ] || [ "$BASE" = ".." ]; then
    cd -P "$BASE" 2>/dev/null && pwd -P
elif [ -d "$(pwd -P)/$BASE" ]; then
    cd -P "$BASE" 2>/dev/null && pwd -P
else
    echo "$(pwd -P)/$BASE"
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
