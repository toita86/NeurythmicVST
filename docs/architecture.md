This is a proposed not a final rapresentation of the final state.

cpgLib
  |
  v
CPG Adapter
- translates raw library API
- controls simulation stepping
- exposes node/network state
- converts triggers into neutral events
  |
  v
Musical Model
- tempo and transport
- node-to-MIDI mapping
- preset/state format
- rhythm-event scheduling
  |
  +------------------------------------+
  |                                    |
  v                                    v
Standalone Application                Plugin Adapter
- tablet lifecycle                    - host transport
- MIDI devices                        - processBlock scheduling
- touch interaction                   - host state
  |
  v
Shared Presentation Model
- selected node
- display positions
- edit commands
- visual activity state
