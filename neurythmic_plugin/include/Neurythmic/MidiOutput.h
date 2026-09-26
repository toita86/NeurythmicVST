#pragma once

#include <array>

#include <juce_audio_basics/juce_audio_basics.h>

namespace neurythmic {

enum class RoutingMode { DrumMachine, PerChannel };
enum class VelocityMode { Constant, Amplitude };

// Audio-thread side of "engine fire -> MIDI". Owns the note tracker and
// accumulates events with sample positions, then flushes them into a
// juce::MidiBuffer. No ValueTree / UI dependency, so channel and note are
// derived from (routing mode, node id, base note, drum channel).
class MidiOutput {
public:
  MidiOutput(int baseNote, int drumChannel);

  void setRoutingMode(RoutingMode mode);
  void setVelocityMode(VelocityMode mode);
  void setMasterVolume(float volume);  // 0..1

  // Registers a node fire at the given sample index. Retriggers: emits a
  // NoteOff of the held note, then a fresh NoteOn.
  void fire(int nodeId, float amplitude, int sampleIndex);

  // Writes accumulated events into the output buffer (sample-accurate) and
  // clears the pending store.
  void flush(juce::MidiBuffer& buffer);

  // Emits NoteOff for every currently-held note at the given sample index.
  void allNotesOff(int sampleIndex);

  // Clears held notes and any pending events (e.g. on state reload).
  void reset();

private:
  int channelFor(int nodeId) const;
  int noteFor(int nodeId) const;
  int velocityFor(float amplitude) const;

  int _baseNote;
  int _drumChannel;
  RoutingMode _routingMode = RoutingMode::DrumMachine;
  VelocityMode _velocityMode = VelocityMode::Amplitude;
  float _masterVolume = 0.8f;

  struct Held {
    bool held = false;
    int channel = 1;
    int note = 60;
  };
  std::array<Held, 16> _held;
  juce::MidiBuffer _pending;
};

}  // namespace neurythmic
