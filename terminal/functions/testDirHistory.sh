BASE_DIR_DIRHISTORY_TEST="/tmp/dirhistory_test"

function testDirHistory() {
  local count=100
  [ -n "$1" ] && count=$1

  mkdir -p "$BASE_DIR_DIRHISTORY_TEST"
  echo "--- Starting DirHistory Population Test ($count entries) ---"
  echo "Base directory: $BASE_DIR_DIRHISTORY_TEST"

  for i in $(seq 1 "$count"); do
    local new_dir="$BASE_DIR_DIRHISTORY_TEST/dir_$i"
    mkdir -p "$new_dir"
    cd "$new_dir"
    # The prompt command or manual cd will trigger the daemon update.
    # Since we are in a script, we might need to manually trigger if PROMPT_COMMAND isn't running
    if [[ $(type -t daemon) == "function" || -x $(which daemon) ]]; then
        daemon send updateDirHistory --tty "$AUTOMATE_LINUX_TTY_NUMBER" --pwd "${PWD}/" > /dev/null
    fi
  done

  echo "Finished populating history."
  
  # Verify via showDb
  local db_script="${AUTOMATE_LINUX_DAEMON_DIR}showDb.sh"
  if [ -f "$db_script" ]; then
    echo "Verifying row count in database..."
    "$db_script" terminal_history | grep -c "^$AUTOMATE_LINUX_TTY_NUMBER"
  fi
}

function testNavigation() {
  local steps=50
  [ -n "$1" ] && steps=$1
  
  echo "--- Starting Navigation Stress Test ($steps steps) ---"
  
  echo "Moving BACKWARD $steps times..."
  for i in $(seq 1 "$steps"); do
    local cmd=$(daemon send cdBackward --tty "$AUTOMATE_LINUX_TTY_NUMBER")
    eval "$cmd"
  done
  echo "Current Position: $PWD"

  echo "Moving FORWARD $steps times..."
  for i in $(seq 1 "$steps"); do
    local cmd=$(daemon send cdForward --tty "$AUTOMATE_LINUX_TTY_NUMBER")
    eval "$cmd"
  done
  echo "Test complete. Final Position: $PWD"
}

function cleanupDirHistoryTest() {
  if [ -d "$BASE_DIR_DIRHISTORY_TEST" ]; then
    echo "Cleaning up test directories: $BASE_DIR_DIRHISTORY_TEST"
    rm -rf "$BASE_DIR_DIRHISTORY_TEST"
    echo "Cleaned up test directories."
  else
    echo "Test directories not found at $BASE_DIR_DIRHISTORY_TEST, no cleanup needed."
  fi
}
