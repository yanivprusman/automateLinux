#!/bin/bash

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" &> /dev/null && pwd)"
TEST_DIR="$SCRIPT_DIR/test_workspace"
REAL_PATH_BIN="$SCRIPT_DIR/../theRealPath"

# Colors for test output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Test counter
TESTS_RUN=0
TESTS_PASSED=0

# Test helper function
run_test() {
    local test_name="$1"
    local expected="$2"
    local cmd="$3"
    
    ((TESTS_RUN++))
    echo -n "Testing $test_name... "
    
    # Run the command and capture output
    local result
    result="$(eval "$cmd")"
    
    if [ "$result" = "$expected" ]; then
        echo -e "${GREEN}PASSED${NC}"
        ((TESTS_PASSED++))
    else
        echo -e "${RED}FAILED${NC}"
        echo "  Expected: $expected"
        echo "  Got:      $result"
    fi
}

# Setup test environment
setup_test_env() {
    echo "Setting up test environment..."
    
    # Clean up any existing test workspace
    rm -rf "$TEST_DIR"
    mkdir -p "$TEST_DIR"
    
    # Create test directory structure
    mkdir -p "$TEST_DIR/dir1/subdir1"
    mkdir -p "$TEST_DIR/dir2/subdir2"
    
    # Create some regular files
    echo "test file 1" > "$TEST_DIR/dir1/file1.txt"
    echo "test file 2" > "$TEST_DIR/dir2/file2.txt"
    
    # Create various types of symlinks
    # Relative symlinks
    cd "$TEST_DIR"
    ln -s "dir1/file1.txt" "relative_link1"
    ln -s "../test_workspace/dir2/file2.txt" "relative_link2"
    ln -s "dir1" "dir_link"
    
    # Absolute symlinks
    ln -s "$TEST_DIR/dir2/file2.txt" "absolute_link"
    
    # Nested symlinks
    ln -s "relative_link1" "nested_link"
    
    # Circular symlinks
    ln -s "circular2" "circular1"
    ln -s "circular1" "circular2"
    
    # Create a deep symlink chain
    ln -s "chain2" "chain1"
    ln -s "chain3" "chain2"
    ln -s "dir1/file1.txt" "chain3"
    
    cd - > /dev/null
}

# Run the tests
run_tests() {
    local curr_dir="$PWD"
    
    # Test 1: Basic file path resolution
    run_test "Basic file path" \
        "$TEST_DIR/dir1/file1.txt" \
        "$REAL_PATH_BIN $TEST_DIR/dir1/file1.txt"
    
    # Test 2: Simple relative path
    cd "$TEST_DIR"
    run_test "Relative path" \
        "$TEST_DIR/dir1/file1.txt" \
        "$REAL_PATH_BIN ./dir1/file1.txt"
    cd "$curr_dir"
    
    # Test 3: Relative symlink
    run_test "Relative symlink" \
        "$TEST_DIR/dir1/file1.txt" \
        "$REAL_PATH_BIN $TEST_DIR/relative_link1"
    
    # Test 4: Absolute symlink
    run_test "Absolute symlink" \
        "$TEST_DIR/dir2/file2.txt" \
        "$REAL_PATH_BIN $TEST_DIR/absolute_link"
    
    # Test 5: Nested symlink
    run_test "Nested symlink" \
        "$TEST_DIR/dir1/file1.txt" \
        "$REAL_PATH_BIN $TEST_DIR/nested_link"
    
    # Test 6: Directory symlink
    run_test "Directory symlink" \
        "$TEST_DIR/dir1" \
        "$REAL_PATH_BIN $TEST_DIR/dir_link"
    
    # Test 7: Path with '../'
    cd "$TEST_DIR/dir1"
    run_test "Path with ../" \
        "$TEST_DIR/dir2/file2.txt" \
        "$REAL_PATH_BIN ../dir2/file2.txt"
    cd "$curr_dir"
    
    # Test 8: Deep symlink chain
    run_test "Deep symlink chain" \
        "$TEST_DIR/dir1/file1.txt" \
        "$REAL_PATH_BIN $TEST_DIR/chain1"
    
    # Test 9: Non-existent file
    run_test "Non-existent file" \
        "" \
        "$REAL_PATH_BIN $TEST_DIR/does_not_exist"
    
    # Test 10: Circular symlinks (should handle gracefully)
    run_test "Circular symlinks" \
        "" \
        "$REAL_PATH_BIN $TEST_DIR/circular1"
    
    # Test 11: Path containing spaces
    mkdir -p "$TEST_DIR/space dir"
    touch "$TEST_DIR/space dir/space file.txt"
    run_test "Path with spaces" \
        "$TEST_DIR/space dir/space file.txt" \
        "$REAL_PATH_BIN '$TEST_DIR/space dir/space file.txt'"
    
    # Test 12: Double dot in middle of path
    run_test "Double dot in middle" \
        "$TEST_DIR/dir2/file2.txt" \
        "$REAL_PATH_BIN $TEST_DIR/dir1/../dir2/file2.txt"
    
    # Test 13: Multiple symlinks in path
    mkdir -p "$TEST_DIR/multi/level/dir"
    ln -s "multi" "$TEST_DIR/link1"
    ln -s "level" "$TEST_DIR/multi/link2"
    ln -s "dir" "$TEST_DIR/multi/level/link3"
    run_test "Multiple symlinks in path" \
        "$TEST_DIR/multi/level/dir" \
        "$REAL_PATH_BIN $TEST_DIR/link1/link2/link3"
        
    # Test 14: Script referenced through symlink from different directory
    mkdir -p "$TEST_DIR/scripts/bin"
    echo '#!/bin/bash\necho "test script"' > "$TEST_DIR/scripts/test_script.sh"
    chmod +x "$TEST_DIR/scripts/test_script.sh"
    cd "$TEST_DIR/scripts/bin"
    ln -s "../test_script.sh" "script_link.sh"
    echo '#!/bin/bash\n'"$REAL_PATH_BIN"' ../another_script.sh' > "$TEST_DIR/scripts/bin/caller_script.sh"
    chmod +x "$TEST_DIR/scripts/bin/caller_script.sh"
    touch "$TEST_DIR/scripts/another_script.sh"
    run_test "Script referenced through symlink from different directory" \
        "$TEST_DIR/scripts/another_script.sh" \
        "cd $TEST_DIR/scripts/bin && $REAL_PATH_BIN ../another_script.sh"
    cd "$curr_dir"
    
    # Test 15: Path resolution with sudo
    # Create a test script that uses sudo
    echo '#!/bin/bash\n'"$REAL_PATH_BIN"' "$TEST_DIR/target_file.txt"' > "$TEST_DIR/sudo_test.sh"
    chmod +x "$TEST_DIR/sudo_test.sh"
    touch "$TEST_DIR/target_file.txt"
    run_test "Path resolution with sudo" \
        "$TEST_DIR/target_file.txt" \
        "sudo $REAL_PATH_BIN $TEST_DIR/target_file.txt"
        
    # Test 16: Complex symlink chain with relative paths
    mkdir -p "$TEST_DIR/chain_test/a/b/c"
    echo "test content" > "$TEST_DIR/chain_test/target.txt"
    cd "$TEST_DIR/chain_test"
    # Create absolute symlinks to avoid relative path issues
    ln -s "$TEST_DIR/chain_test/target.txt" "a/link1"
    ln -s "$TEST_DIR/chain_test/a/link1" "a/b/link2"
    ln -s "$TEST_DIR/chain_test/a/b/link2" "a/b/c/link3"
    run_test "Complex symlink chain with relative paths" \
        "$TEST_DIR/chain_test/target.txt" \
        "cd $TEST_DIR/chain_test/a/b/c && $REAL_PATH_BIN ./link3"
    cd "$curr_dir"
}

# Run cleanup on script exit
cleanup() {
    echo "Cleaning up test environment..."
    rm -rf "$TEST_DIR"
}
trap cleanup EXIT

# Main execution
echo "Starting theRealPath tests..."
setup_test_env
run_tests

# Print summary
echo
echo "Test Summary:"
echo "Tests run:    $TESTS_RUN"
echo "Tests passed: $TESTS_PASSED"
if [ "$TESTS_RUN" -eq "$TESTS_PASSED" ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi