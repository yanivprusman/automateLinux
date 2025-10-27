#!/usr/bin/env bash
set -euo pipefail
input="${1:-}"
if [ -z "$input" ]; then
  if [ -t 0 ]; then
    echo "Usage: $0 \"<window-class>\" or provide input via stdin" >&2
    exit 2
  else
    read -r input
  fi
fi
case "$input" in
  gnome-terminal-server)
    send_key="g"
    ;;
  Code)
    send_key="C"
    ;;
  google-chrome)
    send_key="g"
    ;;
  *)
    send_key=""
    ;;
esac
ydotool key 38:1 key 38:0 # just test for now
# ydotool keydown Shift_L keydown Shift_L keydown Shift_L
# ydotool key "$send_key"
# ydotool keyup Shift_L keyup Shift_L keyup Shift_L
