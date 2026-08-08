Weekly outcome:
Generate MIDI from CPG events

Why it matters:
This goal allows to undestrand if the underling sctructure is correct and allows a use in a DAW/host. 

Maximum scope:
Try different networks MIDI output in a DAW.

Minimum success:
Unit test that shows checks the MIDI output is correct

Tasks:
* Define a simple mapping from CPG node to MIDI note.
* Add channel, note and velocity defaults.
* Send output to a selected MIDI destination.
* Test with a DAW or software instrument.
* Ensure note-on messages have corresponding note-off messages.
* Document timestamping assumptions.

Proof:
Executable, test.

Blockers:
None

What I will not do:
Working on a UI editor for the network

---

Completed:

Planned implementation tasks is postponed, as a new codebased needs to be added to the project.
The previous implementation of Neurythmic follows a completelly different architecure based on the MVC pattern.
The main components were: [`CPGlib`](https://github.com/DanBennettDev/CPGLib), [`MAX/MSP backend`](https://github.com/DanBennettDev/max_cpg) and the [`frontend` ](https://github.com/DanBennettDev/matsuoka_frontend).

```text
Original Neurythmic simplified architecture:
  GUI → Presenter → OSC Engine → [network] → MAX/MSP → [OSC back] → noteEvents
  (3 layers of indirection to control the model)
```

The frontend was previously unavailable. Now that there is access, a ridefinition of the plan and architecure is required.

```text
Target NeurythmicVST essential architecture:
  GUI → MatsuokaEngine API → CPGLib → MIDI/Audio output
  (direct calls, single process, no serialisation) 
```

Building on the current state of the repository, the primary goal is to integrate the pre-existing frontend logic and components to address all the missing features to have complete successor to the original project. This integration allows also to simply the flow of the system.

Target Architecture:
```text
PluginProcessor (audio thread)
  ├── MatsuokaEngine (CPGLib) — step per sample
  ├── QuantisedEventQueue — tempo-synced grid
  ├── MIDI output via juce::MidiBuffer (firedNodes → NoteOn/Off)
  └── State persistence (juce::ValueTree + DAW params)

PluginEditor (GUI thread)
  ├── NetworkView — OpenGL graph (nodes, arcs, arrows, labels)
  ├── NodeMenu — right-click per-node (freq, constraints, MIDI)
  ├── MainMenu — sidebar (presets, CPG globals, mixer)
  └── Interaction — click/drag/shift-connect/right-click
```
