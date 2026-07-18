#!/bin/bash
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Check for clang-tidy
if ! command -v clang-tidy &> /dev/null; then
    echo -e "${RED}Error: clang-tidy not found${NC}"
    echo "Install with: sudo apt install clang-tidy"
    exit 1
fi

# Check for compile_commands.json
BUILD_DIR="build/dev"
COMPILE_DB="${BUILD_DIR}/compile_commands.json"

if [ ! -f "$COMPILE_DB" ]; then
    echo -e "${YELLOW}compile_commands.json not found. Building first...${NC}"
    cmake --preset dev
    cmake --build --preset dev
fi

# Parse arguments
FIX=0
FILE_PATTERN=""
VERBOSE=0

while [[ $# -gt 0 ]]; do
    case $1 in
        --fix|-f)
            FIX=1
            shift
            ;;
        --verbose|-v)
            VERBOSE=1
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [options] [file_pattern]"
            echo ""
            echo "Options:"
            echo "  --fix, -f      Apply automatic fixes"
            echo "  --verbose, -v  Show detailed output"
            echo "  --help, -h     Show this help message"
            echo ""
            echo "Examples:"
            echo "  $0                      Lint all files"
            echo "  $0 --fix                Apply automatic fixes"
            echo "  $0 PluginProcessor      Lint files matching pattern"
            exit 0
            ;;
        *)
            FILE_PATTERN="$1"
            shift
            ;;
    esac
done

echo -e "${GREEN}=== Running clang-tidy ===${NC}"

# Build clang-tidy command
TIDY_CMD="clang-tidy"
TIDY_ARGS="-p ${BUILD_DIR}"

if [ "$FIX" -eq 1 ]; then
    TIDY_ARGS="${TIDY_ARGS} --fix"
    echo -e "${YELLOW}Mode: Fix${NC}"
else
    echo -e "${YELLOW}Mode: Check${NC}"
fi

# Find files to lint
if [ -n "$FILE_PATTERN" ]; then
    FILES=$(find neurythmic_plugin -type f \( -name "*.cpp" -o -name "*.h" \) | grep "$FILE_PATTERN")
else
    FILES=$(find neurythmic_plugin -type f \( -name "*.cpp" -o -name "*.h" \))
fi

if [ -z "$FILES" ]; then
    echo -e "${YELLOW}No files found matching pattern${NC}"
    exit 0
fi

FILE_COUNT=$(echo "$FILES" | wc -l)
echo -e "${BLUE}Found ${FILE_COUNT} files to lint${NC}"
echo ""

ISSUES=0
FILES_WITH_ISSUES=0

for file in $FILES; do
    if [ "$VERBOSE" -eq 1 ]; then
        echo -e "${BLUE}Checking: ${file}${NC}"
    fi
    
    OUTPUT=$($TIDY_CMD $TIDY_ARGS "$file" 2>&1)
    
    if [ -n "$OUTPUT" ]; then
        # Count warnings/errors
        WARNINGS=$(echo "$OUTPUT" | grep -c "warning:" || true)
        ERRORS=$(echo "$OUTPUT" | grep -c "error:" || true)
        
        if [ "$WARNINGS" -gt 0 ] || [ "$ERRORS" -gt 0 ]; then
            FILES_WITH_ISSUES=$((FILES_WITH_ISSUES + 1))
            ISSUES=$((ISSUES + WARNINGS + ERRORS))
            
            echo -e "${RED}${file}: ${WARNINGS} warnings, ${ERRORS} errors${NC}"
            
            if [ "$VERBOSE" -eq 1 ]; then
                echo "$OUTPUT" | grep -E "(warning|error):" | head -10
                echo ""
            fi
        fi
    fi
done

echo ""
echo -e "${GREEN}=== Lint Complete ===${NC}"
echo -e "Checked: ${FILE_COUNT} files"

if [ "$ISSUES" -gt 0 ]; then
    echo -e "${RED}Found ${ISSUES} issues in ${FILES_WITH_ISSUES} files${NC}"
    if [ "$FIX" -eq 0 ]; then
        echo -e "${YELLOW}Run with --fix to apply automatic fixes${NC}"
    fi
    exit 1
else
    echo -e "${GREEN}No issues found${NC}"
fi
