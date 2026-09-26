#include "../include/Neurythmic/MidiOutput.h"

#include <algorithm>
#include <cmath>

namespace neurythmic {

MidiOutput::MidiOutput(int baseNote, int drumChannel)
    : _baseNote(baseNote), _drumChannel(drumChannel) {}

void MidiOutput::setRoutingMode(RoutingMode mode) {
  _routingMode = mode;
}

void MidiOutput::setVelocityMode(VelocityMode mode) {
  _velocityMode = mode;
}

void MidiOutput::setMasterVolume(float volume) {
  _masterVolume = std::clamp(volume, 0.0f, 1.0f);
}

int MidiOutput::channelFor(int nodeId) const {
  if (_routingMode == RoutingMode::PerChannel)
    return nodeId + 1;
  return _drumChannel;
}

int MidiOutput::noteFor(int nodeId) const {
  if (_routingMode == RoutingMode::DrumMachine)
    return _baseNote + nodeId;
  return _baseNote;
}

int MidiOutput::velocityFor(float amplitude) const {
  float normalised = 1.0f;
  if (_velocityMode == VelocityMode::Amplitude)
    normalised = std::clamp(std::pow(amplitude * 2.0f, 2.0f), 0.0f, 1.0f);

  const float scaled = normalised * _masterVolume * 127.0f;
  return std::clamp(static_cast<int>(scaled), 0, 127);
}

void MidiOutput::fire(int nodeId, float amplitude, int sampleIndex) {
  if (nodeId < 0 || nodeId >= static_cast<int>(_held.size()))
    return;

  const int channel = channelFor(nodeId);
  const int note = noteFor(nodeId);
  const int velocity = velocityFor(amplitude);

  auto& held = _held[static_cast<size_t>(nodeId)];
  if (held.held)
    _pending.addEvent(juce::MidiMessage::noteOff(held.channel, held.note),
                      sampleIndex);

  _pending.addEvent(juce::MidiMessage::noteOn(
                        channel, note, static_cast<juce::uint8>(velocity)),
                    sampleIndex);
  held = {true, channel, note};
}

void MidiOutput::flush(juce::MidiBuffer& buffer) {
  buffer.addEvents(_pending, 0, -1, 0);
  _pending.clear();
}

void MidiOutput::allNotesOff(int sampleIndex) {
  for (auto& held : _held) {
    if (held.held) {
      _pending.addEvent(juce::MidiMessage::noteOff(held.channel, held.note),
                        sampleIndex);
      held.held = false;
    }
  }
}

void MidiOutput::reset() {
  for (auto& held : _held)
    held.held = false;
  _pending.clear();
}

}  // namespace neurythmic
