# Neurythmic

JUCE audio plugin implementing a Central Pattern Generator (CPG) network for adaptive rhythm generation, based on Matsuoka's Neural Oscillator.

## Build

```bash
sudo apt install -y ninja-build ccache clang-format clang-tidy
./scripts/build.sh
```

## Test

```bash
./scripts/test.sh --all
```

## Scripts

- `build.sh` - Build (dev by default, `release` for optimized)
- `test.sh` - Run tests
- `format.sh` - Format code
- `lint.sh` - Static analysis

See `docs/dev_tooling.md` for full documentation.
