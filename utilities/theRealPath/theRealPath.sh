theRealPath1() {
  SCRIPT="$0"
  while [ -h "$SCRIPT" ]; do
    LINK=$(ls -ld -- "$SCRIPT" 2>/dev/null | awk '{print $(NF)}')
    case $LINK in
      /*) SCRIPT="$LINK" ;;
      *)  SCRIPT="$(dirname -- "$SCRIPT")/$LINK" ;;
    esac
  done
  SCRIPT_DIR=$(cd -P -- "$(dirname -- "$SCRIPT")" >/dev/null 2>&1 && pwd)
  if [ -n "$1" ]; then
    case $1 in
      /*) FULL_PATH="$1" ;;
      *)  FULL_PATH="$SCRIPT_DIR/$1" ;;
    esac
    REAL_PATH=$(cd -P -- "$(dirname -- "$FULL_PATH")" >/dev/null 2>&1 && pwd)/$(basename -- "$FULL_PATH")
    echo "$REAL_PATH"
  else
    echo "$SCRIPT_DIR"
  fi
}
theRealPath() {
  # echo $1
  # `bash -c 'cd dir1 && ./pwd.sh'`
  echo "in theRealPath with arg: $1"
}
# dirname $1
# echo `bash -c 'cd dir1 && ./pwd.sh'`
# echo `bash -c 'cd $(dirname "$1") && ./pwd.sh'`

# echo $0
# echo $1
# bash -c 'dir=$(dirname "$1"); file=$(basename "$1"); echo "${dir}${file}"' _ "$1"
bash -c 'cd "$(dirname "$1")" && echo "$(pwd)/$(basename "$1")"' _ "$1"

# bash -c 'cd "$(dirname "$1")" && basename "$1" || echo "Error: pwd.sh not found in $(pwd)"' _ "$1"
