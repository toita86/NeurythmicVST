# Retrospective reconstruction — Phase 0 and Phase 1

Date: 2026-09-26

## Phase 0

Entry type: Retrospective reconstruction
Contemporaneous evidence: roadmap.md, git history, test output
Later reflection: written after adopting the autoethnographic method
Confidence in reconstruction: Medium

### Factual account
- CPGLib built as a static library (MatsuokaEngine, QuantisedEventQueue, ScalingCurve).
- JUCE 8.0.12 plugin scaffold (VST3/AU), CMake with CPM, Ninja, ccache.
- Basic 3-node hardcoded network with a Start/Stop button.
- Placeholder NodeComponent (pulsing circle, hard 150 ms flash on fire).
- 29 tests (20 CPGLib + 9 plugin), clang-format, clang-tidy, C++23.

### Researcher input
- Create a basic juce scaffold project an link the CPGLib as a static library, be sure to add test and add a small 3 node network to ensure that the library works property.

### Agent questions
- None.

## Phase 1

Entry type: Retrospective reconstruction
Contemporaneous evidence: AI transcript, git commits, roadmap.md, test output
Later reflection: written after adopting the autoethnographic method
Confidence in reconstruction: High

Autoethnographic focus: AI-assisted architecture reconstruction.

### Factual account
- 1.1 ConfigManager: singleton that loads embedded settings.xml and scalingCurve.txt with juce::XmlDocument. Config exposed as read-only const references. Obsolete tags removed (synth, OSC flags). Wired into PluginProcessor (CPG params c/b/g/t1Overt2, freqCompensation, weight-scaling curve, calibrate).
- 1.2 NetworkState: juce::ValueTree schema (Network -> Node -> Connection) with Identifier property names and helpers. Replaced the flat _nodes[16] array and dirty flags.
- 1.4 FlashEnvelope: direct port of GUI_AD_Ramp (Idle -> Rising -> Falling state machine), pure math, no JUCE.
- 1.3 NetworkController: owns the ValueTree and 16 FlashEnvelopes, bridges to MatsuokaEngine. Rule: mutate the tree first, then the engine, then doQueuedActions(). Node and connection lifecycle, positioning, focus, updateFromEngine().
- 1.5 PresetManager: save/load the ValueTree as XML (.nprs) via ValueTree::createXml/fromXml, plus NetworkController::rebuild() to restore the engine.
- Wiring: PluginProcessor owns engine + controller; PluginEditor has a dynamic NodeComponent list; NodeComponent became a passive renderer.

### Bugs found and fixed
- Focus struct contained a member of its own type by value (incomplete type error). Split into _focus and _prevFocus.
- New child nodes were created but never laid out (0x0 bounds), so they were invisible.
- Editor buttons overlapped; a getNodeCount()==1 guard limited adding to one child.
- t1Overt2 was missing from the ConfigManager-to-engine wiring.

### Researcher input
- Pasted the full Phase 1 roadmap (bite-sized tasks, one JUCE concept each).
- Asked to partecipate to the implementation: "I partially want to implement some features myself, help me undestand how JUCE relates to the architectural chooices made".
- This looked like one of the most important architectural factor that potentially can shape any subsequent implementation, I wanted to be more in control of it.
- Reported compile errors and runtime behaviour back to the agent (Focus error; "create child does not work, why is it pulsating and becoming yellow").
- Answered all clarifying questions below.

### Agent questions and answers
- Startup network shape? -> single root node.
- Phase 1 editor view? -> minimal dynamic node list.
- ValueTree schema scope? -> engine + visual properties only (MIDI fields deferred).
- Where should the FlashEnvelope live? -> controller owns it, NodeComponent stays a passive renderer.

### Provisional codes
- AI-assisted reverse engineering
- architectural translation
- verification burden
- trust calibration
- preservation versus modernization

### AI involvement declaration
The agent proposed the questions, design decisions, and code sketches; the researcher reviewed and applied them and drove the implementation.

## Traceability
The commit 4fee87fe7b770da3370357572bef7ed1ae4a93de is last commit related to this phases.
