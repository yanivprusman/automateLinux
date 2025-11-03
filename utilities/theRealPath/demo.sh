#!/bin/bash

# Color codes for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}SOURCE vs SUBPROCESS DEMONSTRATION${NC}"
echo -e "${BLUE}================================================${NC}\n"

# Create a test script that modifies various aspects
cat > /tmp/test_script.sh << 'SCRIPT_EOF'
#!/bin/bash

# 1. Set environment variables
export DEMO_VAR="I was set in the script"
DEMO_LOCAL="I'm a local variable"

# 2. Change directory
original_dir=$(pwd)
cd /tmp

# 3. Define a function
my_function() {
    echo "Hello from my_function!"
}

# 4. Create an alias
alias demo_alias='echo "Alias executed!"'

# 5. Modify PATH
export PATH="/tmp/custom_path:$PATH"

# 6. Show script's PID and PPID
echo "Script PID: $$"
echo "Script PPID: $PPID"
echo "Current directory in script: $(pwd)"
echo "DEMO_VAR in script: $DEMO_VAR"
SCRIPT_EOF

chmod +x /tmp/test_script.sh

# Store initial state
INITIAL_DIR=$(pwd)
INITIAL_PATH="$PATH"
MAIN_PID=$$

echo -e "${YELLOW}Initial State:${NC}"
echo "Main shell PID: $MAIN_PID"
echo "Current directory: $INITIAL_DIR"
echo "DEMO_VAR: ${DEMO_VAR:-<not set>}"
echo "PATH starts with: ${PATH:0:50}..."
echo ""

# ============================================
# DEMONSTRATION 1: Running as Subprocess
# ============================================
echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}TEST 1: Running as SUBPROCESS (./script.sh)${NC}"
echo -e "${GREEN}================================================${NC}\n"

echo -e "${YELLOW}Executing: ./test_script.sh${NC}"
/tmp/test_script.sh
echo ""

echo -e "${YELLOW}After subprocess execution:${NC}"
echo "Main shell PID: $$ (unchanged)"
echo "Current directory: $(pwd)"
echo "DEMO_VAR: ${DEMO_VAR:-<not set>}"
echo "my_function exists? $(type my_function 2>/dev/null | grep -q function && echo 'YES' || echo 'NO')"
echo "demo_alias exists? $(alias demo_alias 2>/dev/null | grep -q alias && echo 'YES' || echo 'NO')"
echo "PATH starts with: ${PATH:0:50}..."
echo "PATH modified? $(echo $PATH | grep -q '/tmp/custom_path' && echo 'YES' || echo 'NO')"
echo ""

echo -e "${RED}RESULT: All changes were ISOLATED in the subprocess${NC}"
echo -e "${RED}Nothing persisted to the parent shell!${NC}\n"

# ============================================
# DEMONSTRATION 2: Sourcing
# ============================================
echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}TEST 2: SOURCING (source script.sh)${NC}"
echo -e "${GREEN}================================================${NC}\n"

# Reset to initial directory for fair comparison
cd "$INITIAL_DIR"

echo -e "${YELLOW}Executing: source test_script.sh${NC}"
source /tmp/test_script.sh
echo ""

echo -e "${YELLOW}After sourcing:${NC}"
echo "Main shell PID: $$ (still same process)"
echo "Current directory: $(pwd)"
echo "DEMO_VAR: ${DEMO_VAR:-<not set>}"
echo "DEMO_LOCAL: ${DEMO_LOCAL:-<not set>}"

# Test function
echo -n "my_function exists? "
if type my_function 2>/dev/null | grep -q function; then
    echo "YES - calling it:"
    my_function
else
    echo "NO"
fi

# Test alias
echo -n "demo_alias exists? "
if alias demo_alias 2>/dev/null | grep -q alias; then
    echo "YES - calling it:"
    shopt -s expand_aliases
    demo_alias 2>/dev/null || echo "Alias defined but needs interactive shell to execute"
else
    echo "NO"
fi

echo "PATH starts with: ${PATH:0:50}..."
echo "PATH modified? $(echo $PATH | grep -q '/tmp/custom_path' && echo 'YES' || echo 'NO')"
echo ""

echo -e "${GREEN}RESULT: All changes PERSISTED in the current shell!${NC}\n"

# ============================================
# DEMONSTRATION 3: Exit Behavior
# ============================================
echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}TEST 3: EXIT BEHAVIOR${NC}"
echo -e "${GREEN}================================================${NC}\n"

# Create script with exit
cat > /tmp/exit_test.sh << 'EXIT_EOF'
#!/bin/bash
echo "About to exit..."
exit 42
echo "This line never executes"
EXIT_EOF

chmod +x /tmp/exit_test.sh

echo -e "${YELLOW}Subprocess with exit:${NC}"
/tmp/exit_test.sh
echo "Exit code: $?"
echo "Main shell still running after subprocess exit!"
echo ""

echo -e "${YELLOW}Sourcing with exit (DANGEROUS - commented out):${NC}"
echo "# source /tmp/exit_test.sh"
echo "If we sourced this, our main shell would EXIT!"
echo -e "${RED}This is why you should NEVER source scripts with 'exit' commands${NC}\n"

# ============================================
# DEMONSTRATION 4: Performance
# ============================================
echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}TEST 4: PERFORMANCE COMPARISON${NC}"
echo -e "${GREEN}================================================${NC}\n"

# Create simple script
cat > /tmp/perf_test.sh << 'PERF_EOF'
#!/bin/bash
: # Do nothing (colon is a no-op)
PERF_EOF

chmod +x /tmp/perf_test.sh

echo "Running each method 100 times..."

# Time subprocess
echo -n "Subprocess: "
time for i in {1..100}; do
    /tmp/perf_test.sh > /dev/null 2>&1
done

# Time sourcing
echo -n "Sourcing: "
time for i in {1..100}; do
    source /tmp/perf_test.sh
done

echo -e "\n${YELLOW}Note: Sourcing is typically faster (no fork overhead)${NC}\n"

# ============================================
# DEMONSTRATION 5: Variable Scope
# ============================================
echo -e "${GREEN}================================================${NC}"
echo -e "${GREEN}TEST 5: VARIABLE SCOPE${NC}"
echo -e "${GREEN}================================================${NC}\n"

export PARENT_VAR="I'm from parent"

cat > /tmp/var_test.sh << 'VAR_EOF'
#!/bin/bash
echo "In script - PARENT_VAR: $PARENT_VAR"
echo "In script - Setting CHILD_VAR=child_value"
CHILD_VAR="child_value"
export CHILD_VAR
VAR_EOF

chmod +x /tmp/var_test.sh

echo -e "${YELLOW}Subprocess:${NC}"
/tmp/var_test.sh
echo "After subprocess - CHILD_VAR: ${CHILD_VAR:-<not set>}"
echo ""

echo -e "${YELLOW}Sourcing:${NC}"
unset CHILD_VAR  # Clean slate
source /tmp/var_test.sh
echo "After sourcing - CHILD_VAR: ${CHILD_VAR:-<not set>}"
echo ""

# Cleanup
cd "$INITIAL_DIR"
rm -f /tmp/test_script.sh /tmp/exit_test.sh /tmp/perf_test.sh /tmp/var_test.sh

echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}SUMMARY${NC}"
echo -e "${BLUE}================================================${NC}"
echo -e "
${GREEN}Use SOURCING when:${NC}
  • Loading configuration files (.bashrc, .profile)
  • Setting environment variables for current session
  • Defining functions/aliases in current shell
  • You want changes to persist

${GREEN}Use SUBPROCESS when:${NC}
  • Running standalone scripts
  • You want isolation (no side effects)
  • Script contains 'exit' commands
  • Doing actual work/automation

${RED}Key Danger:${NC}
  • NEVER source scripts with 'exit' - it will close your shell!
"