#!/bin/bash

################################################################################
#                          ft_script Test Suite                               #
#                                                                              #
# This script runs a comprehensive test suite for ft_script.                  #
# It tests various command-line options and edge cases.                       #
################################################################################

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
RESET='\033[0m'

# Test counter
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Program name
PROG="./ft_script"

# Test output directory
TEST_DIR="test_output"

################################################################################
# Helper Functions
################################################################################

# Print test header
print_test_header() {
    echo -e "\n${BLUE}========================================${RESET}"
    echo -e "${BLUE}Test $1: $2${RESET}"
    echo -e "${BLUE}========================================${RESET}"
}

# Print success message
print_success() {
    echo -e "${GREEN}✓ PASS${RESET}: $1"
    ((TESTS_PASSED++))
}

# Print failure message
print_failure() {
    echo -e "${RED}✗ FAIL${RESET}: $1"
    ((TESTS_FAILED++))
}

# Run a test
run_test() {
    ((TESTS_RUN++))
}

# Clean up test files
cleanup() {
    rm -f "$TEST_DIR"/*.txt
    rm -f typescript
}

################################################################################
# Test Cases
################################################################################

# Setup
echo -e "${YELLOW}Setting up test environment...${RESET}"
mkdir -p "$TEST_DIR"
cleanup

# Check if program exists
if [ ! -x "$PROG" ]; then
    echo -e "${RED}Error: $PROG not found or not executable${RESET}"
    echo "Please run 'make' first"
    exit 1
fi

################################################################################
# Test 1: Help option
################################################################################
print_test_header "1" "Help option (-h)"
run_test

$PROG -h > "$TEST_DIR/help_output.txt" 2>&1
EXIT_CODE=$?

if [ $EXIT_CODE -eq 1 ] && grep -q "Usage:" "$TEST_DIR/help_output.txt"; then
    print_success "Help message displayed correctly"
else
    print_failure "Help option failed"
fi

################################################################################
# Test 2: Simple command execution
################################################################################
print_test_header "2" "Simple command execution (-c)"
run_test

$PROG -c "echo 'Hello World'" "$TEST_DIR/test2.txt" > /dev/null 2>&1
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ] && grep -q "Hello World" "$TEST_DIR/test2.txt"; then
    print_success "Simple command executed and logged"
else
    print_failure "Simple command execution failed"
    cat "$TEST_DIR/test2.txt"
fi

################################################################################
# Test 3: Multi-line output
################################################################################
print_test_header "3" "Multi-line output"
run_test

$PROG -c "printf 'Line 1\nLine 2\nLine 3\n'" "$TEST_DIR/test3.txt" > /dev/null 2>&1
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ] && \
   grep -q "Line 1" "$TEST_DIR/test3.txt" && \
   grep -q "Line 2" "$TEST_DIR/test3.txt" && \
   grep -q "Line 3" "$TEST_DIR/test3.txt"; then
    print_success "Multi-line output captured correctly"
else
    print_failure "Multi-line output failed"
    cat "$TEST_DIR/test3.txt"
fi

################################################################################
# Test 4: Append mode
################################################################################
print_test_header "4" "Append mode (-a)"
run_test

# First write
$PROG -c "echo 'First'" "$TEST_DIR/test4.txt" > /dev/null 2>&1
# Second write (append)
$PROG -a -c "echo 'Second'" "$TEST_DIR/test4.txt" > /dev/null 2>&1
EXIT_CODE=$?

# Count occurrences
FIRST_COUNT=$(grep -c "First" "$TEST_DIR/test4.txt")
SECOND_COUNT=$(grep -c "Second" "$TEST_DIR/test4.txt")

if [ $EXIT_CODE -eq 0 ] && [ $FIRST_COUNT -eq 1 ] && [ $SECOND_COUNT -eq 1 ]; then
    print_success "Append mode works correctly"
else
    print_failure "Append mode failed (First: $FIRST_COUNT, Second: $SECOND_COUNT)"
    cat "$TEST_DIR/test4.txt"
fi

################################################################################
# Test 5: Quiet mode
################################################################################
print_test_header "5" "Quiet mode (-q)"
run_test

$PROG -q -c "echo 'Quiet test'" "$TEST_DIR/test5.txt" > "$TEST_DIR/test5_stdout.txt" 2>&1
EXIT_CODE=$?

# Quiet mode should suppress "Script started/done" in stdout
if [ $EXIT_CODE -eq 0 ] && \
   ! grep -q "Script started" "$TEST_DIR/test5_stdout.txt" && \
   grep -q "Quiet test" "$TEST_DIR/test5_stdout.txt"; then
    print_success "Quiet mode suppresses start/done messages in stdout"
else
    print_failure "Quiet mode failed"
    echo "STDOUT:"
    cat "$TEST_DIR/test5_stdout.txt"
fi

################################################################################
# Test 6: Exit status option
################################################################################
print_test_header "6" "Exit status option (-e)"
run_test

# Test with successful command
$PROG -e -c "exit 0" "$TEST_DIR/test6a.txt" > /dev/null 2>&1
EXIT_CODE_0=$?

# Test with specific exit code
$PROG -e -c "exit 42" "$TEST_DIR/test6b.txt" > /dev/null 2>&1
EXIT_CODE_42=$?

if [ $EXIT_CODE_0 -eq 0 ] && [ $EXIT_CODE_42 -eq 42 ]; then
    print_success "Exit status option returns correct codes (0, 42)"
else
    print_failure "Exit status option failed (got $EXIT_CODE_0 and $EXIT_CODE_42)"
fi

################################################################################
# Test 7: Default output file
################################################################################
print_test_header "7" "Default output file (typescript)"
run_test

rm -f typescript
$PROG -c "echo 'Default file test'" > /dev/null 2>&1
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ] && [ -f typescript ] && grep -q "Default file test" typescript; then
    print_success "Default output file (typescript) created"
else
    print_failure "Default output file creation failed"
fi
rm -f typescript

################################################################################
# Test 8: Empty command
################################################################################
print_test_header "8" "Empty output handling"
run_test

$PROG -c "true" "$TEST_DIR/test8.txt" > /dev/null 2>&1
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ] && [ -f "$TEST_DIR/test8.txt" ]; then
    print_success "Empty output handled correctly"
else
    print_failure "Empty output handling failed"
fi

################################################################################
# Test 9: Special characters
################################################################################
print_test_header "9" "Special characters in output"
run_test

$PROG -c "echo 'Special: \$HOME @#% &*()'" "$TEST_DIR/test9.txt" > /dev/null 2>&1
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ] && grep -q "Special:" "$TEST_DIR/test9.txt"; then
    print_success "Special characters handled correctly"
else
    print_failure "Special characters handling failed"
    cat "$TEST_DIR/test9.txt"
fi

################################################################################
# Test 10: Long output
################################################################################
print_test_header "10" "Long output handling"
run_test

$PROG -c "seq 1 1000" "$TEST_DIR/test10.txt" > /dev/null 2>&1
EXIT_CODE=$?

LINE_COUNT=$(wc -l < "$TEST_DIR/test10.txt")

# Should have 1000 lines + start/done messages
if [ $EXIT_CODE -eq 0 ] && [ $LINE_COUNT -gt 1000 ]; then
    print_success "Long output handled correctly ($LINE_COUNT lines)"
else
    print_failure "Long output handling failed ($LINE_COUNT lines)"
fi

################################################################################
# Test 11: Command with arguments
################################################################################
print_test_header "11" "Command with arguments"
run_test

$PROG -c "ls -la /tmp" "$TEST_DIR/test11.txt" > /dev/null 2>&1
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ] && [ -s "$TEST_DIR/test11.txt" ]; then
    print_success "Command with arguments executed"
else
    print_failure "Command with arguments failed"
fi

################################################################################
# Test 12: Start and done messages
################################################################################
print_test_header "12" "Start and done messages"
run_test

$PROG -c "echo 'Test'" "$TEST_DIR/test12.txt" > /dev/null 2>&1
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ] && \
   grep -q "Script started on" "$TEST_DIR/test12.txt" && \
   grep -q "Script done on" "$TEST_DIR/test12.txt"; then
    print_success "Start and done messages present"
else
    print_failure "Start/done messages missing"
    cat "$TEST_DIR/test12.txt"
fi

################################################################################
# Test 13: Flush mode (bonus)
################################################################################
print_test_header "13" "Flush mode (-f) [BONUS]"
run_test

$PROG -f -c "echo 'Flush test'" "$TEST_DIR/test13.txt" > /dev/null 2>&1
EXIT_CODE=$?

if [ $EXIT_CODE -eq 0 ] && grep -q "Flush test" "$TEST_DIR/test13.txt"; then
    print_success "Flush mode works"
else
    print_failure "Flush mode failed"
fi

################################################################################
# Test 14: Multiple options
################################################################################
print_test_header "14" "Multiple options (-a -q -e)"
run_test

$PROG -a -q -e -c "echo 'Multi-option'; exit 5" "$TEST_DIR/test14.txt" > "$TEST_DIR/test14_stdout.txt" 2>&1
EXIT_CODE=$?

if [ $EXIT_CODE -eq 5 ] && \
   grep -q "Multi-option" "$TEST_DIR/test14.txt" && \
   ! grep -q "Script started" "$TEST_DIR/test14_stdout.txt"; then
    print_success "Multiple options work together"
else
    print_failure "Multiple options failed (exit: $EXIT_CODE)"
fi

################################################################################
# Test 15: Invalid option
################################################################################
print_test_header "15" "Invalid option handling"
run_test

$PROG -X > /dev/null 2>&1
EXIT_CODE=$?

if [ $EXIT_CODE -ne 0 ]; then
    print_success "Invalid option rejected correctly"
else
    print_failure "Invalid option not handled"
fi

################################################################################
# Summary
################################################################################
echo -e "\n${BLUE}========================================${RESET}"
echo -e "${BLUE}Test Summary${RESET}"
echo -e "${BLUE}========================================${RESET}"
echo -e "Tests run:    ${TESTS_RUN}"
echo -e "Tests passed: ${GREEN}${TESTS_PASSED}${RESET}"
echo -e "Tests failed: ${RED}${TESTS_FAILED}${RESET}"

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "\n${GREEN}All tests passed!${RESET} ✓"
    exit 0
else
    echo -e "\n${RED}Some tests failed!${RESET} ✗"
    exit 1
fi
