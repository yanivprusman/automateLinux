alias mk='mkdir'
alias boot='history -a && sudo reboot'
alias pkill='sudo pkill -9'
alias pkillev='sudo pkill -9 evsieve'
alias evsievep='sudo evsieve --input /dev/input/event* --print format=direct'
alias 1restart='restartCorsairKeyBoardLogiMouseService.sh reset'
alias 1stop='stopCorsairKeyBoardLogiMouseService.sh'

# sudo $(realpath "$(dirname "$(realpath "${BASH_SOURCE[0]}")")/_sudoStop.sh")

theRealPath() {
  local path="$1"
  # Remove leading slash and any ../ if present
  path="${path#/}"
  path="${path#../}"
  # Use the workspace root directory
  local workspace_root="/home/yaniv/coding/automateLinux"
  # Remove any trailing whitespace
  path="$(echo -n "${path}" | tr -d '[:space:]')"
  echo "$(realpath "${workspace_root}/${path}")"
}
export -f theRealPath