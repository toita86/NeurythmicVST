# Session ID
AE-2026-10-08-1

## Context
- Roadmap phase: Phase 5 — Menu & Widget System
- Development task: right-click context menus (NodeMenu, ConnectionMenu) via
  `juce::CallOutBox`, a docked toggleable MainMenu sidebar, custom widgets
  (frequency/sync/grid/resolution matrices, ConstraintGUI), and the controller
  parameter mutators they drive (frequency, self-noise, phase, sync mode,
  quantise grid/multiple/offset/amount).
- Duration:
    - time: 2.5 hours 
    - usage: 16.5 M tokens
    - cost: 0.72$
- Agent and model: opencode / deepseek-v4-pro
- Repository branch: ADD-phase_5

## Evidence
- Commit: none yet (changes uncommitted at time of writing)
- Changed files:
  - new: include/Neurythmic/MenuValues.h, MenuTheme.h, RadioButtonMatrix.{h,cpp},
    ConstraintGUI.{h,cpp}, NodeMenu.{h,cpp}, ConnectionMenu.{h,cpp},
    MainMenu.{h,cpp}
  - modified: include/Neurythmic/NetworkController.{h,cpp} (parameter mutators +
    getters, connection getters, node-parameter defaults)
  - modified: include/Neurythmic/NetworkViewComponent.cpp (right-click opens
    menus)
  - modified: include/Neurythmic/PluginEditor.{h,cpp} (MainMenu sidebar,
    removed standalone "add child" button)
  - modified: include/Neurythmic/Neurythmic.h, neurythmic_plugin.cpp
    (module registration)
  - modified: CPGLib/CPG.{h,cpp}, MatsuokaEngine.{h,cpp} (getNodeSynchMode
    getter)
  - new: test/source/MenuTest.cpp
  - modified: test/CMakeLists.txt, docs/roadmap.md
  - new: include/Neurythmic/IconButton.{h,cpp} (drawn close/hamburger icons)
  - modified: include/Neurythmic/PresetManager.{h,cpp} (chooser lifetime,
    non-native, top-level dialog)
  - modified: include/Neurythmic/PluginEditor.cpp (resizable editor)
- Tests: 133 passing (24 CPGLib + 109 plugin; 17 new in test/source/MenuTest.cpp)
- Prompt transcript: this conversation
- Screenshot or build log: clean dev build (VST3 + Standalone), clang-tidy
  0 errors in new code

## Factual account
Phase 5 began with an architecture discussion. Four decisions were settled with
the researcher: the NodeMenu's MIDI tab (note/piano, octave, attack, decay,
velocity, env mode) is deferred and folded into Phase 6, because it is the only
Phase 5 item needing new MIDI/output semantics rather than UI-over-existing
state; context menus use `juce::CallOutBox` (custom content, no arrow, fall back
to a hand-rolled component if it looks wrong); the MainMenu is a docked,
toggleable 270px sidebar; and delivery is one phase, test-first, milestone by
milestone. `docs/roadmap.md` was updated to match (MIDI tab moved to §6.5,
CallOutBox note in §5.6, widget strategy in §5.5).

Development was test-first. A new `test/source/MenuTest.cpp` specifies the
controller parameter mutators (frequency, self-noise, phase offset, sync mode,
quantise grid/multiple/offset/amount) and their connection getters, plus a pure
`MenuValues` mapping namespace (frequency multiple/label snapping, sync-mode and
grid-type index maps, resolution multiples {8,4,2,1} and per-grid labels). To
support this, `NetworkController` gained the mutators (ValueTree first, engine
second, then `doQueuedActions`), connection getters, and a
`setNodeParameterDefaults` helper that initializes every node's noise/phase/sync
/quantise properties to engine defaults at creation; the preset `rebuild` was
extended to restore the quantise parameters it previously dropped. A
`getNodeSynchMode` getter was added to the engine (making `CPG::getNodeSynchMode`
const).

Two engine semantics were discovered through failing tests. The engine stores a
node's phase offset as a sample-quantised delay, so `setNodePhaseOffset`/`getNode
PhaseOffset` round-trips only approximately (the ValueTree, the source of truth,
stays exact). And the quantiser stores the grid *offset* as an `unsigned`, so the
float parameter truncates (0.5 → 0) — offsets are integer grid positions.

The GUI layer followed. A header-only `MenuTheme` palette ports the legacy
`ofxDatGuiThemeMatsuoka` colours; a generic `RadioButtonMatrix` (labelled radio
grid) underpins the frequency, sync, grid and resolution matrices; `ConstraintGUI`
draws the 24/32-line quantise grid. `NodeMenu` is a two-tab CallOutBox component
(Node: frequency matrix, fine-tune, sync mode, self-noise, phase, add/delete
child; Constraint: freedom, grid type, resolution, ConstraintGUI) that drives the
controller mutators live. `ConnectionMenu` exposes weight (edits the scale
factor), phase and remove. `MainMenu` is the docked sidebar (CPG tab with NEW
Preset / Save Preset File / Load Preset File, and a ✕ toggle). Right-click in
`NetworkViewComponent` now launches the relevant CallOutBox; the standalone
"add child" button was removed in favour of the NodeMenu.

Two scope reductions were made and recorded in the roadmap: the Constraint tab's
"Raw Mode" toggle is a synth `NEURALENV` parameter with no backing, so it was
deferred with the MIDI tab; and the MainMenu's preset dropdown/name-editor and
"Reload Settings" were omitted because the VST has no preset list manager and
settings.xml is baked into the binary (file-based save/load only). The
Constraint "Freedom" slider is inverted from the engine's quantise amount
(freedom = 1 − amount).

Manual host testing surfaced three issues after the note was first written.

The ✕ (close) and ☰ (hamburger) tab icons were Unicode glyphs missing from the
button font, so they rendered as a broken character. They were replaced with a
new `IconButton` component that draws the close-X and three-bar hamburger with
`juce::Graphics`, so they render regardless of the font.

Save/Load preset did nothing at first. An initial attempt passed a parent
component to `FileChooser::launchAsync`, whose third argument is actually a
`FilePreviewComponent` (a compile error caught that); the real bug was that the
`FileChooser` was a local `shared_ptr` destroyed the moment `browseForSave`/
`browseForLoad` returned, tearing the dialog down before it could appear. The
chooser is now stored as a `PresetManager::_activeChooser` member and uses the
non-native (JUCE) file browser, which is reliable inside a host.

In the standalone build this worked, but in Reaper two problems remained: the
editor was a fixed 500×500 non-resizable square (so the 270px sidebar filled half
of it), and the file dialog was parented to the sidebar and so clipped to its
270px bounds. The editor is now resizable (1000×700 default, 700×500 to
2560×1600), and the file dialog is a top-level window rather than a child of the
menu.

## Immediate researcher reflection
- What did I expect? A mostly mechanical port of the legacy menus into JUCE
  components over the existing controller.
- What surprised me? How much of Phase 5 was not UI at all — the MIDI tab had no
  backing state, and two engine semantics (sample-quantised phase, integer grid
  offset) only surfaced when the tests round-tripped them.
- What did I accept, modify, or reject? Accepted CallOutBox, a docked toggleable
  sidebar, and milestone-by-milestone delivery; deferred the MIDI tab and "Raw
  Mode" to Phase 6; reduced the MainMenu preset controls to file-based
  save/load rather than invent a preset library.
- Where did my own JUCE, C++, DSP, or DAW knowledge matter? Choosing CallOutBox
  for dismissal/edge-clamping, the `int → unsigned` engine-API boundaries, and
  recognizing that "freedom" in the UI inverts the engine's "quantise amount".
- How confident am I in the result, from 1 to 5, and why? 3 — the controller
  logic is unit-tested and the build is clean, but the menus' layout/feel and
  the CallOutBox behaviour have not been exercised in a host yet.

Follow-up after manual testing in standalone and Reaper:
- The Unicode icon failure and the FileChooser lifetime bug were both things the
  unit tests could not catch — they only surfaced in a running host.
- First iteration of save/load fix (adding a parent component) was a wrong diagnosis; the
  lifetime bug was the actual cause, and that same parent later *caused* the
  clipping reported in Reaper. Undoing it and going top-level was the real fix.
- Confidence revised up to 4 for the controller/engine logic (still 133
  passing), but the GUI layer still needs one more pass in a DAW to confirm the
  resizable window and top-level dialog behave on Windows/macOS, not just Linux.

## Agent-generated prompts and researcher answers
- Agent: "The NodeMenu MIDI tab has no backing state — defer, implement fully,
  or stub?" Researcher: defer it and fold it into Phase 6 (it belongs with the
  per-node mixer/MIDI channel work).
- Agent: "How should the right-click menus be implemented?" Researcher: use
  `juce::CallOutBox`; fall back to a custom component if the result is not
  satisfying.
- Agent: "How should the 270px MainMenu be presented?" Researcher: docked,
  toggleable sidebar.
- Agent: "One phase or sub-steps?" Researcher: one phase, TDD milestone by
  milestone.

## Provisional codes
- AI-assisted reverse engineering
- architectural translation
- verification burden
- trust calibration
- real-time safety
- preservation versus modernization

## AI involvement declaration
The agent organized the evidence and proposed questions and codes.
The factual and reflective account was reviewed by the researcher.
