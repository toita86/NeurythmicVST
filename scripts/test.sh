#!/bin/bash
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Parse arguments
RUN_ALL=0
TEST_PATTERN=""
VERBOSE=0

while [[ $# -gt 0 ]]; do
    case $1 in
        --all|-a)
            RUN_ALL=1
            shift
            ;;
        --verbose|-v)
            VERBOSE=1
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [options] [test_pattern]"
            echo ""
            echo "Options:"
            echo "  --all, -a      Run all test suites (plugin + cpg)"
            echo "  --verbose, -v  Show detailed test output"
            echo "  --help, -h     Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                    Run plugin tests"
            echo "  $0 --all              Run all tests"
            echo "  $0 MatsuokaEngine     Run tests matching pattern"
            echo "  $0 --all --verbose    Run all tests with verbose output"
            exit 0
            ;;
        *)
            TEST_PATTERN="$1"
            shift
            ;;
    esac
done

echo -e "${GREEN}=== Running Tests ===${NC}"

# Build tests first
echo -e "${YELLOW}Building tests...${NC}"
cmake --build --preset dev --target NeurythmicTests CPGLibTests 2>&1 | tail -5

# Run plugin tests
if [ "$RUN_ALL" -eq 1 ] || [ -z "$TEST_PATTERN" ]; then
    echo -e "${BLUE}--- Plugin Tests ---${NC}"
    if [ "$VERBOSE" -eq 1 ]; then
        ctest --preset dev --output-on-failure
    else
        ctest --preset dev --output-on-failure 2>&1 | grep -E "(Test|Passed|Failed|Total)"
    fi
fi

# Run CPGLib tests
if [ "$RUN_ALL" -eq 1 ] || [ "$TEST_PATTERN" = "CPGLib" ] || [ "$TEST_PATTERN" = "Matsuoka*" ]; then
    echo -e "${BLUE}--- CPGLib Tests ---${NC}"
    if [ -f "build/dev/test/CPGLibTests" ]; then
        if [ -n "$TEST_PATTERN" ] && [ "$TEST_PATTERN" != "CPGLib" ]; then
            build/dev/test/CPGLibTests --gtest_filter="*${TEST_PATTERN}*"
        else
            build/dev/test/CPGLibTests
        fi
    else
        echo -e "${YELLOW}CPGLib tests binary not found at build/dev/test/CPGLibTests${NC}"
        echo -e "${YELLOW}Building CPGLib tests...${NC}"
        cmake --build --preset dev --target CPGLibTests 2>&1 | tail -5
        if [ -f "build/dev/test/CPGLibTests" ]; then
            build/dev/test/CPGLibTests
        fi
    fi
fi

# Run specific test pattern
if [ -n "$TEST_PATTERN" ] && [ "$RUN_ALL" -eq 0 ] && [ "$TEST_PATTERN" != "CPGLib" ]; then
    echo -e "${BLUE}--- Running tests matching: ${TEST_PATTERN} ---${NC}"
    if [ -f "build/dev/test/NeurythmicTests" ]; then
        build/dev/test/NeurythmicTests --gtest_filter="*${TEST_PATTERN}*"
    fi
fi

echo -e "${GREEN}=== Tests Complete ===${NC}"
