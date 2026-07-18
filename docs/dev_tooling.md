# Development Tooling Guide

This document explains the development tools and workflow for the Neurythmic project.

## Required Tools

Install all required tools with:

```bash
sudo apt install -y ninja-build ccache clang-format clang-tidy
```

### Tool Descriptions

| Tool | Purpose |
|------|---------|
| **Ninja** | Fast build system (2-5x faster than Make). Used as the default generator for all presets. |
| **ccache** | Compiler cache that speeds up rebuilds. Stores compiled objects and reuses them when source hasn't changed. |
| **clang-format** | Automatic code formatter. Ensures consistent style across the codebase. |
| **clang-tidy** | Static analysis tool. Catches bugs, enforces best practices, and suggests improvements. |
| **CMake** | Build system generator. Version 3.25+ required. |
| **GoogleTest** | Testing framework. Automatically downloaded via CPM when `BUILD_TESTS=ON`. |

## CMake Presets

The project uses CMake presets to standardize build configurations. Available presets:

| Preset | Description | Use Case |
|--------|-------------|----------|
| `dev` | Debug build with tests, Ninja, ccache | **Daily development** |
| `release` | Optimized release build | Creating distributable binaries |
| `relwithdebinfo` | Optimized build with debug symbols | Profiling and debugging release builds |
| `ci` | Clean build with warnings as errors | CI/CD pipelines |

### Using Presets

```bash
# Configure and build with dev preset
cmake --preset dev
cmake --build --preset dev

# Or use one-liner
cmake --preset dev && cmake --build --preset dev

# Run tests
ctest --preset dev
```

## Convenience Scripts

All scripts are in the `scripts/` directory and are executable.

### build.sh

Builds the project with automatic configuration detection.

```bash
./scripts/build.sh              # Build dev preset
./scripts/build.sh release      # Build release preset
./scripts/build.sh clean        # Remove all build directories
VERBOSE=1 ./scripts/build.sh    # Show ccache statistics
```

**Features:**
- Checks for required tools (ninja, ccache)
- Auto-configures if build directory doesn't exist
- Shows ccache stats in verbose mode
- Displays output artifact locations

### test.sh

Builds and runs tests.

```bash
./scripts/test.sh                   # Run plugin tests
./scripts/test.sh --all             # Run all tests (plugin + CPGLib)
./scripts/test.sh MatsuokaEngine    # Run tests matching pattern
./scripts/test.sh --all --verbose   # Run all tests with detailed output
```

**Features:**
- Builds test targets automatically
- Supports test filtering with patterns
- Separate plugin and CPGLib test suites
- Verbose mode for debugging

### format.sh

Formats C++ code using clang-format.

```bash
./scripts/format.sh           # Format all files
./scripts/format.sh --check   # Check formatting without modifying
./scripts/format.sh --verbose # Show detailed output
```

**Features:**
- Processes all `.cpp`, `.h`, `.hpp` files in `neurythmic_plugin/` and `CPGLib/`
- Check mode for CI validation
- Reports formatted vs unchanged files

### lint.sh

Runs clang-tidy static analysis.

```bash
./scripts/lint.sh                    # Lint all files
./scripts/lint.sh --fix              # Apply automatic fixes
./scripts/lint.sh PluginProcessor    # Lint files matching pattern
./scripts/lint.sh --verbose          # Show detailed output
```

**Features:**
- Uses `compile_commands.json` from build directory
- Auto-builds if compile database missing
- Supports automatic fixes with `--fix`
- Reports warnings and errors per file

## Typical Workflow

### First Time Setup

```bash
# Install tools
sudo apt install -y ninja-build ccache clang-format clang-tidy

# Build the project
./scripts/build.sh

# Run tests
./scripts/test.sh --all
```

### Daily Development

```bash
# 1. Make code changes
vim neurythmic_plugin/source/PluginProcessor.cpp

# 2. Build
./scripts/build.sh

# 3. Test
./scripts/test.sh

# 4. Format code before commit
./scripts/format.sh

# 5. Check for issues
./scripts/lint.sh

# 6. Commit
git add -A
git commit -m "feat: add CPG rhythm generation"
```

### Debugging

```bash
# Build with debug symbols
./scripts/build.sh

# Run standalone plugin in debugger
gdb ./build/dev/NeurythmicPlugin_artefacts/Debug/Standalone/Neurythmic

# Or use coredumps
ulimit -c unlimited
./build/dev/NeurythmicPlugin_artefacts/Debug/Standalone/Neurythmic
```

### Release Build

```bash
# Build optimized release
./scripts/build.sh release

# Outputs are in:
# build/release/NeurythmicPlugin_artefacts/Release/
```

## ccache Statistics

ccache dramatically speeds up rebuilds. View statistics:

```bash
ccache -s
```

Example output:
```
Cache statistics:
  Primary config:   /home/user/.config/ccache/ccache.conf
  Secondary config: /etc/ccache.conf
  Cache directory:  /home/user/.cache/ccache
  Cache hit (direct):                 1234
  Cache hit (preprocessed):            567
  Cache miss:                          890
  Files in cache:                     2345
  Cache size:                        456.7 MB
  Max cache size:                      5.0 GB
```

**Tips:**
- First build will be slow (cache miss)
- Subsequent builds are much faster (cache hit)
- Clean builds after `git clean -fdx` are still fast
- ccache is shared across all build directories

## Code Quality

### Formatting

The project uses `.clang-format` for consistent style:
- Based on Chromium style
- 4-space indentation
- 120 character line limit
- Sorted includes

Run before every commit:
```bash
./scripts/format.sh
```

### Static Analysis

The project uses `.clang-tidy` for code quality:
- Enables most clang-tidy checks
- Disables noisy/irrelevant checks
- Enforces naming conventions
- Catches common bugs

Run regularly:
```bash
./scripts/lint.sh
```

### Compiler Warnings

The project uses strict compiler warnings via `cmake/CompilerWarnings.cmake`:
- `-Wall -Wextra -Wpedantic`
- `-Wconversion -Wshadow -Wsign-conversion`
- And many more

Warnings are treated as errors in CI builds.

## IDE Integration

### VS Code

Install extensions:
- C/C++ (Microsoft)
- CMake Tools
- clangd
- CodeLLDB (for debugging)

Settings (`.vscode/settings.json`):
```json
{
  "cmake.configureArgs": [
    "-DCMAKE_BUILD_TYPE=Debug",
    "-DBUILD_TESTS=ON"
  ],
  "C_Cpp.default.compileCommands": "${workspaceFolder}/build/dev/compile_commands.json"
}
```

### CLion

1. Open `CMakeLists.txt` as project
2. Select `dev` preset in CMake configuration
3. CLion will auto-detect compile_commands.json

### Vim/Neovim

Use `compile_commands.json` with:
- **coc-clangd** - Language server with diagnostics
- **ALE** - Async linting
- **clang-format** - Auto-format on save

## Troubleshooting

### Build Fails with "ninja: command not found"

```bash
sudo apt install ninja-build
```

### Build Fails with "ccache: command not found"

```bash
sudo apt install ccache
```

### Tests Fail to Link

Ensure JUCE modules are properly linked:
```bash
rm -rf build/
cmake --preset dev
cmake --build --preset dev
```

### clang-tidy Reports False Positives

Update `.clang-tidy` to disable specific checks:
```yaml
Checks: "*,
  -check-to-disable,
  ..."
```

### ccache Not Working

Check if ccache is being used:
```bash
ccache -s
```

If cache hits are 0, ensure `CMAKE_CXX_COMPILER_LAUNCHER` is set:
```bash
grep CCACHE build/dev/CMakeCache.txt
```

## Performance Tips

1. **Use ccache**: Already enabled in presets. First build is slow, subsequent builds are fast.

2. **Use Ninja**: Already the default. 2-5x faster than Make.

3. **Build only what you need**:
   ```bash
   cmake --build --preset dev --target NeurythmicPlugin
   ```

4. **Parallel builds**: Ninja does this automatically. For manual builds:
   ```bash
   cmake --build build/dev -j$(nproc)
   ```

5. **Incremental builds**: Only rebuild changed files. ccache helps here.

6. **Precompiled headers**: Not currently used, but can speed up JUCE builds.

## CI/CD Integration

The `ci` preset is designed for CI:

```yaml
# .github/workflows/build.yml
- name: Configure
  run: cmake --preset ci

- name: Build
  run: cmake --build --preset ci

- name: Test
  run: ctest --preset ci
```

Features:
- Warnings as errors
- Clean build directory
- Verbose test output
- No ccache (CI environments are ephemeral)

## Additional Resources

- [CMake Documentation](https://cmake.org/documentation/)
- [Ninja Manual](https://ninja-build.org/manual.html)
- [ccache Manual](https://ccache.dev/manual/latest.html)
- [clang-format](https://clang.llvm.org/docs/ClangFormat.html)
- [clang-tidy](https://clang.llvm.org/extra/clang-tidy/)
- [GoogleTest](https://google.github.io/googletest/)
