#!/bin/sh

# If no argument, print current directory
[ -z "$1" ] && cd -P "$PWD" && pwd -P && exit 0

# Save original directory and convert to absolute path
ORIG_PWD=$PWD
case "$1" in /*) TARGET=$1 ;; *) TARGET=$PWD/$1 ;; esac

# Let cd -P handle path resolution
cd -P "$(dirname "$TARGET")" 2>/dev/null || { cd "$ORIG_PWD"; exit 1; }
BASE=$(basename "$TARGET")

# Print resolved path
if [ "$BASE" = "." ] || [ "$BASE" = ".." ]; then
    cd -P "$BASE" 2>/dev/null && pwd -P
elif [ -d "$(pwd -P)/$BASE" ]; then
    cd -P "$BASE" 2>/dev/null && pwd -P
else
    echo "$(pwd -P)/$BASE"
fi

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
