SCRIPT="$0"
while [ -h "$SCRIPT" ]; do
  LINK=$(ls -ld "$SCRIPT" | awk '{print $NF}')
  case "$LINK" in
    /*)
      SCRIPT="$LINK" ;;
    *)
      SCRIPT="$(dirname "$SCRIPT")/$LINK" ;;
  esac
done
SCRIPT_DIR=$(cd -P "$(dirname "$SCRIPT")" > /dev/null 2>&1 && pwd)
if [ "$1" = "../../" ]; then
  echo "$(cd "$SCRIPT_DIR/../.." > /dev/null 2>&1 && pwd)"
else
  TARGET="$1"
  if [ -z "$TARGET" ]; then
    cd -P "$PWD" > /dev/null 2>&1 && pwd
  else
    case "$TARGET" in
      "."|"./")
        cd -P "$PWD" > /dev/null 2>&1 && pwd
        ;;
      ".."|"../")
        cd -P "$PWD/.." > /dev/null 2>&1 && pwd
        ;;
      *)
        DIR_PATH="$(cd -P "$(dirname "$SCRIPT_DIR/$TARGET")" > /dev/null 2>&1 && pwd)"
        BASE_NAME="$(basename "$TARGET")"
        FULL_PATH="$DIR_PATH/$BASE_NAME"
        if [ -e "$FULL_PATH" ]; then
          if [ -d "$FULL_PATH" ]; then
            cd -P "$FULL_PATH" > /dev/null 2>&1 && pwd
          else
            echo "$FULL_PATH"
          fi
        else
          echo "$DIR_PATH/$BASE_NAME"
        fi
        ;;
    esac
  fi
fi
did i ovecomplicate it or does every part needed