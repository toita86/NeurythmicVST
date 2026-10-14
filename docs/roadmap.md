# NeurythmicVST — Full JUCE MIDI Instrument Roadmap

## Target Architecture

```
PluginProcessor (audio thread)
  ├── MatsuokaEngine (CPGLib) — step per sample
  ├── QuantisedEventQueue — host-tempo-synced grid
  ├── MIDI output via juce::MidiBuffer (firedNodes → NoteOn/Off)
  └── State persistence (juce::ValueTree + DAW params)

PluginEditor (GUI thread)
  ├── NetworkView — software juce::Graphics graph (nodes, arcs, arrows, labels)
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
| Rendering backend | Software (`juce::Graphics`) | Cross-platform, no GL context/driver fragility; graph is ≤16 nodes so CPU rendering is trivial |
| Audio output | MIDI-only | No internal synthesis; drives external VST samplers/synths |
| MAX/MSP dependency | Removed | Engine runs locally via CPGLib |
| Connection weight | Derived (`calcWeight(distance, scaleFactor)`) | Faithful to legacy: weight follows distance, recomputed on drag; the menu slider edits the scale factor, not the raw weight |
| Selection state | UI-transient (controller only) | Selection is ephemeral; it is never serialized into presets |

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

### 3.1 Rendering Setup

- `NetworkViewComponent` (`juce::Component`) draws the whole graph in its
  `paint(juce::Graphics&)` — no OpenGL context, no shaders.
- Repaint is driven by the editor's 30 Hz timer (flash animation); `repaint()`
  on change.
- Editor window set to 500×500 so the legacy `windowMinDim` normalised-coordinate
  mapping stays faithful (`scaling = min(1, minDim/1000)`).

### 3.2 Node Rendering

- `g.drawEllipse` for each node; root gets an additional larger "halo" circle at
  `node0HaloSize` (1.35x).
- Color palette: 10 named colors cycling by node index (from settings.xml).
- Brightness: `getNodeBrightness(intensity)` between `intensityFloor` (0.7) and 1.0.
- Line thickness: `getNodeSpread(intensity)` (node flash → line weight).

### 3.3 Connection Rendering

- **Parent-child edges**: `g.drawLine` between `projectToCircumference()` points
  (at `nodeRadius * node0HaloSize`), thickness `getLineWidth(weight)`.
- **Input edges** (non-parent): curved arcs — `makeInputEdge()` + `makeArc()`
  points built into a `juce::Path`, stroked with `PathStrokeType`.
  - Note: the legacy `makeInputEdge` used a non-normalised perpendicular for the
    arc centre, which made the centre inconsistent with the radius and produced
    looping arcs; the port normalises the perpendicular (a documented deviation
    from the verbatim port).
- **Arrow heads**: `makeArrowHead()` triangle filled via `juce::Path`.
- **Dotted lines**: zero-weight connections stroked with
  `PathStrokeType::createDashedStroke`.
- **Color**: `lightGrey.lerp(connColour, colourScale)` — grey to orange.
- Line thickness: mapped from weight via `minLineThickness` to `maxLineThickness`.

### 3.4 Node Labels

- Above node: frequency multiple label (e.g. "0.5", "2", "8") in Inter font.
- Below node: bar division label (quantise grid resolution).
- Drawn in the same `paint()` pass with `g.drawText` — JUCE-native text, no
  glyph meshes.
- Frequency multiple = `freq / rootFreq`, snapped to the legacy display
  increments; bar division read from `engine.getNodeQuantiser_BarDivision()`.

### 3.5 GraphVis Port — Geometry

- Pure geometry (arc/input-edge/arrowhead generation, `projectToCircumference`,
  `getLineWidth`, `getNodeSpread`, `getColourScale`, `getNodeBrightness`,
  `makeConnectionID`) lives in the GL-free `GraphGeometry` unit so it is
  unit-testable headlessly.
- Connection keying for Phase 4 hit-testing: the `(sourceId, targetId)` pair
  (the legacy `((fromID+1) * 1000) + toID` encoding is dropped — it only keyed
  the GL mesh map).
- The scene is re-derived from the controller on every paint (≤16 nodes /
  ≤15 edges — trivial cost); the legacy per-object dirty-flag cache is not ported.
- Root halo rendered directly (no legacy `id == -1` sentinel kludge).

### 3.6 Rendering Decision Note

- Originally attempted as a verbatim OpenGL geometry-shader port (faithful thick
  mitered lines + fragment-shader dashes). On Linux the GL child-window did not
  composite to screen, so Phase 3 was re-implemented in pure `juce::Graphics`,
  which reproduces thick lines, arcs, dashes and arrowheads with no context,
  driver or Wayland fragility. (Recorded in the autoethnographic note.)

---

## Phase 4 — Interaction Layer
**Goal:** Full mouse-driven network editing inside the plugin window.

### 4.1 Mouse Event Handling

| Gesture | Target | Action |
|---------|--------|--------|
| Left click | Node | Select node (Ctrl = multi-select), set focus |
| Left click | Connection | Select connection, set focus |
| Left click | Empty area | Deselect all, clear focus |
| Drag (LMB) | Selected node | Move selection, recompute connected weights |
| Drag (RMB) | Empty area | Move all nodes (pan the whole graph) |
| Shift+click | Node A → Node B | Create (or toggle) connection from A to B |
| Alt+click | Node | Reset node to defaults |
| Right-click | Node / Connection | Select + set focus (menu opens in Phase 5) |
| Ctrl+click | Nodes | Multi-select toggle |

- All handled in `NetworkViewComponent::mouseDown/mouseDrag/mouseUp`
- Pixel ↔ normalized coordinate conversion via window min dimension
- Modifier keys read from `juce::ModifierKeys`

### 4.2 Focus & Selection

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

- **Focus** = the transient interaction target (what is under the cursor / being
  dragged). **Selection** = the persistent highlight, held as UI-transient state
  in the controller (a set of node IDs + one optional connection), *not* in the
  ValueTree — selection is never serialized into presets.
- Selected nodes/connections are drawn in `selectedColour`.
- Drag preview (translucent ghost) is deferred to Phase 7 polish.

### 4.3 Hit Testing

- **Node hit test**: bounding box using `nodeClickableRadius` (normalized 0.035).
- **Straight connection** (parent-child): point-in-quadrilateral raycast against
  the projected segment widened by `connectionClickableWidth`.
- **Curved connection** (input edge): distance from point to arc centre between
  `radius ± connectionClickableWidth` (15px), then an angle-range check.
- Geometry is derived on the fly from node positions (no cached mesh map): the
  same `GraphGeometry::makeConnectionGeometry()` used by the renderer feeds the
  hit test, so the clickable region always matches the drawn shape.
- Hit test order: nodes first, then connections (nodes take priority).
- Connection identity is the `(sourceId, targetId)` pair (the legacy
  `((from+1)*1000)+to` keying is dropped — it only existed for the GL mesh map).

### 4.4 Connection Creation Flow

1. Shift held on a node click → enter "add input" mode.
2. Click source node A, then target node B (both shift-held).
3. `NetworkController::toggleConnection(from, to)`:
   - already connected & not the parent → remove the connection;
   - otherwise → `addConnection` with `scaleFactor = newConnWeightScale`,
     `weight = calcWeight(distance, scaleFactor)`.
4. Engine registers the connection via `setConnection`.

### 4.5 Node Deletion Flow

- Only leaf nodes (no children) can be deleted.
- The *trigger* is the NodeMenu "Delete Node" button (Phase 5); the controller
  method (`NetworkController::deleteNode`) already exists and frees the slot.
- Engine limitation: MAX_NODES fixed at 16 — deletion frees the slot for reuse.

### 4.6 Connection Weight Model (scaleFactor)

- Every connection stores a persistent `scaleFactor`; its displayed weight is
  derived: `weight = calcWeight(distance, scaleFactor)`.
- Dragging a node recomputes the weights of its incoming and outgoing
  connections from their scale factors (faithful to the legacy behaviour, where
  distance encodes coupling strength).
- Parent-child edges default to `newParentChildConnWeightScale` (1.0); input
  edges default to `newConnWeightScale` (0.0).
- Phase 5's connection-weight slider edits the scale factor (not the raw
  weight), then recomputes the weight.

---

## Phase 5 — Menu & Widget System

**Goal:** Complete GUI parity — all parameter controls accessible through JUCE menus.

### 5.1 Custom Tab Container Component

- Tab bar with SVG icon buttons (load via `juce::Drawable::createFromSVG()`)
- Tab switching calls `onTabChange(int index)` callback
- Dark theme: `ofxDatGuiThemeMatsuoka` port (#2C3137 bg, #F8F3F0 text, #6E6E6E slider fill)
- Additional themes: BigFont, Warning (red), Blue
- Layout: icon row at top, content area below fills remaining space

### 5.2 MainMenu (Docked Right Sidebar, Toggleable, Width = 270px)

- Docked on the right edge; the network view resizes into the remaining area.
- Toggleable: hidden via the ✕ tab, re-shown via a tab/gear button.

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
│  [Node]  [Constraint]               │  ← SVG tab bar (MIDI tab → Phase 6)
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
└─────────────────────────────────────┘
```

Position: anchored to right-click cursor position, constrained to plugin window bounds. The MIDI tab (note/piano, octave, shared velocity) is added in Phase 6 — see §6.9.

### 5.4 ConnectionMenu (On Connection Right-Click)

```
Weight: [=====●=] 3.5          ← Slider (0 to connectionWeightMax)
Phase:  [====●===] 0.25        ← Slider (0–1)
[Remove Connection]            ← Button
```

### 5.5 Custom Widget Implementations

- Strategy: JUCE stock controls (`Slider`, `TextButton`, `ToggleButton`,
  `ComboBox`, `MidiKeyboardComponent`) where visually faithful; custom
  `juce::Component` subclasses for the matrix/radio widgets below and
  `ConstraintGUI`.
- Shared dark-theme constants (see §5.1) used across all widgets.

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

- Context menus (NodeMenu, ConnectionMenu) rendered in a `juce::CallOutBox`
  (arrow hidden) anchored to the right-click cursor; it provides outside-click
  dismissal, Escape-to-close and screen-edge clamping. If the visual result is
  not faithful, fall back to a hand-rolled floating `Component`.
- Open on right-click → create `juce::Component` at cursor position
- Tab switching stores current values before switching
- Close on: click outside, press Escape, or close button (✕)
- Values committed on each parameter change (live, no Apply button)
- Edge detection: menu constrained within plugin window bounds

---

## Phase 6 — Mixer Panel & Per-Node MIDI

**Goal:** Per-node volume/mute/solo and per-node MIDI note selection, plus master
controls.

### 6.1 Scope note (MIDI-only)

- The legacy frontend drove a synth engine (`setNodeVol` / `setNodeAttack` /
  `setNodeDecay` / `setNodeNeuralEnv` / `setNodeMute` / `setNodePitch` /
  `setNodePreset`). This plugin is a MIDI instrument: it emits note events only.
- The synth-only parameters — Voice Preset, Attack, Decay and Env Mode
  (`NEURALENV`) — are **dropped**. The MIDI tab is reduced to per-node note
  (piano + octave); per-node velocity is the shared mixer volume.

### 6.2 Data Model (Node properties)

New `NetworkState::Props` on `Node`:

| Property | Type | Default | Meaning |
|----------|------|---------|---------|
| `volume` | float | `newNodeVolumeInit` (0.6) | shared mixer fader / velocity scale, 0.0–1.5 |
| `midiNote` | int | `midiTriggerNote + nodeId` | per-node note override (0–127) |
| `muted` | bool | false | channel mute |

- **Solo is UI/session-transient**: held in `NetworkController` (like selection),
  never serialized.
- Persisted in presets / DAW state: `volume`, `midiNote`, `muted`. Solo is not.

### 6.3 Note × routing reconciliation (the two MIDI routing modes)

- `MidiOutput` already has two routing modes (`DrumMachine`, `PerChannel`) and two
  velocity modes (`Constant`, `Amplitude`) — the two MIDI-mode pairs.
- Per-node `midiNote` **overrides** `noteFor()` in **both** routing modes; routing
  only selects the channel:
  - `DrumMachine` → channel `midiDrumChannel`;
  - `PerChannel` → channel `nodeId + 1`.
- The default per-node note is `midiTriggerNote + nodeId`, so drum behaviour is
  unchanged until the user edits the note. (This intentionally changes the
  `PerChannel` default from a fixed `baseNote` to `baseNote + nodeId`.)
- Velocity: `velocityFor` multiplies by the per-node gain (`volume`). Mute/solo
  set a per-node emit gate.

### 6.4 Real-time-safe delivery

- The `ValueTree` is message-thread only; the audio thread never reads it.
- New header-only `MixerSnapshot`: 16-wide `std::atomic` arrays (note, gain,
  enabled), owned by `NetworkController` (one instance, mutated on the message
  thread).
- Controller mutators write tree → engine → snapshot (message thread).
- `PluginProcessor::processBlock` copies the snapshot into `MidiOutput` once per
  block (no lock, no allocation).
- `rebuild()` / `clear()` repopulate the snapshot from the tree;
  `createChild` / `deleteNode` update the slot.

### 6.5 MixerPanel (Inside MainMenu Tab 1)

Layout is the legacy horizontal-row form (the sidebar is 270px; vertical-fader
columns would not fit), inside a vertical `Viewport`:

```
┌─────────────────────────────────────┐
│ [color] Node 1   [==========●] 0.60 │  ← volume slider (horizontal)
│                    [M] [S]          │  ← mute (red) / solo (yellow)
│ [color] Node 2   [===●======] 0.40  │
│ ...                                 │  ← scrolls if > viewport
├─────────────────────────────────────┤
│ MASTER           [========●=] 0.80  │
│ BPM 120   Root 2.00 Hz              │  ← read-only labels
└─────────────────────────────────────┘
```

### 6.6 MixerChannel component

```cpp
class MixerChannel : public juce::Component {
    juce::Label nameLabel;
    juce::Slider volume;      // horizontal, 0.0–1.5
    juce::TextButton mute, solo;
    juce::Label valueLabel;
    int nodeId;
};
```

### 6.7 Solo behaviour

- If any channel is soloed → only soloed channels emit.
- Effective per-node emit = `!muted && (!anySolo || soloed)`.
- A master solo-clear button unsolos all.
- Non-soloed channels are dimmed visually.

### 6.8 Master controls (in MainMenu CPG tab)

- Master volume slider — binds to the existing `masterVolume` APVTS parameter.
- Root frequency display (read-only).
- Current BPM display (from host or internal fallback).
- **No Engine Start/Stop**: in a DAW the host transport is the gate.

### 6.9 NodeMenu MIDI Tab

Adds a third `[MIDI]` tab to the NodeMenu (see §5.3):

```
Piano Keyboard (2 octaves)              ← juce::MidiKeyboardComponent (note selection)
Octave: [-1] [0] [+1] ... [+5]          ← Matrix (octave shift)
Velocity:[====●==] 0.60                 ← Slider (shared per-node volume)
```

- Note = `octave * 12 + semitone`, stored as the absolute `midiNote`.
- No voice preset / attack / decay / env mode (synth-only, dropped — see §6.1).

---

## Phase 7 — DAW Integration & Polish

**Goal:** Professional plugin behavior in a DAW environment.

### 7.1 Host Transport & Tempo

- `TempoInfo` (the `TempoSource` snapshot) is extended with `ppqPosition`/`hasPpq`
  and `timeSigNumerator`/`timeSigDenominator`; `PlayheadTempoSource::read()`
  populates them from `juce::AudioPlayHead::PositionInfo`.
- BPM changes while playing → node-0 frequency `bpm/60` (the engine auto-derives
  the quantiser tempo from node 0; no extra `setTempo` call needed).
- Loop / seek → detect a `ppqPosition` discontinuity against the expected
  per-block delta; on a backward or oversized jump call `engine.reset()` and
  emit NoteOff for all held notes.
- Transport stop → emit NoteOff for all active notes (engine is not stepped, so
  no new notes are produced). Transport start → **resume without reset** (the
  engine keeps its phase; the user opted against a downbeat re-sync).
- Time signature → maps the CPGLib grid by **denominator** (`x/4` → `_32nd`,
  `x/8` → `_24th`). It drives a per-node **"follow host"** grid default; an
  explicit NodeMenu grid choice overrides it for that node.

### 7.2 Expand DAW Automation Parameters

**Architecture decision:** the `juce::ValueTree` stays the canonical data model
(presets and the `MixerSnapshot` pipeline are unchanged); the APVTS is a
**mediated automation facade**, not a second source of truth.

- Parameters are a **fixed upfront pool**: for each of the 16 node slots, five
  parameters are created in `createParameterLayout()` — one per node id:
  - `node_NN_freq` — per-node frequency multiple (child nodes only; node 0 is
    governed by the global `rootFreq` parameter and host tempo)
  - `node_NN_volume` — per-node MIDI velocity / mixer gain (0–1.5)
  - `node_NN_pitch` — MIDI note number (0–127)
  - `node_NN_mute` — mute state (bool)
  - `node_NN_quantAmount` — per-node quantise strength (0–1)
- IDs are zero-padded (`node_01_…` … `node_16_…`) so they sort cleanly in the
  DAW parameter list. `NN` is the node id + 1 so displayed labels read "Node 1".
- The 16-slot pool is a deliberate choice: JUCE exposes
  `AudioProcessor::addParameter` but **not** `removeParameter`, so a truly
  dynamic add/remove pool is impossible; a fixed pool keeps the parameter list
  stable across the session. Unused slots remain present (inert).
- **Bridge (message thread only):**
  - Outbound (UI/menu/preset → APVTS): `NetworkController` writes the tree and
    then raises a `onNodeParamsChanged(id)` callback; `PluginProcessor` copies
    the node's values into the APVTS params.
  - Inbound (host automation → tree): `PluginProcessor` implements
    `AudioProcessorValueTreeState::Listener`; `parameterChanged()` parses the id
    and calls the matching controller setter.
  - A `_syncingParams` guard suppresses the callback that would otherwise loop.
  - The controller stays APVTS-free (callbacks default to no-ops) so the
    existing tests and construction sites are untouched; APVTS listener
    callbacks arrive on the message thread (APVTS flushes host automation to its
    tree on a timer), so no `ValueTree` access happens on the audio thread.

### 7.3 Window Resize Support

**Deferred.** The editor is already resizable (`PluginEditor` sets resize limits
and the view scales by `min(width, height)`); no changes are made in this phase.

### 7.4 Visual Polish

- Single software `juce::Graphics` paint pass — no flicker (already the case).
- **Node fire growth:** when a node fires, the coloured circle grows — both the
  outer radius (new `GraphGeometry::getNodeRadius(intensity, cfg)`) and the ring
  thickness (`getNodeSpread`) scale with the flash envelope. Radius growth is
  config-driven via `nodeFlashRadiusGrow` in `settings.xml`.
- Node hover: subtle glow/outline while the pointer is over a node.
- Connection hover: highlight the hovered connection and show a weight tooltip
  (via `SettableTooltipClient` + a `TooltipWindow` owned by the editor).
- Background: dark grey-blue (#19191E) matching matsuoka_frontend (already the
  case).

### 7.5 Testing & QA

**Unit / integration tests (headless, `ctest`):**
- `TempoTest`: `mapTimeSignatureToGrid` mapping; ppq discontinuity triggers a
  reset + all-notes-off; transport stop clears held notes; transport start
  resumes without reset; BPM change updates node-0 frequency.
- `AutomationTest`: the fixed pool exists; host → controller (`setValue` on a
  node param updates controller state); controller → host (a mutator updates the
  APVTS value); the guard prevents feedback loops; preset round-trip preserves
  node params; a deleted node's slot resets.
- `GraphGeometryTest`: `getNodeRadius` grows monotonically with intensity.

**Manual QA (checklist, performed in a DAW — not automated here):**
- Load in Reaper / Ableton Live / Logic Pro (AU) / FL Studio.
- Transport start/stop, loop-region playback, tempo automation, time-signature
  changes → no hung notes, no crash.
- Automate a node parameter from the host → verify the UI follows.
- Save/reload the project → verify the network and node parameters restore.
- 16-node network → watch the audio callback for dropouts.
- Debug build → confirm the JUCE leak detector reports nothing.

---

## Phase 8 — Cross-Platform Distribution

**Goal:** Ship installable builds for macOS, Windows and Linux without needing a
toolchain, under a clear versioning policy.

A `.vst3` bundle is OS- and CPU-architecture-specific; the local `build/dev`
(Linux `x86_64`, Debug) cannot run on another platform. Cheapest reliable route
is free GitHub Actions runners building on `ubuntu-latest`, `windows-latest` and
`macos-latest`, attaching the POST_BUILD `Neurythmic.zip` artefact to a GitHub
Release.

### 8.1 CI Release Workflow (`.github/workflows/release.yml`)

- Trigger on `push` tags matching `v*` (and `workflow_dispatch`, which takes a
  `version` input defaulting to `0.0.0`; a dispatched run builds but does not
  create a Release).
- Repository is `toita86/NeurythmicVST`; the release job needs
  `permissions: contents: write` and uses the default `GITHUB_TOKEN`.
- Matrix job per OS:
  - **Linux:** `apt-get install -y libasound2-dev libfreetype6-dev libx11-dev
    libxcomposite-dev libxcursor-dev libxinerama-dev libxrandr-dev
    libxrender-dev libxext-dev libglu1-mesa-dev ninja-build`.
  - **Windows:** `ilammy/msvc-dev-cmd@v1` (the `release` preset uses the Ninja
    generator, which needs the MSVC environment).
  - **macOS:** nothing extra — `CMakeLists.txt` already sets
    `CMAKE_OSX_ARCHITECTURES "x86_64;arm64"` (universal binary).
- Configure + build: `cmake --preset release && cmake --build --preset release`
  (`Release`, LTO, tests off — the shippable build).
- **Version injection:** the workflow passes
  `-DNEURYTHMIC_VERSION=${GITHUB_REF_NAME#v}` at configure time so the bundle and
  the git tag can never disagree (see §8.2).
- **Artefact naming:** the POST_BUILD `Neurythmic.zip` is copied to a per-OS name
  including the CPU architecture —
  `Neurythmic-v<ver>-macOS-universal.zip`,
  `Neurythmic-v<ver>-Windows-x86_64.zip`,
  `Neurythmic-v<ver>-Linux-x86_64.zip` — and uploaded via
  `actions/upload-artifact`.
- On tag, create a GitHub Release (`softprops/action-gh-release`) attaching all
  three zips.

### 8.2 Versioning Policy

**Decision:** the first release is **`0.1.0`** — a pre-1.0 MVP. Until 1.0.0 the
project makes **no preset / automation compatibility guarantee**; parameter IDs
and the state schema may change between minor versions.

- **Single source of truth = the git tag.** `CMakeLists.txt` defines
  `NEURYTHMIC_VERSION` with a cache default of `0.0.0`; the release workflow
  overrides it from the pushed tag (`v0.1.0` → `0.1.0`). Local/dev builds keep
  the `0.0.0` fallback.
- Because a non-zero version lets AU hosts cache the component, **the version
  must be bumped on every release** for hosts to notice an update. `0.0.0`
  retains the development convenience that AU hosts always revalidate.
- The version is exposed to the UI/tests as the `NEURYTHMIC_VERSION` compile
  definition and shown as a small label in the CPG tab of the main menu.
- **Identity codes are frozen:** `COMPANY_NAME "Toita86"`,
  `PLUGIN_MANUFACTURER_CODE Toit`, `PLUGIN_CODE Ntpl`, product name
  `Neurythmic`. Hosts key automation/preset/project recall on these; changing
  them makes the host treat the build as a different plugin.
- **State schema version:** the `<Network>` root carries a `stateVersion`
  property (`kStateVersion = 1`), written by `initEmptyNetwork()` and preserved
  across save/load. Trees without the property (pre-Phase-8 presets) load as
  version 1, giving future migrations a hook.

### 8.3 Packaging & Install Instructions

- **VST3 manifest:** `VST3_AUTO_MANIFEST` is set to `TRUE` so JUCE generates a
  correct `moduleinfo.json` (class, version, category), avoiding validator
  warnings and keeping the version in one place.
- `assets/README.txt` (bundled in each zip) documents the per-OS install paths
  for macOS (`~/Library/Audio/Plug-Ins/VST3/`), Windows
  (`%LOCALAPPDATA%/Programs/Common/VST3/`) and Linux (`~/.vst3/`).
- `README.md` gains a "Download / Install" section pointing at
  `https://github.com/toita86/NeurythmicVST/releases`, plus the runtime
  dependency note.

### 8.4 Notes / Known Trade-offs

- **macOS Gatekeeper:** unsigned downloads are quarantined; colleagues may need
  right-click → Open. Notarization is out of scope.
- **Windows SmartScreen:** unsigned binaries may show a warning on first run.
- **Linux runtime deps:** end users still need `libfreetype6` + `libasound2`
  at load time (normally present on desktop distros) — noted in `README.md`.
- **macOS deployment target** stays `10.11` (supported by JUCE 8.0.12).

---

## Maybes (Deferred / Future Versions)

| Feature | Priority | Notes |
|---------|----------|-------|
| Undo/Redo via `juce::UndoManager` | Low | Nice DAW integration but not MVP |
| Standalone build target | Low | Primary use is DAW plugin |
| Internal FM synthesis (MatsuSynth) | Low | MIDI-out-only for now |
| Cross-backend rendering (Metal/DX) | Low | Software `juce::Graphics` already covers all platforms |
| Preset morphing / interpolation | Low | Future creative feature |
| MIDI CC output (mod wheel per node, etc.) | Low | Beyond note events |
| Python bindings for CPGLib | Low | Already partially in CPGLib repo |
| Lua scripting for generative logic | Low | Future extensibility idea |

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

