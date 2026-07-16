# CPGLib - Structure, Components & Compilation

A **static C++ library** implementing a **Central Pattern Generator (CPG)** network based on **Matsuoka's Neural Oscillator**. Designed for creative rhythm generation (music, audiovisual installations). Written by Daniel Bennett, C++11, CMake-based.

---

## Layered Architecture

### Layer 1 - Core Math (Low-Level)

| File | Role |
|---|---|
| `matsu_signal_calcs.h/.c` | **C-linkage** Matsuoka oscillator equations. RK4 integration, stability enforcement. The raw math. |
| `Matsu_calcs.h/.cpp` | **C++ wrapper** around the same oscillator math (class `MatsuCalcs`). Older/alternative interface with the same equations. |
| `core.h/.cpp` | Project-wide constants: `DEFAULTSAMPLERATE` (1000), `MAX_NODES` (16), `MAX_DELAYLINE_LENGTH` (20000), freq compensation, debug macros. |

### Layer 2 - Node & Utilities

| File | Role |
|---|---|
| `matsuNode.h/.cpp` | **`MatsuNode`** - A single two-neuron Matsuoka oscillator node. Manages parameters (T1, T2, C, B, G), internal state (X1, X2, V1, V2), inputs/outputs, delay line, signal state detection (zero crossings, peaks), hierarchy (parent/children), sync modes. |
| `delayLine.h` | **`DelayLine<S>`** - Template ring-buffer delay line. Used for phase offsets and output delay per node. Resizable. |
| `XORRand.h/.cpp` | **`XORRand`** - Fast xoroshiro128+ RNG for node self-noise. |
| `ScalingCurve.h/.cpp` | **`ScalingCurve`** - Interpolated lookup table for scaling connection weights based on frequency ratios between nodes (entrainment compensation). |

### Layer 3 - Network

| File | Role |
|---|---|
| `CPG.h/.cpp` | **`CPG`** - The CPG network. Manages a `vector<MatsuNode>`, connections between nodes (weights, phase offsets), frequency setting with inheritance, node add/delete, external sync/driving. Non-copyable singleton-style. |

### Layer 4 - Event & Quantisation

| File | Role |
|---|---|
| `QuantiseGrid_soft.h/.cpp` | **`QuantiseGrid_soft`** - Rhythmic grid mechanics. Tracks tempo-synced position, supports 24th-note and 32nd-note grids, variable quantisation strength. |
| `QuantisedEventQueue.h/.cpp` | **`QuantisedEventQueue`** - Sits between the CPG signals and event output. Quantises node firing events to a rhythmic grid with per-node control (grid type, multiplier, offset, strength). Uses a pool-based linked-list event queue. |

### Layer 5 - Main Engine (Public API)

| File | Role |
|---|---|
| `MatsuokaEngine.h/.cpp` | **`MatsuokaEngine`** - The main public interface. Thread-safe (mutex-protected). Wraps `CPG` + `QuantisedEventQueue`. Provides queued actions pattern for thread-safe control from a UI thread while audio runs on another thread. Handles calibration, event callbacks, pause/shutdown. |

### Extras

| File | Role |
|---|---|
| `pybindings/` | Python bindings via Boost.Python (`cpglib.cpp`). Separate CMakeLists.txt. |
| `doxygen/` | Doxygen documentation config + generated HTML. |
| `MatsuokaEngine_src.vcxproj` | Visual Studio project (Windows build). |

---

## Nice to Know

- **Thread model**: `MatsuokaEngine` uses a **queued actions pattern** -- control methods are queued and executed on `doQueuedActions()`, designed for a slow UI thread + fast audio thread.
- **Max 16 nodes** (`MAX_NODES`), hardcoded.
- **Default sample rate**: 1000 Hz (1ms resolution, control-rate, not audio-rate).
- **Calibration required**: `calibrate()` must be called to set frequency compensation before direct frequency control works accurately.
- **Connection weight scaling**: Because Matsuoka entrainment thresholds depend on frequency ratios, a scaling curve compensates so that equal weights produce equal effects regardless of node frequencies.
- **Quantiser is optional** and per-node. Supports 4/4 (32 steps) and 3/4 (24 steps) grids with variable "strength" (0 = free CPG rhythm, 1 = strict grid).
- **Two math backends**: `matsu_signal_calcs.c` (C, extern "C") and `Matsu_calcs.cpp` (C++ class). Both implement the same RK4 Matsuoka equations.
- **Not copyable**: `CPG` explicitly deletes copy constructor. `MatsuokaEngine` has a custom copy constructor.

---

## How to Compile

### As a standalone static library (via CMake)

```bash
cd CPGLib
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

This produces `libMatsuokaEngine.a` (output goes to `../../build/x64/Release/` per the CMakeLists.txt).

### Requirements

- **CMake >= 3.0** this requirement is not supported anymore by make this is way was bumped to the next supported version `4.0`
- **C++11 compiler** (GCC/Clang/MSVC)
- The CMakeLists sets `CMAKE_OSX_ARCHITECTURES x86_64;i386` (macOS universal) -- remove/adjust on Linux.

### Python bindings (optional)

```bash
cd CPGLib/pybindings
mkdir build && cd build
cmake ..
make
```

Requires **Boost.Python** (`libboost-python3-dev`) and **Python 3** dev headers.

### Integration with NeurythmicVST

CPGLib is meant to be linked as a static library (`MatsuokaEngine`) into a host application. Add it via:

```cmake
add_subdirectory(CPGLib)
target_link_libraries(your_target MatsuokaEngine)
```
