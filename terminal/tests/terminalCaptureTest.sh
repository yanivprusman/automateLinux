#!/bin/bash
# Test script for terminal capture with index-based design
# Run with: bash terminal/tests/terminalCaptureTest.sh

set -e
source "${AUTOMATE_LINUX_DIR:-/opt/automateLinux}/terminal/functions/terminalCapture.sh"

TEST_DIR="/tmp/terminalCaptureTest_$$"
TEST_LOG="$TEST_DIR/test.log"
TEST_INDEX="$TEST_DIR/test.index"
PASSED=0
FAILED=0

setup() {
    mkdir -p "$TEST_DIR"
    : > "$TEST_LOG"
    : > "$TEST_INDEX"
}

teardown() {
    rm -rf "$TEST_DIR"
}

pass() { ((PASSED++)); echo "  ✓ $1"; }
fail() { ((FAILED++)); echo "  ✗ $1: $2"; }

# Simulate writing output to log
write_output() {
    printf '%b' "$1" >> "$TEST_LOG"
}

# Simulate prompt (write index entry)
write_prompt() {
    local offset=$(stat -c%s "$TEST_LOG" 2>/dev/null || echo 0)
    echo "{\"ts\":$(date +%s),\"offset\":$offset}" >> "$TEST_INDEX"
}

# Get output between two prompts using index
get_command_output() {
    local cmd_num="${1:-1}"  # 1 = most recent
    local index_lines
    mapfile -t index_lines < "$TEST_INDEX"
    local count=${#index_lines[@]}

    [ $count -lt 2 ] && { echo ""; return 1; }

    local end_idx=$((count - cmd_num))
    local start_idx=$((end_idx - 1))

    [ $start_idx -lt 0 ] && { echo ""; return 1; }

    local start_offset=$(echo "${index_lines[$start_idx]}" | sed 's/.*"offset":\([0-9]*\).*/\1/')
    local end_offset=$(echo "${index_lines[$end_idx]}" | sed 's/.*"offset":\([0-9]*\).*/\1/')
    local len=$((end_offset - start_offset))

    dd if="$TEST_LOG" bs=1 skip="$start_offset" count="$len" 2>/dev/null
}

#--- Tests ---

test_simple_output() {
    echo "Test: Simple output"
    setup

    write_prompt
    write_output "hello world\n"
    write_prompt

    local result=$(get_command_output 1)
    if [ "$result" = "hello world"$'\n' ]; then
        pass "Simple output captured"
    else
        fail "Simple output" "got '$result'"
    fi
}

test_multiline_output() {
    echo "Test: Multi-line output"
    setup

    write_prompt
    write_output "line1\nline2\nline3\n"
    write_prompt

    local result=$(get_command_output 1)
    local expected="line1"$'\n'"line2"$'\n'"line3"$'\n'
    if [ "$result" = "$expected" ]; then
        pass "Multi-line output captured"
    else
        fail "Multi-line" "got '$result'"
    fi
}

test_escape_sequences() {
    echo "Test: Escape sequences preserved in log"
    setup

    write_prompt
    write_output $'\x1b[31mred\x1b[0m\n'
    write_prompt

    local result=$(get_command_output 1)
    # Raw result should contain escapes
    if [[ "$result" == *$'\x1b'* ]]; then
        pass "Escapes preserved in raw output"
    else
        fail "Escapes" "escapes were stripped"
    fi

    # Stripped result should be clean
    local stripped=$(echo -n "$result" | stripEscapeSequences)
    if [ "$stripped" = "red" ]; then
        pass "Escapes stripped correctly"
    else
        fail "Strip escapes" "got '$stripped'"
    fi
}

test_empty_output() {
    echo "Test: Empty output between prompts"
    setup

    write_prompt
    write_prompt  # No output between

    local result=$(get_command_output 1)
    if [ -z "$result" ]; then
        pass "Empty output handled"
    else
        fail "Empty output" "got '$result'"
    fi
}

test_multiple_commands() {
    echo "Test: Multiple commands"
    setup

    write_prompt
    write_output "first\n"
    write_prompt
    write_output "second\n"
    write_prompt
    write_output "third\n"
    write_prompt

    local r1=$(get_command_output 1)
    local r2=$(get_command_output 2)
    local r3=$(get_command_output 3)

    if [ "$r1" = "third"$'\n' ]; then
        pass "Command 1 (most recent) correct"
    else
        fail "Command 1" "got '$r1'"
    fi

    if [ "$r2" = "second"$'\n' ]; then
        pass "Command 2 correct"
    else
        fail "Command 2" "got '$r2'"
    fi

    if [ "$r3" = "first"$'\n' ]; then
        pass "Command 3 correct"
    else
        fail "Command 3" "got '$r3'"
    fi
}

test_carriage_return() {
    echo "Test: Carriage return in output"
    setup

    write_prompt
    write_output "before\rafter\n"
    write_prompt

    local result=$(get_command_output 1)
    # Raw should have CR
    if [[ "$result" == *$'\r'* ]]; then
        pass "CR preserved in raw"
    else
        fail "CR preserved" "CR was lost"
    fi

    # Stripped should not
    local stripped=$(echo -n "$result" | stripEscapeSequences)
    if [[ "$stripped" != *$'\r'* ]]; then
        pass "CR stripped correctly"
    else
        fail "CR strip" "CR remained"
    fi
}

test_binary_like_content() {
    echo "Test: Binary-like content"
    setup

    write_prompt
    write_output "has \x00 null and \x1b escapes\n"
    write_prompt

    local result=$(get_command_output 1)
    if [ -n "$result" ]; then
        pass "Binary-like content captured"
    else
        fail "Binary content" "empty result"
    fi
}

test_large_output() {
    echo "Test: Large output (1000 lines)"
    setup

    write_prompt
    for i in $(seq 1 1000); do
        write_output "line $i\n"
    done
    write_prompt

    local result=$(get_command_output 1)
    local line_count=$(echo "$result" | wc -l)
    if [ "$line_count" -eq 1000 ]; then
        pass "Large output: 1000 lines captured"
    else
        fail "Large output" "got $line_count lines"
    fi
}

#--- Run all tests ---

echo "=== Terminal Capture Tests ==="
echo ""

test_simple_output
test_multiline_output
test_escape_sequences
test_empty_output
test_multiple_commands
test_carriage_return
test_binary_like_content
test_large_output

teardown

echo ""
echo "=== Results: $PASSED passed, $FAILED failed ==="
[ $FAILED -eq 0 ] && exit 0 || exit 1
