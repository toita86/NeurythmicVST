# Session ID
AE-2026-09-26-1

## Context
- Roadmap phase: Phase 2 — MIDI Output
- Development task: turn engine fire events into MIDI (routing modes, velocity
  mapping, host tempo/transport, DAW parameters, DAW state persistence).
- Duration:
    - time: 30 minutes 
    - usage: 215,374 tokens
    - cost: 0.86$
- Agent and model: opencode / deepseek-v4-pro
- Repository branch: main

## Evidence
- Commit: none yet (changes uncommitted at time of writing)
- Changed files:
  - new: neurythmic_plugin/{include/Neurythmic,source}/MidiOutput.{h,cpp}
  - new: neurythmic_plugin/{include/Neurythmic,source}/TempoSource.{h,cpp}
  - new: test/source/MidiOutputTest.cpp, test/source/main.cpp
  - modified: PluginProcessor.{h,cpp}, PluginEditor.{h,cpp},
    ConfigManager.{h,cpp}, Neurythmic.h, neurythmic_plugin.cpp,
    assets/settings.xml, test/CMakeLists.txt, docs/roadmap.md
- Tests: 74 passing (24 CPGLib + 50 plugin), 14 new tests in Phase 2
  (9 MidiOutput, 4 processBlock integration via FakeTempoSource, 1 state
  persistence round-trip). Zero assertion failures, zero leaks.
- Screenshot or build log: clean dev build (VST3 + Standalone), no warnings in
  new code.

## Factual account
Phase 2 was implemented test-first. Before writing code the roadmap was
reconciled with the current direction of the project that
disagreed with it; update it wherever an architectural decision changed.

Fourteen tests were written first against an API that did not yet exist
(red). Then: 
a TempoSource seam (TempoInfo {hasHost, isPlaying, bpm}, a real
PlayheadTempoSource wrapping juce::AudioPlayHead::getPosition(), and a
FakeTempoSource for tests); 
a MidiOutput class (two routing modes, retrigger =
NoteOff-then-NoteOn, velocity = clamp((amp*2)^2) * masterVolume * 127 or
masterVolume * 127 in constant mode, allNotesOff, reset); 
a new <MIDI> block in settings.xml (midiTriggerNote 60, midiDrumChannel 10) parsed by ConfigManager;
and a PluginProcessor wired with fireOnPeak=true, an
AudioProcessorValueTreeState of five parameters (rootFreq, masterVolume,
masterQuantAmount, velocityMode, routingMode), an event callback feeding
MidiOutput, and get/setStateInformation serialising the ValueTree to a gzip
MemoryBlock. The Start/Stop button and its _running flag were removed; the
editor now gates on transport isPlaying.

Three issues surfaced during build: juce::parseXML has no stream overload,
juce::Optional uses hasValue() rather than has_value(), and getCurrentPosition()
is deprecated in favour of getPosition(). More significantly, constructing the
APVTS starts a JUCE Timer, which asserted "MessageManager missing" in the
headless gtest process; this was resolved by giving the test target a custom
main() that holds a juce::ScopedJuceInitialiser_GUI. Pre-existing lint warnings
in NetworkController/PluginEditor were left untouched.

## Immediate researcher reflection
- What did I expect? A straightforward port of the MIDI plan already drafted in
  the weekly notes.
- What surprised me? That adding the DAW-parameter layer (APVTS) pulled in
  JUCE's event system and broke the headless test harness; and how cleanly the
  "two interchangeable routing modes" framing dissolved the audio/UI
  thread-boundary problem(having a per-node note/channel in the ValueTree would force the audio 
  thread to read the ValueTree, which phase_1 deliberately avoided. By using a global routingMode 
  switch with two interchangeable behaviours, all channel/note assignment 
  derived on the audio thread (no ValueTree reads needed)).
- What did I accept, modify, or reject? kept the old (amp*2)^2 velocity clamp; kept
  rootFreq as master and made internalTempo a derived display; insisted on a
  testable tempo-source seam; made the drum-machine mode the default and
  routingMode a DAW parameter.
- Where did my own JUCE, C++, DSP, or DAW knowledge matter? Choosing the
  precedence rules for tempo and the per-node note/channel deferral.
- How confident am I in the result, from 1 to 5, and why? 4 — tests are green
  and the build is clean, but the velocity curve and host-stop behaviour are
  unverified against a real DAW yet.

## Agent-generated prompts and researcher answers
### Qs set 1
A: docs/roadmap.md §2 and docs/weekly_plan/phase_2.md describe different Phase 2 MIDI models. Which is canonical for the build?

R: roadmap.md is the canonical one and should be updated with if there are any architecural changes decided

A: How should engine amplitude |lastMaxima| (not guaranteed 0–1) map to MIDI velocity 0–127?

R: Keep old (amp*2)^2 clamp

A: Confirm the Phase 2 DAW parameter set and transport behaviour?

R: Also keep rootFreq

A: How far should the test-first approach go in Phase 2?

R: Add a tempo-source seam

### Qs set 2
A: How do rootFreq (Hz) and internalTempo (BPM) relate, given they encode the same underlying value?

R: rootFreq master, internalTempo derived

A: Where does per-node MIDI note/channel live, and what is Phase 2's scope for it?

R: The midi behaviour should work as two different interchangable options, option 1 you have a drum machine style midi so one midi channel in which each node hits a different note and the rest is left to the daw. option 2 is that each node is a midi channel and you can tweak what node is hitting. 

### Qs set 3
A: Which routing mode is the default when a new/empty network starts?

R: Drum machine (single channel)

A: Where does the routingMode switch live?

R: DAW parameter

## Additional consideration
- Adding AudioProcessorValueTreeState forced a JUCE MessageManager, which broke the headless test harness — resolved with a custom main() holding ScopedJuceInitialiser_GUI. This is a "verification burden" data point.

- Pre-existing lint/-Wsign-conversion warnings remain in NetworkController.cpp and PluginEditor.cpp:83 — untouched (out of Phase 2 scope).

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
