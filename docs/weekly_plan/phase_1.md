# Phase 1 - State Model & Configuration

**Goal:** define the canonical data model that everything else depends on.

---

## Final Architecture

```
settings.xml / scalingCurve.txt (embedded binary)
        |
ConfigManager (singleton)          MatsuokaEngine (CPGLib, audio thread)
        |                                      |
        +-----------------+                    |  queued actions / getEvents()
                          v                    v
                    NetworkController 
                       |  owns:
                       +-- ValueTree root   (single source of truth)
                       +-- FlashEnvelope[16]
                       |
              +--------+---------+
              |                  |
     PresetManager         PluginProcessor / PluginEditor / NodeComponent
     (XML <-> ValueTree)   (consume tree + envelopes)
```

---

## The Five Pieces

| # | Task | What it does | JUCE / pattern taught |
|---|------|--------------|-----------------------|
| 1.1 | **ConfigManager** | Singleton loads `settings.xml` + `scalingCurve.txt`; exposes values as `const&` | `juce::XmlDocument` / `XmlElement` DOM; singleton pattern |
| 1.2 | **NetworkState** | ValueTree schema: `Network -> Node -> Connection`, property `Identifier`s, helpers | `juce::ValueTree` (shared handles), `juce::Identifier` (string pool), `juce::var` |
| 1.4 | **FlashEnvelope** | AD ramp port of `GUI_AD_Ramp`: `trigger -> step -> getValue` | pure-math class, state machine |
| 1.3 | **NetworkController** | Owns tree + envelopes; bridges to engine; node/connection lifecycle, positioning, focus, `updateFromEngine()` | ValueTree-first -> engine-second, mediator pattern |
| 1.5 | **PresetManager** | Save/load tree as XML (`.nprs`); `rebuild()` restores engine | `ValueTree::createXml`/`fromXml`, `juce::File`, `juce::FileChooser` |

---

## Key Architectural Decisions

- **Single source of truth** = the `ValueTree`. The flat `_nodes[16]` array + dirty flags are gone.
- **Node identity** = `id` property, looked up via `getChildWithProperty` (replaces `_nodes[id]`).
- **Connections live on the target node** as child `Connection` trees (mirrors the engine's input-on-target model).
- **Parent-child encoded twice**: `parentId` property **and** a `Connection` (`sourceId == parentId`).
- **ValueTree-first, engine-second**, then `doQueuedActions()` - transactional safety.
- **Observer notifications** (`ValueTree::Listener`) replace `changed` / `changedPosition` dirty flags.
- **Event polling moved off the audio thread** - `processBlock` only `step()`s; the controller's `updateFromEngine()` consumes `getEvents()` on the message thread.
- **MIDI-only** - the OSC-engine synth methods (pitch/vol/mute) don't exist here, so the port dropped them.

---

## Wiring (the glue)

- `PluginProcessor`: owns `MatsuokaEngine _engine` + `NetworkController _controller`; sets CPG params (`t1Overt2`, `c`, `b`, `g`, `freqCompensation`), weight-scaling curve, `calibrate()`. `_setupNetwork()` replaced by the single-root controller.
- `PluginEditor`: dynamic `NodeComponent` list rebuilt from the tree; `timerCallback` -> `controller.updateFromEngine()` -> amplitude/freq/intensity per node.
- `NodeComponent`: passive renderer (radius = amplitude, colour lerp = flash intensity).

---

## Result

Unit tests covering ConfigManager, NetworkState, FlashEnvelope, NetworkController and PresetManager, plus a working plugin that spawns and flashes child nodes.

---

## Next

**Phase 2 - MIDI Output** (first playable milestone).
