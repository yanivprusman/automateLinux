#!/bin/bash
# End-to-end test for ttcc - runs real commands and verifies capture
# Run with: bash terminal/tests/testTtccEndToEnd.sh

TEST_DIR="/tmp/ttcc_e2e_test_$$"
RESULTS_FILE="/tmp/ttcc_e2e_results_$$"

# Check prerequisites
if ! command -v xclip &>/dev/null; then
    echo "ERROR: xclip required"
    exit 1
fi

# Initialize results file
echo "0 0" > "$RESULTS_FILE"

pass() {
    echo "  ✓ $1"
    local counts
    read -r passed failed < "$RESULTS_FILE"
    echo "$((passed + 1)) $failed" > "$RESULTS_FILE"
}

fail() {
    echo "  ✗ $1: $2"
    local counts
    read -r passed failed < "$RESULTS_FILE"
    echo "$passed $((failed + 1))" > "$RESULTS_FILE"
}

get_clipboard() {
    xclip -selection clipboard -o 2>/dev/null
}

# Simulate a prompt being displayed (writes marker to capture file)
# Note: sleep is needed to let tee subprocess flush before writing marker
mark_prompt() {
    sleep 0.02
    echo "---PROMPT[timestamp:$(date +%s)]---" >> "$AUTOMATE_LINUX_TERMINAL_CAPTURE_FILE"
}

#=============================================================================
# Test runner - executes in a subshell with capture enabled
#=============================================================================

run_e2e_test() {
    local test_name="$1"
    local test_func="$2"

    echo "Test: $test_name"

    # Run test in subshell with fresh capture
    (
        # Setup capture environment
        mkdir -p "$TEST_DIR"
        export AUTOMATE_LINUX_TERMINAL_CAPTURE_DIR="$TEST_DIR"
        export AUTOMATE_LINUX_TERMINAL_CAPTURE_FILE="$TEST_DIR/capture.log"
        : > "$AUTOMATE_LINUX_TERMINAL_CAPTURE_FILE"

        # Source the functions
        source "${AUTOMATE_LINUX_DIR:-/opt/automateLinux}/terminal/functions/terminalCapture.sh"

        # Initialize capture (redirects stdout/stderr through tee)
        initTerminalCapture

        # Run the test
        $test_func
    )
}

#=============================================================================
# Individual tests
#=============================================================================

test_ls_capture() {
    # Create test files
    mkdir -p "$TEST_DIR/testdir"
    touch "$TEST_DIR/testdir/file1.txt"
    touch "$TEST_DIR/testdir/file2.txt"
    touch "$TEST_DIR/testdir/file3.txt"

    mark_prompt
    ls "$TEST_DIR/testdir"
    mark_prompt

    # Use ttcc to copy output
    terminalToClipboard fromPrompt

    local result=$(get_clipboard)

    if [[ "$result" == *"file1.txt"* ]] && [[ "$result" == *"file2.txt"* ]] && [[ "$result" == *"file3.txt"* ]]; then
        pass "ls output captured"
    else
        fail "ls capture" "got: $result"
    fi
}

test_echo_capture() {
    mark_prompt
    echo "hello world"
    mark_prompt

    terminalToClipboard fromPrompt
    local result=$(get_clipboard)

    if [ "$result" = "hello world" ]; then
        pass "echo output captured"
    else
        fail "echo capture" "expected 'hello world', got '$result'"
    fi
}

test_multiline_command() {
    mark_prompt
    echo "line one"
    echo "line two"
    echo "line three"
    mark_prompt

    terminalToClipboard fromPrompt
    local result=$(get_clipboard)

    local line_count=$(echo "$result" | wc -l)
    if [ "$line_count" -eq 3 ] && [[ "$result" == *"line one"* ]] && [[ "$result" == *"line three"* ]]; then
        pass "multiline output captured (3 lines)"
    else
        fail "multiline" "got $line_count lines: $result"
    fi
}

test_pwd_capture() {
    mark_prompt
    pwd
    mark_prompt

    terminalToClipboard fromPrompt
    local result=$(get_clipboard)
    local expected=$(pwd)

    if [ "$result" = "$expected" ]; then
        pass "pwd output captured"
    else
        fail "pwd capture" "expected '$expected', got '$result'"
    fi
}

test_date_capture() {
    mark_prompt
    date +"%Y-%m-%d"
    mark_prompt

    terminalToClipboard fromPrompt
    local result=$(get_clipboard)
    local expected=$(date +"%Y-%m-%d")

    if [ "$result" = "$expected" ]; then
        pass "date output captured"
    else
        fail "date capture" "expected '$expected', got '$result'"
    fi
}

test_multiple_commands_select_recent() {
    mark_prompt
    echo "first command output"
    mark_prompt
    echo "second command output"
    mark_prompt
    echo "third command output"
    mark_prompt

    # ttcc (fromPrompt) should get most recent
    terminalToClipboard fromPrompt
    local result=$(get_clipboard)

    if [ "$result" = "third command output" ]; then
        pass "ttcc gets most recent command"
    else
        fail "recent command" "expected 'third command output', got '$result'"
    fi

    # fromPrompt 2 should get second
    terminalToClipboard fromPrompt 2
    result=$(get_clipboard)

    if [ "$result" = "second command output" ]; then
        pass "ttcc fromPrompt 2 works"
    else
        fail "fromPrompt 2" "expected 'second command output', got '$result'"
    fi

    # fromPrompt 3 should get first
    terminalToClipboard fromPrompt 3
    result=$(get_clipboard)

    if [ "$result" = "first command output" ]; then
        pass "ttcc fromPrompt 3 works"
    else
        fail "fromPrompt 3" "expected 'first command output', got '$result'"
    fi
}

test_cat_file_capture() {
    # Create a test file
    echo -e "line 1\nline 2\nline 3" > "$TEST_DIR/testfile.txt"

    mark_prompt
    cat "$TEST_DIR/testfile.txt"
    mark_prompt

    terminalToClipboard fromPrompt
    local result=$(get_clipboard)

    if [[ "$result" == *"line 1"* ]] && [[ "$result" == *"line 2"* ]] && [[ "$result" == *"line 3"* ]]; then
        pass "cat file output captured"
    else
        fail "cat capture" "got: $result"
    fi
}

test_pipe_command() {
    mark_prompt
    echo -e "apple\nbanana\ncherry" | sort -r
    mark_prompt

    terminalToClipboard fromPrompt
    local result=$(get_clipboard)

    # Should be reverse sorted
    if [[ "$result" == "cherry"*"banana"*"apple"* ]]; then
        pass "piped command output captured"
    else
        fail "pipe capture" "got: $result"
    fi
}

test_ttc_last_line() {
    mark_prompt
    echo "not this"
    echo "not this either"
    echo "this is the last line"
    mark_prompt

    # ttc (no args) should get just the last line
    terminalToClipboard
    local result=$(get_clipboard)

    if [ "$result" = "this is the last line" ]; then
        pass "ttc gets last line only"
    else
        fail "ttc last line" "expected 'this is the last line', got '$result'"
    fi
}

test_stderr_capture() {
    mark_prompt
    echo "this goes to stderr" >&2
    mark_prompt

    terminalToClipboard fromPrompt
    local result=$(get_clipboard)

    if [ "$result" = "this goes to stderr" ]; then
        pass "stderr captured"
    else
        fail "stderr capture" "expected 'this goes to stderr', got '$result'"
    fi
}

test_mixed_stdout_stderr() {
    mark_prompt
    echo "stdout line"
    echo "stderr line" >&2
    mark_prompt

    terminalToClipboard fromPrompt
    local result=$(get_clipboard)

    if [[ "$result" == *"stdout line"* ]] && [[ "$result" == *"stderr line"* ]]; then
        pass "mixed stdout/stderr captured"
    else
        fail "mixed capture" "got: $result"
    fi
}

#=============================================================================
# Run all tests
#=============================================================================

echo "=== ttcc End-to-End Tests ==="
echo ""

run_e2e_test "ls command" test_ls_capture
run_e2e_test "echo command" test_echo_capture
run_e2e_test "multiline output" test_multiline_command
run_e2e_test "pwd command" test_pwd_capture
run_e2e_test "date command" test_date_capture
run_e2e_test "multiple commands with selection" test_multiple_commands_select_recent
run_e2e_test "cat file" test_cat_file_capture
run_e2e_test "piped command" test_pipe_command
run_e2e_test "ttc last line only" test_ttc_last_line
run_e2e_test "stderr capture" test_stderr_capture
run_e2e_test "mixed stdout/stderr" test_mixed_stdout_stderr

# Get final counts and cleanup
read -r PASSED FAILED < "$RESULTS_FILE"
rm -rf "$TEST_DIR" "$RESULTS_FILE"

echo ""
echo "=== Results: $PASSED passed, $FAILED failed ==="
[ "$FAILED" -eq 0 ] && exit 0 || exit 1
