#!/bin/bash
# Integration test for terminalToClipboard (ttc/ttcc) - uses real clipboard
# Run with: bash terminal/tests/testTerminalToClipboardReal.sh
#
# Requires: xclip installed and working X11/Wayland session

TEST_DIR="/tmp/terminalToClipboardTest_$$"
CAPTURE_FILE="$TEST_DIR/capture"
PASSED=0
FAILED=0

# Check xclip availability
if ! command -v xclip &>/dev/null; then
    echo "ERROR: xclip not found. Install with: sudo apt install xclip"
    exit 1
fi

# Test clipboard access
if ! echo "test" | xclip -selection clipboard 2>/dev/null; then
    echo "ERROR: Cannot access clipboard. Is DISPLAY/WAYLAND_DISPLAY set?"
    exit 1
fi

# Global setup
global_setup() {
    mkdir -p "$TEST_DIR"
    export AUTOMATE_LINUX_TERMINAL_CAPTURE_FILE="$CAPTURE_FILE"
    source "${AUTOMATE_LINUX_DIR:-/opt/automateLinux}/terminal/functions/terminalCapture.sh"
}

reset_capture() {
    : > "$CAPTURE_FILE"
    # Clear clipboard
    echo -n "" | xclip -selection clipboard
}

teardown() {
    rm -rf "$TEST_DIR"
}

pass() { ((PASSED++)); echo "  ✓ $1"; }
fail() { ((FAILED++)); echo "  ✗ $1: $2"; }

write_prompt() {
    local ts="${1:-$(date +%s)}"
    echo "---PROMPT[timestamp:$ts]---" >> "$CAPTURE_FILE"
}

write_output() {
    printf '%b' "$1" >> "$CAPTURE_FILE"
}

get_clipboard() {
    xclip -selection clipboard -o 2>/dev/null
}

#=============================================================================
# TESTS
#=============================================================================

test_last_line() {
    echo "Test: Last line copied to real clipboard"
    reset_capture

    write_prompt 1000
    write_output "first line\n"
    write_output "second line\n"
    write_output "last line\n"
    write_prompt 2000

    terminalToClipboard
    local result=$(get_clipboard)

    if [ "$result" = "last line" ]; then
        pass "Last line in clipboard"
    else
        fail "Last line" "expected 'last line', got '$result'"
    fi
}

test_line_n() {
    echo "Test: Line N from end"
    reset_capture

    write_prompt 1000
    write_output "line A\n"
    write_output "line B\n"
    write_output "line C\n"
    write_prompt 2000

    terminalToClipboard 2
    local result=$(get_clipboard)

    if [ "$result" = "line B" ]; then
        pass "Line 2 from end in clipboard"
    else
        fail "Line N" "expected 'line B', got '$result'"
    fi
}

test_range() {
    echo "Test: Line range N-M"
    reset_capture

    write_prompt 1000
    write_output "one\n"
    write_output "two\n"
    write_output "three\n"
    write_prompt 2000

    terminalToClipboard 3-1
    local result=$(get_clipboard)
    local expected="one
two
three"

    if [ "$result" = "$expected" ]; then
        pass "Range in clipboard"
    else
        fail "Range" "expected '$expected', got '$result'"
    fi
}

test_from_prompt() {
    echo "Test: fromPrompt (ttcc)"
    reset_capture

    write_prompt 1000
    write_output "command output line 1\n"
    write_output "command output line 2\n"
    write_prompt 2000

    terminalToClipboard fromPrompt
    local result=$(get_clipboard)
    local expected="command output line 1
command output line 2"

    if [ "$result" = "$expected" ]; then
        pass "fromPrompt output in clipboard"
    else
        fail "fromPrompt" "expected '$expected', got '$result'"
    fi
}

test_from_prompt_n() {
    echo "Test: fromPrompt N"
    reset_capture

    write_prompt 1000
    write_output "cmd1 output\n"
    write_prompt 2000
    write_output "cmd2 output\n"
    write_prompt 3000
    write_output "cmd3 output\n"
    write_prompt 4000

    # Most recent
    terminalToClipboard fromPrompt
    local result=$(get_clipboard)
    if [ "$result" = "cmd3 output" ]; then
        pass "fromPrompt 1 (most recent)"
    else
        fail "fromPrompt 1" "expected 'cmd3 output', got '$result'"
    fi

    # Second most recent
    terminalToClipboard fromPrompt 2
    result=$(get_clipboard)
    if [ "$result" = "cmd2 output" ]; then
        pass "fromPrompt 2"
    else
        fail "fromPrompt 2" "expected 'cmd2 output', got '$result'"
    fi

    # Third (oldest)
    terminalToClipboard fromPrompt 3
    result=$(get_clipboard)
    if [ "$result" = "cmd1 output" ]; then
        pass "fromPrompt 3"
    else
        fail "fromPrompt 3" "expected 'cmd1 output', got '$result'"
    fi
}

test_escape_strip() {
    echo "Test: ANSI escapes stripped"
    reset_capture

    write_prompt 1000
    write_output $'\x1b[31mred text\x1b[0m\n'
    write_prompt 2000

    terminalToClipboard
    local result=$(get_clipboard)

    if [ "$result" = "red text" ]; then
        pass "Escapes stripped in clipboard"
    else
        fail "Escape strip" "expected 'red text', got '$result'"
    fi
}

test_multiline_preserve() {
    echo "Test: Multiline content preserved"
    reset_capture

    write_prompt 1000
    write_output "line 1\n"
    write_output "line 2\n"
    write_output "line 3\n"
    write_prompt 2000

    terminalToClipboard fromPrompt
    local result=$(get_clipboard)
    local line_count=$(echo "$result" | wc -l)

    if [ "$line_count" -eq 3 ]; then
        pass "3 lines preserved in clipboard"
    else
        fail "Multiline" "expected 3 lines, got $line_count"
    fi
}

test_special_chars() {
    echo "Test: Special characters preserved"
    reset_capture

    write_prompt 1000
    write_output 'echo "hello $USER" | grep -E "^[a-z]+$"\n'
    write_prompt 2000

    terminalToClipboard
    local result=$(get_clipboard)
    local expected='echo "hello $USER" | grep -E "^[a-z]+$"'

    if [ "$result" = "$expected" ]; then
        pass "Special chars preserved"
    else
        fail "Special chars" "got '$result'"
    fi
}

#=============================================================================
# Run tests
#=============================================================================

global_setup

echo "=== terminalToClipboard Real Clipboard Tests ==="
echo ""

test_last_line
test_line_n
test_range
test_from_prompt
test_from_prompt_n
test_escape_strip
test_multiline_preserve
test_special_chars

teardown

echo ""
echo "=== Results: $PASSED passed, $FAILED failed ==="
[ $FAILED -eq 0 ] && exit 0 || exit 1
