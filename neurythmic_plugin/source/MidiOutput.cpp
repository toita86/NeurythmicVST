#include "../include/Neurythmic/MidiOutput.h"

#include <algorithm>
#include <cmath>

namespace neurythmic {

MidiOutput::MidiOutput(int baseNote, int drumChannel)
    : _baseNote(baseNote), _drumChannel(drumChannel) {
  for (size_t i = 0; i < _nodeNote.size(); ++i) {
    _nodeNote[i] = baseNote + static_cast<int>(i);
    _nodeGain[i] = 1.0f;
    _nodeEnabled[i] = true;
  }
}

void MidiOutput::setRoutingMode(RoutingMode mode) {
  _routingMode = mode;
}

void MidiOutput::setVelocityMode(VelocityMode mode) {
  _velocityMode = mode;
}

void MidiOutput::setMasterVolume(float volume) {
  _masterVolume = std::clamp(volume, 0.0f, 1.0f);
}

void MidiOutput::setNodeNote(int nodeId, int note) {
  if (nodeId < 0 || nodeId >= static_cast<int>(_nodeNote.size()))
    return;
  note = std::clamp(note, 0, 127);
  if (_nodeNote[static_cast<size_t>(nodeId)] == note)
    return;
  _nodeNote[static_cast<size_t>(nodeId)] = note;

  // Changing a held note must release the old one to avoid a hung note.
  auto& held = _held[static_cast<size_t>(nodeId)];
  if (held.held) {
    _pending.addEvent(juce::MidiMessage::noteOff(held.channel, held.note), 0);
    held.held = false;
  }
}

void MidiOutput::setNodeGain(int nodeId, float gain) {
  if (nodeId < 0 || nodeId >= static_cast<int>(_nodeGain.size()))
    return;
  _nodeGain[static_cast<size_t>(nodeId)] = std::max(0.0f, gain);
}

void MidiOutput::setNodeEnabled(int nodeId, bool enabled) {
  if (nodeId < 0 || nodeId >= static_cast<int>(_nodeEnabled.size()))
    return;
  _nodeEnabled[static_cast<size_t>(nodeId)] = enabled;
  if (!enabled) {
    auto& held = _held[static_cast<size_t>(nodeId)];
    if (held.held) {
      _pending.addEvent(juce::MidiMessage::noteOff(held.channel, held.note), 0);
      held.held = false;
    }
  }
}

int MidiOutput::channelFor(int nodeId) const {
  if (_routingMode == RoutingMode::PerChannel)
    return nodeId + 1;
  return _drumChannel;
}

int MidiOutput::noteFor(int nodeId) const {
  if (nodeId < 0 || nodeId >= static_cast<int>(_nodeNote.size()))
    return _baseNote;
  return _nodeNote[static_cast<size_t>(nodeId)];
}

int MidiOutput::velocityFor(int nodeId, float amplitude) const {
  float normalised = 1.0f;
  if (_velocityMode == VelocityMode::Amplitude)
    normalised = std::clamp(std::pow(amplitude * 2.0f, 2.0f), 0.0f, 1.0f);

  const float gain =
      (nodeId >= 0 && nodeId < static_cast<int>(_nodeGain.size()))
          ? _nodeGain[static_cast<size_t>(nodeId)]
          : 1.0f;
  const float scaled = normalised * gain * _masterVolume * 127.0f;
  return std::clamp(static_cast<int>(scaled), 0, 127);
}

void MidiOutput::fire(int nodeId, float amplitude, int sampleIndex) {
  if (nodeId < 0 || nodeId >= static_cast<int>(_held.size()))
    return;
  if (!_nodeEnabled[static_cast<size_t>(nodeId)])
    return;

  const int channel = channelFor(nodeId);
  const int note = noteFor(nodeId);
  const int velocity = velocityFor(nodeId, amplitude);

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
