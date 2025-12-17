#!/bin/bash

# Get the directory of this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
. "$SCRIPT_DIR/../colors.sh"

# Check if -debug was passed to the test script
DEBUG_FLAG=""
if [[ "$1" == "-debug" ]]; then
    DEBUG_FLAG="-debug"
fi

echo -e "${BLUE}--- Running Sanity Tests for theRealPath ---${NC}"

# --- Test setup ---
# It's important to run these tests from the project root, as that's the typical user environment
cd "$SCRIPT_DIR/../.." 

# --- Terminal Tests ---
echo -e "${YELLOW}--- Terminal Tests ---${NC}"

echo -e "${CYAN}1. theRealPath (no args)${NC}"
# Use a subshell to capture both stdout and stderr
(theRealPath $DEBUG_FLAG) 2>&1

echo -e "${CYAN}2. theRealPath go.sh${NC}"
(theRealPath $DEBUG_FLAG go.sh) 2>&1

echo -e "${CYAN}3. theRealPath /etc/hosts${NC}"
(theRealPath $DEBUG_FLAG /etc/hosts) 2>&1

# --- Sudo Tests ---
echo -e "${YELLOW}--- Sudo Tests ---${NC}"
echo "Note: Sudo tests may require a password."

echo -e "${CYAN}1. sudo theRealPath (no args)${NC}"
(sudo theRealPath $DEBUG_FLAG) 2>&1

echo -e "${CYAN}2. sudo theRealPath go.sh${NC}"
(sudo theRealPath $DEBUG_FLAG go.sh) 2>&1

# --- Subprocess Test ---
echo -e "${YELLOW}--- Subprocess Test ---${NC}"
cat <<EOF > "subprocess_test_script.sh"
#!/bin/bash
echo "--- Running from subprocess_test_script.sh ---"
# Pass the debug flag to theRealPath
(theRealPath $DEBUG_FLAG go.sh) 2>&1
EOF
chmod +x "subprocess_test_script.sh"
./subprocess_test_script.sh
rm "subprocess_test_script.sh"

# --- Sourced Test ---
echo -e "${YELLOW}--- Sourced Test ---${NC}"
cat <<EOF > "sourced_test_script.sh"
#!/bin/bash
. terminal/theRealPath.sh
echo "--- Running from sourced_test_script.sh ---"
# Pass the debug flag to theRealPath
(theRealPath $DEBUG_FLAG go.sh) 2>&1
EOF
chmod +x "sourced_test_script.sh"
./sourced_test_script.sh
rm "sourced_test_script.sh"

echo -e "${BLUE}--- Sanity Tests Complete ---${NC}"