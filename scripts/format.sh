#!/bin/bash
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check for clang-format
if ! command -v clang-format &> /dev/null; then
    echo -e "${RED}Error: clang-format not found${NC}"
    echo "Install with: sudo apt install clang-format"
    exit 1
fi

# Parse arguments
DRY_RUN=0
VERBOSE=0

while [[ $# -gt 0 ]]; do
    case $1 in
        --check|-c)
            DRY_RUN=1
            shift
            ;;
        --verbose|-v)
            VERBOSE=1
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  --check, -c    Check formatting without modifying files"
            echo "  --verbose, -v  Show detailed output"
            echo "  --help, -h     Show this help message"
            exit 0
            ;;
        *)
            shift
            ;;
    esac
done

echo -e "${GREEN}=== Formatting C++ Files ===${NC}"

# Find all C++ files
FILES=$(find neurythmic_plugin CPGLib -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" \) 2>/dev/null)

if [ -z "$FILES" ]; then
    echo -e "${YELLOW}No C++ files found${NC}"
    exit 0
fi

FILE_COUNT=$(echo "$FILES" | wc -l)
echo -e "${BLUE}Found ${FILE_COUNT} files to process${NC}"

FORMATTED=0
UNCHANGED=0
FAILED=0

for file in $FILES; do
    if [ "$DRY_RUN" -eq 1 ]; then
        # Check mode - just verify formatting
        if ! clang-format --dry-run --Werror "$file" 2>/dev/null; then
            echo -e "${RED}Needs formatting: ${file}${NC}"
            FAILED=$((FAILED + 1))
        else
            UNCHANGED=$((UNCHANGED + 1))
            if [ "$VERBOSE" -eq 1 ]; then
                echo -e "${GREEN}OK: ${file}${NC}"
            fi
        fi
    else
        # Format mode - apply changes
        if clang-format -i "$file" 2>/dev/null; then
            if [ "$VERBOSE" -eq 1 ]; then
                echo -e "${GREEN}Formatted: ${file}${NC}"
            fi
            FORMATTED=$((FORMATTED + 1))
        else
            echo -e "${RED}Failed: ${file}${NC}"
            FAILED=$((FAILED + 1))
        fi
    fi
done

echo ""
echo -e "${GREEN}=== Format Complete ===${NC}"

if [ "$DRY_RUN" -eq 1 ]; then
    echo -e "Checked: ${FILE_COUNT} files"
    echo -e "${GREEN}Already formatted: ${UNCHANGED}${NC}"
    if [ "$FAILED" -gt 0 ]; then
        echo -e "${RED}Needs formatting: ${FAILED}${NC}"
        exit 1
    fi
else
    echo -e "Processed: ${FILE_COUNT} files"
    echo -e "${GREEN}Formatted: ${FORMATTED}${NC}"
    if [ "$FAILED" -gt 0 ]; then
        echo -e "${RED}Failed: ${FAILED}${NC}"
    fi
fi
