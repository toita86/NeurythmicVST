# Session ID
AE-2026-10-06-1

## Context
- Roadmap phase: Phase 4 — Interaction Layer
- Development task: mouse-driven network editing (selection, hit-testing,
  node dragging with weight recompute, shift+click connection creation,
  move-all pan, alt+click node reset)
- Duration:
    - time: 2.5 hours 
    - usage: 18.1 M tokens
    - cost: 1.65$
- Agent and model: opencode / deepseek-v4-pro
- Repository branch: ADD-phase_4

## Evidence
- Commit: none yet (changes uncommitted at time of writing)
- Changed files:
  - modified: GraphGeometry.{h,cpp} (connection geometry + hit-testing)
  - modified: NetworkState.{h,cpp} (scaleFactor connection property)
  - modified: NetworkController.{h,cpp} (selection, hit-test, drag, toggle)
  - modified: NetworkViewComponent.{h,cpp} (mouse handlers, selection highlight)
  - new: test/source/InteractionTest.cpp
  - modified: test/source/GraphGeometryTest.cpp, test/CMakeLists.txt
  - modified: docs/roadmap.md
- Tests: 115 passing (24 CPGLib + 91 plugin; 17 new — 7 GraphGeometry
  hit-test/geometry + 10 Interaction)
- Prompt transcript: this conversation
- Screenshot or build log: clean dev build (VST3 + Standalone), clang-tidy
  0 errors in new code

## Factual account
Phase 4 began with an architecture discussion rather than code. Five decisions
were settled with the researcher, all resolved in the faithful direction:
connection weight stays *derived* from node distance via a per-connection
`scaleFactor` (so dragging a node recomputes its weights live); full Ctrl+click
multi-select with group drag; right-button drag on empty space pans the whole
graph; Alt+click resets a node; and right-click selects + sets focus only, with
the context menus deferred to Phase 5.

Development was test-first. The connection geometry was unified: a new
`GraphGeometry::makeConnectionGeometry()` produces the same pixel-space
arc/segment that the renderer draws, and the hit-testers
(`isStraightConnectionAtPoint`, `isCurvedConnectionAtPoint`) consume it, so the
clickable region always matches the drawn shape. A `scaleFactor` property was
added to the Connection in the ValueTree; `createChild`/`addConnection` store the
parent-child (1.0) vs input-edge (0.0) defaults and derive `weight =
calcWeight(distance, scaleFactor)`. Selection was implemented as transient
controller state (a node-ID set plus one optional connection), explicitly not
serialized into the ValueTree. `NetworkController` gained `toggleConnection`,
`connectionAtPoint`, `getIsConnected`, the drag trio
(`setNodePositionOffsets`/`moveSelectedNodes`/`moveAllNodes`) and `resetNode`;
`NetworkViewComponent` gained `mouseDown/mouseDrag/mouseUp` mapping
`juce::ModifierKeys` onto those calls and drawing selection in `selectedColour`.

Two legacy quirks surfaced. First, the input-edge arc-centre formula in the
legacy `GraphVis::makeInputEdge` uses a non-normalised perpendicular, so the arc
centre sits far outside the drawn radius; this was ported verbatim so the hit
test stays consistent with the renderer, but it is worth a visual check when
input edges first appear on screen. Second, the legacy Alt+click "reset node"
was a placeholder that actually reset node 0; it was re-implemented as a
per-node reset-to-zero of the oscillator state (CPGLib's `reset` has no
single-argument form). The legacy `((from+1)*1000)+to` connection keying and the
dead `actionInProgress == addInput` path were dropped.

Both quirks were confirmed on manual testing. Shift+click input edges rendered
as a broken "looping arrow" — the predicted arc-centre inconsistency, now fixed
by normalising the perpendicular in `makeInputEdge` (with a regression test for
a long chord). Alt+click left a node firing a very weak signal — the
reset-to-zero was starving the Matsuoka oscillator, so a single-argument
`MatsuokaEngine::reset(nodeID)` was added to restore the engine's default
initial state (X1_INIT etc.) rather than zeros.

## Immediate researcher reflection
- What did I expect? A mechanical port of the legacy `GUI_NetworkView` mouse
  handling into `juce::Component` callbacks.
- What surprised me? The legacy input-edge geometry is internally inconsistent
  (arc centre vs radius), and the legacy Alt-click reset was a stub — both
  reminders that "faithful" can mean faithfully reproducing a bug.
- What did I accept, modify, or reject? Accepted the derived-weight
  (`scaleFactor`) model, full multi-select, move-all and alt-reset; modified
  selection into transient controller state and connection identity into
  `(sourceId, targetId)` pairs; rejected serializing selection into the
  ValueTree and the dead `addInput` code path.
- Where did my own JUCE, C++, DSP, or DAW knowledge matter? ModifierKeys and
  MouseEvent plumbing, the point-in-quadrilateral raycast, and keeping the
  engine mutations on the queued-action path (ValueTree first, engine second).
- How confident am I in the result, from 1 to 5, and why? 3 — hit-testing and
  the interaction logic are unit-tested and the build is clean, but the feel of
  the gestures and the visual appearance of input edges need manual DAW
  verification.

## Agent-generated prompts and researcher answers
- Agent: "Should connection weight be derived (scaleFactor) and recomputed on
  drag, or static?" Researcher: derived — store scaleFactor, recompute on drag.
- Agent: "How far should multi-select go in Phase 4?" Researcher: full
  Ctrl+click multi-select with group drag.
- Agent: "Keep the right-button-drag move-all gesture?" Researcher: keep it.
- Agent: "Include Alt+click node reset?" Researcher: include it.
- Agent: "What should right-click do before the menus exist?" Researcher:
  select + set focus only; menus arrive in Phase 5.

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
