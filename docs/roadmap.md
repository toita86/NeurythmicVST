# NeurythmicVST — Full JUCE MIDI Instrument Roadmap

## Target Architecture

```
PluginProcessor (audio thread)
  ├── MatsuokaEngine (CPGLib) — step per sample
  ├── QuantisedEventQueue — host-tempo-synced grid
  ├── MIDI output via juce::MidiBuffer (firedNodes → NoteOn/Off)
  └── State persistence (juce::ValueTree + DAW params)

PluginEditor (GUI thread)
  ├── NetworkView — OpenGL graph (nodes, arcs, arrows, labels)
  ├── NodeMenu — right-click per-node (freq, constraints, MIDI)
  ├── MainMenu — sidebar (presets, CPG globals, mixer)
  └── Interaction — click/drag/shift-connect/right-click
```

---

## Scope Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Undo/Redo | Maybe (deferred) | Nice-to-have, not core to functionality |
| Build targets | VST3 + AU only (no Standalone) | Primary use case is DAW plugin |
| Rendering backend | OpenGL | Keep geometry shader approach from matsuoka_frontend |
| Audio output | MIDI-only | No internal synthesis; drives external VST samplers/synths |
| MAX/MSP dependency | Removed | Engine runs locally via CPGLib |

---

## Phase 0 — What's Already Done

| Item | Status |
|------|--------|
| CPGLib (MatsuokaEngine + QuantisedEventQueue + ScalingCurve) | Built as static lib |
| JUCE 8.0.12 plugin scaffold (VST3/AU) | Built |
| CMake (CPM, Ninja, ccache, 4 presets) | Configured |
| 29 tests (20 CPGLib + 9 plugin) | Passing |
| Basic 3-node hardcoded network + Start/Stop | Working |
| NodeComponent (pulsing circle, flash on fire) | Functional — will be replaced |
| clang-format, clang-tidy, C++23, .clangd | Enforced |

---

## Phase 1 — State Model & Configuration
**Goal:** Define the canonical data model that everything else depends on.

### 1.1 Port MatsuGlobals → ConfigManager

- Read `settings.xml` with `juce::XmlDocument` / `juce::ValueTree`
- Expose all config constants as const references:
  - Visual: nodeRadius, nodeColors[], lineThickness, arrowHeadSize, intesityFloor, etc.
  - CPG: t1OverT2, c, b, g, freqCompensation
  - Layout: nodeClickableRadius, nodeCollideDistance, nodeSpawnDistance
  - Connection: weightScalingLimit, weightScalingExp, curveAmount, min/maxLineWeight
- Provide singleton-style accessor (matching old `MatsuGlobals` pattern)

### 1.2 Define Network State (juce::ValueTree)

```
ROOT (Identifier: "Network")
  ├── Node (Identifier: "Node")
  │     properties: id, freq, phaseOffset, noise, vol, pitch, attack, decay,
  │                 envMode, synthPreset, muted, quantGrid, quantMult,
  │                 quantOffset, quantAmount, positionX, positionY
  │     ├── Connection (Identifier: "Connection")
  │     │     properties: sourceId, targetId, weight, phase, isParentChild
  │     └── ...
  └── ...
```

- Listener pattern: UI components attach `ValueTree::Listener` to react to changes
- Single source of truth — engine syncs to/from this tree

### 1.3 Port GUI_Presenter Logic → NetworkController

- Node lifecycle: create child, delete node (mute + disconnect if not removable)
- Connection lifecycle: add input, remove input, update weight/phase
- Node spawn positioning: spiral/radial algorithm from parent position
- Connection weight calculation: distance-based scaling (port `calcWeight()`)
- Focus system: what is selected, what action is in progress
- `updateFromEngine()`: poll engine `firedNodes()`, step flash envelopes
- 16-node array pattern (lock-free, ID = array index)

### 1.4 Port GUI_AD_Ramp → FlashEnvelope

- Attack-decay envelope for visual node flash
- Pure math class: trigger(velocity), step(), getValue()
- No external dependencies beyond standard library

### 1.5 Port MatsuPreset → PresetManager

- Save full network `ValueTree` as XML via `juce::XmlElement`
- Load XML and reconstruct ValueTree + engine state
- File dialog integration (`juce::FileChooser`)
- Default preset file path configurable

---

## Phase 2 — MIDI Output 
**Goal:** First playable milestone — the plugin produces MIDI notes in a DAW.

### 2.1 Wire Engine to MIDI Buffer (processBlock)

- New `MidiOutput` class maps engine fire events → MIDI notes on the audio thread
  (hooked via `engine.setEventCallback()`).
- Routing model (all derived on the audio thread, no `ValueTree` reads):

  | `routingMode` | Channel | Note |
  |---------------|---------|------|
  | Drum machine (default) | `midiDrumChannel` (10) | `midiTriggerNote + nodeId` |
  | Per-channel | `nodeId + 1` | `midiTriggerNote` (fixed) |

- Retrigger per cycle: `NoteOff` then `NoteOn` for each fire (legato deferred).
- Velocity from event amplitude via `clamp((amp*2)^2, 0, 1) * masterVolume * 127`
  (or `masterVolume * 127` in constant mode).

### 2.2 Host Tempo Synchronization

- `TempoSource` seam reads `{isPlaying, bpm}` (real = `juce::AudioPlayHead`,
  fake = unit-testable).
- Tempo precedence:
  - Host playing → root freq = `bpm/60`.
  - No host → root freq = `rootFreq` (Hz) directly.
- Transport stop → engine paused + `allNotesOff()` (no hung notes).
- Fallback: internal `rootFreq` when no host transport.
- Tempo changes propagate via `setNodeFrequency(0, ...)` → quantiser follows.

### 2.3 AudioProcessor Parameters (DAW Automation)

Expose via `juce::AudioProcessorValueTreeState`:

| Parameter ID | Type | Range | Default | Description |
|-------------|------|-------|---------|-------------|
| `rootFreq` | float | 0.1–20.0 Hz | 2.0 | Root node frequency (master internal tempo) |
| `masterQuantAmount` | float | 0.0–1.0 | 0.7 | Global quantise strength |
| `masterVolume` | float | 0.0–1.0 | 0.8 | Master MIDI velocity scalar |
| `velocityMode` | bool | — | amplitude | Constant vs amplitude-scaled velocity |
| `routingMode` | choice | — | drum-machine | Drum machine (single channel) vs per-node channel |

- `internalTempo` is a derived read-only BPM display (`rootFreq * 60`), not a param.
- `midiTriggerNote` (60) and `midiDrumChannel` (10) live in `settings.xml`/`ConfigManager`.
- `createParameterLayout()` → return `std::unique_ptr<AudioProcessorParameterGroup>`

### 2.4 DAW State Persistence

- `getStateInformation()`: serialize full network ValueTree → `juce::MemoryBlock` (XML compressed)
- `setStateInformation()`: deserialize → rebuild ValueTree → rebuild engine
- This saves/recalls the entire network inside DAW project files

---

## Phase 3 — Graph Visualization
**Goal:** The network graph renders interactively in the plugin window.

### 3.1 OpenGL Rendering Setup

- `juce::OpenGLContext` attached to main component (`NetworkViewComponent`)
- Enable `juce::juce_opengl`; embed `dotted_vert/geom/frag.glsl` via `juce_add_binary_data`
- Render straight to the back buffer — the legacy offscreen FBO was a redundant 1:1 blit (no post-processing), so it is dropped
- `continuousRepainting = false` — only redraw on change via `repaint()`
- Editor window set to 500×500 so the legacy `windowMinDim` normalised-coordinate mapping stays faithful

### 3.2 Node Rendering

- Circle meshes: `nodeRadius` (34px default), `pointsInCircle` (30) vertices
- Color palette: 10 named colors cycling by node index (from settings.xml)
- Vertex data: `GL_LINE_STRIP_ADJACENCY` for geometry shader thick lines
- Brightness: scaled by `FlashEnvelope::getValue() * velocity` between `intensityFloor` (0.7) and 1.0
- Root node: additional larger "halo" circle at `node0HaloSize` (1.35x) multiplier

### 3.3 Connection Rendering

- **Parent-child edges**: straight lines from circumference to circumference
  - Project endpoints via `projectToCircumference()` at `nodeRadius * node0HaloSize`
- **Input edges** (non-parent): curved arcs with perpendicular offset from chord
  - Arc center: perpendicular to midpoint, offset = `defaultCurveAmount * (distance² * 0.2 / 1200)`
- **Arrow heads**: triangle mesh at target end, oriented to line/arc tangent
  - Size: `arrowHeadSize` (12px), width scaled by line weight
- **Dotted lines**: zero-weight connections rendered as dashed via fragment shader
- **Color**: `lightGrey.lerp(connColour, weight/maxWeight)` — grey to orange
- **Selection highlight**: green when connection is selected
- Line thickness: mapped from weight via `minLineThickness` (1px) to `maxLineThickness` (8px)

### 3.4 Node Labels

- Above node: frequency multiple label (e.g. "0.5", "2", "8") in Inter font
- Below node: bar division label (quantise grid resolution)
- Drawn by a software `NodeLabelOverlayComponent` (transparent child of
  `NetworkViewComponent`, `juce::Graphics`) on top of the GL layer — text stays
  JUCE-native rather than being baked into glyph meshes
- Frequency multiple = `freq / rootFreq`, snapped to the legacy display
  increments; bar division read from `engine.getNodeQuantiser_BarDivision()`

### 3.5 GraphVis Port — Geometry + Mesh Store

```cpp
struct MeshObject {
    enum Type { ParentChildEdge, InputEdge, RootNode, ChildNode };
    Type type;
    int id;
    juce::OpenGLShaderProgram::Attribute* position;
    // Hit testing data (connections only) — retained for Phase 4 reuse
    juce::Point<float> startL, startR, endL, endR;
    float angleStart, angleEnd;
    juce::Point<float> centrePoint;
    float radius;
    float freqMult;
    int barDivision;
};
```

- Keyed by ID: node objects use node ID, connections use `((fromID+1) * 1000) + toID`
- Separate maps for dotted/solid/triangle objects
- Rebuild all meshes each repaint (≤16 nodes / ≤15 edges — trivial cost); the
  legacy per-object dirty-flag cache is not ported
- Pure geometry (circle/line/arc/arrowhead vertex generation,
  `projectToCircumference`, `getLineWidth`, `getNodeSpread`, `getColourScale`,
  `makeConnectionID`) extracted into a GL-free `GraphGeometry` unit so it is
  testable headlessly
- Root halo rendered directly (no legacy `id == -1` sentinel kludge)

### 3.6 Shader Porting

- **Vertex shader** (`dotted_vert.glsl`): pass-through position/colour + `gl_VertexID`
- **Geometry shader** (`dotted_geom.glsl`): thick triangle-strip lines with mitered joins
  - `GL_LINE_STRIP_ADJACENCY` → compute perpendicular offsets → emit triangle strip
- **Fragment shader** (`dotted_frag.glsl`): dashed line pattern for zero-weight connections
- Load via `juce::OpenGLShaderProgram` with `addVertexShader/addFragmentShader/addGeometryShader`
- Shader uniforms: `modelViewProjectionMatrix` (orthographic), `thickness`, `dotted`
- Arrowheads (unshaded triangles) use a minimal flat vertex/fragment shader —
  legacy drew them with the implicit default 2D shader

---

## Phase 4 — Interaction Layer
**Goal:** Full mouse-driven network editing inside the plugin window.

### 4.1 Mouse Event Handling

| Gesture | Target | Action |
|---------|--------|--------|
| Left click | Node | Select node, show focus |
| Left click | Connection | Select connection, show weight/phase handles |
| Left click | Empty area | Deselect all |
| Drag | Selected node | Move node position, recalculate connected arcs |
| Shift+click | Node A → Node B | Create connection from A to B |
| Right-click | Node | Open NodeMenu at cursor position |
| Right-click | Connection | Open ConnectionMenu at cursor position |
| Ctrl+click | Nodes | Multi-select |

- All handled in `NetworkViewComponent::mouseDown/mouseDrag/mouseUp/mouseMove`
- Pixel ↔ normalized coordinate conversion via window min dimension

### 4.2 Port Focus System

```cpp
enum class FocusType { RootNode, ChildNode, ParentChildEdge, InputEdge, Menu, None };
enum class ActionState { AddChild, AddInput, SetConnection, DeleteNode, None };

struct Focus {
    FocusType type = FocusType::None;
    int nodeId = -1;
    int connectionFromId = -1;
    int connectionToId = -1;
    juce::Point<float> cursorPos;
    ActionState action = ActionState::None;
    Focus previous; // restored on menu close
};
```

- Visual feedback: selected items drawn in highlight color
- Drag preview: translucent copy of node/connection during move

### 4.3 Hit Testing

- **Node hit test**: bounding box using `nodeClickableRadius` (normalized 0.035)
- **Straight connection**: point-in-quadrilateral raycast algorithm
  - Quad formed by: (startL, startR, endL, endR) — left/right edges of thick line
- **Curved connection**: distance from point to arc center between `radius ± connectionClickableWidth` (15px), then angle range check
- Hit test order: nodes first, then connections (nodes take priority if overlapping)

### 4.4 Connection Creation Flow

1. Shift held → enter "add input" mode
2. Click source node → highlight source, cursor shows connector line
3. Click target node → confirm connection
4. NetworkController creates connection with default weight (0 or parent/child weight)
5. Engine registers connection, OSC message no longer needed

### 4.5 Node Deletion Flow

- Only leaf nodes (no children) can be deleted
- "Delete" mutes node + zeroes all connections + marks inactive
- Engine limitation: MAX_NODES fixed at 16 — deletion frees the slot for reuse

---

## Phase 5 — Menu & Widget System

**Goal:** Complete GUI parity — all parameter controls accessible through JUCE menus.

### 5.1 Custom Tab Container Component

- Tab bar with SVG icon buttons (load via `juce::Drawable::createFromSVG()`)
- Tab switching calls `onTabChange(int index)` callback
- Dark theme: `ofxDatGuiThemeMatsuoka` port (#2C3137 bg, #F8F3F0 text, #6E6E6E slider fill)
- Additional themes: BigFont, Warning (red), Blue
- Layout: icon row at top, content area below fills remaining space

### 5.2 MainMenu (Right Sidebar, Width = 270px)

```
┌─────────────────────────────────────┐
│  [Tab 0: CPG]  [Tab 1: Mixer]  [✕] │  ← SVG tab bar
├─────────────────────────────────────┤
│  ┌─────────────────────────────┐    │
│  │  Content panel              │    │
│  │  (CPG settings or Mixer)   │    │
│  └─────────────────────────────┘    │
│  ┌─────────────────────────────┐    │
│  │  Bottom bar (preset mgmt)  │    │
│  └─────────────────────────────┘    │
└─────────────────────────────────────┘
```

**Tab 0 — CPG Settings:**
- "NEW Preset" button — resets network to single root node
- "RESTORE Preset" button — reloads current preset from disk
- "Save Preset File" button — file save dialog
- "Load Preset File" button — file open dialog
- "Reload Settings" button — re-read settings.xml
- Preset selector dropdown in bottom bar
- Preset name text editor in bottom bar

**Tab 1 — Mixer:** (see Phase 6)

### 5.3 NodeMenu (Right-Click Context Menu, Width = 250px)

```
┌─────────────────────────────────────┐
│  [Node]  [Constraint]  [MIDI]       │  ← SVG tab bar
├─────────────────────────────────────┤
│                                     │
│  Tab 0 — Node:                      │
│  ┌───┬───┬───┬───┬───┐             │
│  │ ¼ │ ⅓ │ ½ │ 1 │ 2 │             │  ← Frequency matrix
│  ├───┼───┼───┼───┼───┤             │     (10 buttons, highlight active)
│  │ 3 │ 4 │ 6 │ 8 │ 16│             │
│  └───┴───┴───┴───┴───┘             │
│  Fine Tune: [====●==]  1.5x         │  ← Slider (0.5–2.0)
│  Sync Mode: [NONE] [1X] [LOCK]      │  ← Matrix (3 buttons, radio mode)
│  Self-Noise: [==●====]  0.3         │  ← Slider (0–1)
│  Phase:      [===●===]  0.25        │  ← Slider (0–1)
│                                     │
│  [Add Child to Node]  [Delete Node] │  ← Buttons
│                                     │
│  Tab 1 — Constraint:                │
│  Freedom:   [===●===] 0.8           │  ← 1 = no quantisation
│  Grid:      [OFF] [24th] [32nd]     │  ← Matrix (3 buttons)
│  Resolution:[8] [4] [2] [1]         │  ← Buttons (24th=8/4/2/1, 32nd=6/3/2/1)
│  Raw Mode:  [○] Off                 │  ← Toggle
│                                     │
│  ┌─────────────────────────────┐    │
│  │  ConstraintGUI              │    │  ← Custom interactive grid widget
│  │  (grid lines visualization) │    │
│  └─────────────────────────────┘    │
│                                     │
│  Tab 2 — MIDI:                      │
│  Voice Preset: [1][2][3][4][5]      │  ← Matrix (voice selection)
│                   [6][7][8]         │
│  ┌─────────────────────────────┐    │
│  │  Piano Keyboard             │    │  ← juce::MidiKeyboardComponent
│  │  (2 octaves)                │    │     (note selection)
│  └─────────────────────────────┘    │
│  Octave: [-1] [0] [+1] ... [+5]    │  ← Matrix (octave shift)
│  Attack: [====●==] 10ms             │  ← Slider (2–2000ms)
│  Decay:  [=====●=====] 300ms        │  ← Slider (50–2000ms)
│  Velocity:[====●==] 100             │  ← Slider (0–127)
│  Env Mode:[ADSR] [Neural]           │  ← Toggle
└─────────────────────────────────────┘
```

Position: anchored to right-click cursor position, constrained to plugin window bounds.

### 5.4 ConnectionMenu (On Connection Right-Click)

```
Weight: [=====●=] 3.5          ← Slider (0 to connectionWeightMax)
Phase:  [====●===] 0.25        ← Slider (0–1)
[Remove Connection]            ← Button
```

### 5.5 Custom Widget Implementations

**FrequencyMatrix** (`juce::Component` subclass):
- 2 rows × 5 columns grid of buttons
- Labels: ¼, ⅓, ½, 1, 2 / 3, 4, 6, 8, 16
- Radio behavior: one selected at a time
- Draw with dark theme colors
- Callback: `onFrequencySelected(double freqMultiple)`

**SyncModeMatrix** (`juce::Component` subclass):
- 3 buttons in a row: NONE, 1X, LOCK
- Radio mode toggle
- Callback: `onSyncModeChanged(SyncMode mode)`

**GridTypeMatrix** (`juce::Component` subclass):
- 3 buttons: OFF, 24, 32
- Radio mode toggle
- Callback: `onGridChanged(GridType type)`

**QuantResolutionSelector** (`juce::Component` subclass):
- 4 buttons side by side
- 24th grid: labels 8, 4, 2, 1
- 32nd grid: labels 6, 3, 2, 1
- Updates labels when grid type changes

**ConstraintGUI** (`juce::Component` subclass):
- Draws horizontal grid lines representing quantise positions
- `paint()`: draw lines at computed positions based on grid type × resolution
- Current offset and multiple shown with highlight
- Potentially interactive: drag to set offset (future enhancement)

### 5.6 Menu Lifecycle

- Open on right-click → create `juce::Component` at cursor position
- Tab switching stores current values before switching
- Close on: click outside, press Escape, or close button (✕)
- Values committed on each parameter change (live, no Apply button)
- Edge detection: menu constrained within plugin window bounds

---

## Phase 6 — Mixer Panel 

**Goal:** Per-node volume/mute/solo, master controls.

### 6.1 MixerPanel (Inside MainMenu Tab 1)

```
┌─────────────────────────────────────┐
│  ┌──────┬──────┬──────┬──────┐     │
│  │ Ch 0 │ Ch 1 │ Ch 2 │ ...  │     │  ← Scrollable if > 6 channels visible
│  │ Node │ Node │ Node │      │     │
│  │  1   │  2   │  3   │      │     │
│  │ [M]  │ [M]  │ [M]  │      │     │  ← Mute toggle (red when active)
│  │ [S]  │ [S]  │ [S]  │      │     │  ← Solo toggle (yellow when active)
│  │  ║   │  ║   │  ║   │      │     │  ← Volume fader (vertical slider)
│  │  ║   │  ║   │  ║   │      │     │
│  │  ║   │  ║   │  ║   │      │     │
│  │  85  │  47  │ 100  │      │     │  ← Volume value label
│  └──────┴──────┴──────┴──────┘     │
│                                     │
│  Master Volume: [========●=] 0.8    │  ← Horizontal master fader
└─────────────────────────────────────┘
```

### 6.2 Channel Component

```cpp
class MixerChannel : public juce::Component {
    juce::TextButton muteButton;
    juce::TextButton soloButton;
    juce::Slider volumeFader;     // vertical, 0.001–1.5 range
    juce::Label nameLabel;        // node label
    juce::Label valueLabel;       // current volume

    int nodeId;
    void resized() override;      // layout: label top, buttons below, fader fills
    void paint() override;        // dark background with subtle border

    // Solo logic
    bool isSoloed = false;
    bool isMuted = false;
};
```

### 6.3 Solo Behavior

- If any channel is soloed → only soloed channels produce MIDI
- Non-soloed channels are dimmed visually
- Master solo clear button to unsolo all

### 6.4 Master Controls (in MainMenu CPG tab)

- Master volume fader: scalar applied to all MIDI velocities
- Root frequency display (read-only, set via root node params or DAW automation)
- Current BPM display (from host or internal fallback)
- Engine Start/Stop button

---

## Phase 7 — DAW Integration & Polish

**Goal:** Professional plugin behavior in a DAW environment.

### 7.1 Host Transport & Tempo

- BPM changes while playing → update `QuantiseGrid::setTempo()` immediately (no glitch)
- Transport stop → engine pause, `juce::MidiBuffer::addEvent(NoteOff)` for all active notes
- Transport start → engine resume, quantised grid re-syncs phase to downbeat
- Loop region → detect loop jump via `ppqPosition` discontinuity, reset engine if needed
- Time signature changes → update grid divisions (24 for 3/4, 32 for 4/4)

### 7.2 Expand DAW Automation Parameters

Add per-node automatable parameters to `AudioProcessorValueTreeState`:
- `node_{id}_freq` — per-node frequency
- `node_{id}_volume` — per-node MIDI velocity
- `node_{id}_pitch` — MIDI note number
- `node_{id}_mute` — mute state (bool)
- `node_{id}_quantAmount` — per-node quantise strength

Parameter naming convention: prefix with `node_N_` for clean sorting in DAW parameter list.

### 7.3 Window Resize Support

- `PluginEditor::resized()` → recalculate NetworkView bounds
- Graph scales to fit available area (maintains aspect ratio via `windowMinDim`)
- Widget positions recalculated proportionally
- Minimum window size: 600×400 (same as current)

### 7.4 Visual Polish

- Double-buffered rendering via OpenGL FBO — no flicker
- Node hover: subtle glow/outline on mouse hover before click
- Connection hover: highlight path, show weight tooltip
- Smooth state transitions: interpolate node position on drag
- Node flash: inherited from `FlashEnvelope` — smooth attack/decay curve
- Background: dark grey-blue (#19191E) matching matsuoka_frontend

### 7.5 Testing & QA

**Unit Tests:**
- MIDI output correctness: verify NoteOn/Off timing and velocity against quantised grid
- Engine state sync: verify ValueTree ↔ Engine round-trip consistency
- Tempo sync accuracy: measure event jitter at various BPM values
- Preset save/load: verify byte-identical network after round-trip

**Integration Tests:**
- Create network → set params → save preset → reload → verify identical
- Rapid tempo changes during playback → verify no hung notes, no crash
- 16-node maximum network → verify performance within DAW callback budget
- Plugin reload in DAW project → verify state restoration

**Manual QA:**
- Test in Reaper, Ableton Live, Logic Pro (AU), FL Studio
- Stress test: max nodes, rapid interaction, tempo automation
- Check for memory leaks (JUCE leak detector in debug builds)

---

## Maybes (Deferred / Future Versions)

| Feature | Priority | Notes |
|---------|----------|-------|
| Undo/Redo via `juce::UndoManager` | Low | Nice DAW integration but not MVP |
| Standalone build target | Low | Primary use is DAW plugin |
| Internal FM synthesis (MatsuSynth) | Low | MIDI-out-only for now |
| Cross-backend rendering (Metal/DX) | Low | OpenGL works everywhere in JUCE 8 |
| Preset morphing / interpolation | Low | Future creative feature |
| MIDI CC output (mod wheel per node, etc.) | Low | Beyond note events |
| Python bindings for CPGLib | Low | Already partially in CPGLib repo |
| Lua scripting for generative logic | Low | Future extensibility idea |

---

## Summary Schedule

| Phase | Days | Cumulative | Key Deliverable |
|-------|------|------------|-----------------|
| 0 | — | — | Existing NeurythmicVST scaffold |
| 1 — State & Config | 1-3 | Day 3 | ValueTree network, ConfigManager, Presenter, Presets |
| 2 — MIDI Output | 3-5 | Day 5 | **Playable MIDI instrument in DAW** |
| 3 — GraphVis | 5-8 | Day 8 | Network renders with OpenGL |
| 4 — Interaction | 8-10 | Day 10 | Full mouse editing of network |
| 5 — Menus & Widgets | 10-17 | Day 17 | Complete parameter control GUI |
| 6 — Mixer & Master | 17-19 | Day 19 | Volume/mute/solo per node |
| 7 — DAW Polish | 19-22 | Day 22 | Automation, host sync, testing |

**Total: ~22 working days (~4.5 calendar weeks)**

---

## Files Removed from matsuoka_frontend Scope

| File / System | Reason |
|---------------|--------|
| `Matsuoka_OSC_Engine.h/.cpp` | OSC proxy — engine runs locally, no MAX/MSP |
| `src/lib/ofxDatGui_dtb/` (entire directory) | Replaced by JUCE Components |
| `menus/Menu.h/.cpp` (base class) | Replaced by JUCE Component hierarchy |
| `menus/MenuTabs.h/.cpp` | Replaced by custom tab container |
| `ofxOsc` addon dependency | No OSC communication |
| `ofxXmlSettings` dependency | Replaced by `juce::XmlElement` |
| `ofxSvg` dependency | Replaced by `juce::Drawable::createFromSVG()` |
| `MatsuSynth.h` | MIDI-only, no internal synthesis |
| `Logo.h` | Optional (plugin has its own branding assets) |
| `SimpleEvent.h` / `FMNote` templates | MIDI-only, note events are simpler |
| All OF types: `ofVec2f`, `ofColor`, `ofVboMesh`, `ofFbo`, `ofShader`, `ofTrueTypeFont` | Replaced by JUCE equivalents |
| `addons.make` | Replaced by CMake CPM |
| `.vcxproj` / `.sln` | Replaced by CMake |
| Windows DLLs (`glut32.dll`, `fmodex.dll`, etc.) | JUCE handles platform layer |

