#!/bin/bash
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check for required tools
check_tools() {
    local missing=()
    
    if ! command -v ninja &> /dev/null; then
        missing+=("ninja-build")
    fi
    
    if ! command -v ccache &> /dev/null; then
        missing+=("ccache")
    fi
    
    if [ ${#missing[@]} -ne 0 ]; then
        echo -e "${RED}Error: Missing required tools: ${missing[*]}${NC}"
        echo "Install with: sudo apt install ${missing[*]}"
        exit 1
    fi
}

# Show ccache stats
show_ccache_stats() {
    if command -v ccache &> /dev/null; then
        echo -e "${YELLOW}=== ccache Statistics ===${NC}"
        ccache -s
        echo ""
    fi
}

# Main build function
build() {
    check_tools
    
    local preset="${1:-dev}"
    local build_dir="build/${preset}"
    
    echo -e "${GREEN}=== Building Neurythmic (${preset}) ===${NC}"
    
    # Configure if build directory doesn't exist or CMakeLists.txt is newer
    if [ ! -d "${build_dir}" ] || [ "CMakeLists.txt" -nt "${build_dir}/CMakeCache.txt" ]; then
        echo -e "${YELLOW}Configuring with preset: ${preset}${NC}"
        cmake --preset "${preset}"
    fi
    
    # Build
    echo -e "${YELLOW}Building...${NC}"
    cmake --build --preset "${preset}"
    
    echo -e "${GREEN}=== Build Complete ===${NC}"
    
    # Show ccache stats if verbose
    if [ "${VERBOSE:-0}" = "1" ]; then
        show_ccache_stats
    fi
    
    # Show output location
    echo -e "${GREEN}Outputs in: ${build_dir}/NeurythmicPlugin_artefacts/${NC}"
}

# Parse arguments
case "${1}" in
    clean)
        echo "Cleaning build directories..."
        rm -rf build/*/
        echo "Clean complete"
        ;;
    release)
        build "release"
        ;;
    *)
        build "dev"
        ;;
esac
