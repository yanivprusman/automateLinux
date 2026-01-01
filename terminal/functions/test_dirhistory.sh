BASE_DIR_DIRHISTORY_TEST="/home/yaniv/.gemini/tmp/719946dbf0ba00e3ab7d64db7933288812b0b2cfc38b201e2eec2e375afa3d74/dirhistory_test"

function test_dirhistory() {
  mkdir -p "$BASE_DIR_DIRHISTORY_TEST"
  echo "Created base directory for testing: $BASE_DIR_DIRHISTORY_TEST"

  for i in $(seq 1 100); do
    local new_dir="$BASE_DIR_DIRHISTORY_TEST/dir_$i"
    mkdir -p "$new_dir"
    cd "$new_dir"
    echo "Changed to directory: $(pwd)"
  done

  echo "Finished creating test directories for dirhistory."
}

function cleanup_dirhistory_test() {
  if [ -d "$BASE_DIR_DIRHISTORY_TEST" ]; then
    echo "Cleaning up test directories: $BASE_DIR_DIRHISTORY_TEST"
    rm -rf "$BASE_DIR_DIRHISTORY_TEST"
    echo "Cleaned up test directories."
  else
    echo "Test directories not found at $BASE_DIR_DIRHISTORY_TEST, no cleanup needed."
  fi
}
