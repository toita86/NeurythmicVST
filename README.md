# Neurythmic

JUCE audio plugin implementing a Central Pattern Generator (CPG) network for adaptive rhythm generation, based on Matsuoka's Neural Oscillator.

## Download / Install

Prebuilt VST3 bundles for macOS (universal), Windows and Linux are published on the
[Releases page](https://github.com/toita86/NeurythmicVST/releases). Download the zip
for your platform and copy the `.vst3` bundle to:

- **macOS:** `~/Library/Audio/Plug-Ins/VST3/`
- **Windows:** `%LOCALAPPDATA%/Programs/Common/VST3/`
- **Linux:** `~/.vst3/`

Then reload your DAW.

The builds are unsigned: macOS Gatekeeper may quarantine the bundle (right-click →
Open), Windows SmartScreen may warn on first load, and Linux needs `libfreetype6` +
`libasound2` installed.

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
