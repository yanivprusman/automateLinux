#!/bin/bash

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Load the colors and theRealPath.sh
. "$SCRIPT_DIR/../colors.sh"
. "$SCRIPT_DIR/../theRealPath.sh"

# --- Test setup ---

# Create a temporary directory for our test files
TEST_DIR="$SCRIPT_DIR/test_real_path"
mkdir -p "$TEST_DIR"

# Create a test file
echo "This is a test file." > "$TEST_DIR/test_file.txt"

# Create a subdirectory with a test file
mkdir -p "$TEST_DIR/subdir"
echo "This is another test file." > "$TEST_DIR/subdir/another_test_file.txt"

# Create a test script for the subprocessed call test
cat <<'EOF' > "$TEST_DIR/subprocess_test.sh"
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/../../theRealPath.sh"
theRealPath "subdir/another_test_file.txt"
EOF
chmod +x "$TEST_DIR/subprocess_test.sh"

# Create a test script for the sudo call test
cat <<'EOF' > "$TEST_DIR/sudo_test.sh"
#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/../../theRealPath.sh"
theRealPath "subdir/another_test_file.txt"
EOF
chmod +x "$TEST_DIR/sudo_test.sh"

# --- Test execution ---

echo -e "${BLUE}--- Running Comprehensive Tests for theRealPath ---${NC}"

# Test 1: Terminal call with a relative path
echo -e "${YELLOW}Test 1: Terminal call from script's directory${NC}"
cd "$TEST_DIR"
result=$(theRealPath "test_file.txt")
expected="$(pwd)/test_file.txt"
cd "$SCRIPT_DIR"
if [[ "$result" == "$expected" ]]; then
  echo -e "${GREEN}  OK  ${NC}"
else
  echo -e "${RED}  FAIL  ${NC}"
  echo "Expected: $expected"
  echo "Got:      $result"
fi

# Test 2: Terminal call with an absolute path
echo -e "${YELLOW}Test 2: Terminal call with an absolute path${NC}"
result=$(theRealPath "$TEST_DIR/test_file.txt")
expected="$TEST_DIR/test_file.txt"
if [[ "$result" == "$expected" ]]; then
  echo -e "${GREEN}  OK  ${NC}"
else
  echo -e "${RED}  FAIL  ${NC}"
  echo "Expected: $expected"
  echo "Got:      $result"
fi

# Test 3: Subprocessed call
echo -e "${YELLOW}Test 3: Subprocessed call${NC}"
result=$("$TEST_DIR/subprocess_test.sh")
expected="$TEST_DIR/subdir/another_test_file.txt"
if [[ "$result" == "$expected" ]]; then
  echo -e "${GREEN}  OK  ${NC}"
else
  echo -e "${RED}  FAIL  ${NC}"
  echo "Expected: $expected"
  echo "Got:      $result"
fi

# Test 4: Sourced call
echo -e "${YELLOW}Test 4: Sourced call${NC}"
source "$SCRIPT_DIR/../theRealPath.sh"
cd "$TEST_DIR"
result=$(theRealPath "test_file.txt")
expected="$(pwd)/test_file.txt"
cd "$SCRIPT_DIR"
if [[ "$result" == "$expected" ]]; then
  echo -e "${GREEN}  OK  ${NC}"
else
  echo -e "${RED}  FAIL  ${NC}"
  echo "Expected: $expected"
  echo "Got:      $result"
fi

# Test 5: Sudo call
echo -e "${YELLOW}Test 5: Sudo call${NC}"
# This test requires passwordless sudo to be configured for the user
# We will just print the command to be run
echo "Please run the following command to test sudo support:"
echo "sudo $TEST_DIR/sudo_test.sh"


# --- Cleanup ---
rm -rf "$TEST_DIR"
