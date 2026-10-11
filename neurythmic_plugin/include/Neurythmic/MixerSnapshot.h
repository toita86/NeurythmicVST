#pragma once

#include <array>
#include <atomic>
#include <cstddef>

namespace neurythmic {

constexpr int kMaxMixerNodes = 16;

// Message-thread -> audio-thread bridge for per-node MIDI state.
//
// NetworkController (message thread) writes the three per-node fields; the
// PluginProcessor (audio thread) reads them once per block and copies them into
// MidiOutput. Every slot is a plain std::atomic, so no lock or allocation is
// needed across the thread boundary. Out-of-range access is a safe no-op.
class MixerSnapshot {
public:
  MixerSnapshot() {
    for (int i = 0; i < kMaxMixerNodes; ++i) {
      _note[static_cast<std::size_t>(i)].store(0, std::memory_order_relaxed);
      _gain[static_cast<std::size_t>(i)].store(1.0f, std::memory_order_relaxed);
      _enabled[static_cast<std::size_t>(i)].store(true,
                                                  std::memory_order_relaxed);
    }
  }

  void setNote(int nodeId, int note) {
    if (inRange(nodeId))
      _note[idx(nodeId)].store(note, std::memory_order_relaxed);
  }
  void setGain(int nodeId, float gain) {
    if (inRange(nodeId))
      _gain[idx(nodeId)].store(gain, std::memory_order_relaxed);
  }
  void setEnabled(int nodeId, bool enabled) {
    if (inRange(nodeId))
      _enabled[idx(nodeId)].store(enabled, std::memory_order_relaxed);
  }

  int note(int nodeId) const {
    return inRange(nodeId) ? _note[idx(nodeId)].load(std::memory_order_relaxed)
                           : 0;
  }
  float gain(int nodeId) const {
    return inRange(nodeId) ? _gain[idx(nodeId)].load(std::memory_order_relaxed)
                           : 1.0f;
  }
  bool enabled(int nodeId) const {
    return inRange(nodeId)
               ? _enabled[idx(nodeId)].load(std::memory_order_relaxed)
               : true;
  }

private:
  static bool inRange(int nodeId) {
    return nodeId >= 0 && nodeId < kMaxMixerNodes;
  }
  static std::size_t idx(int nodeId) {
    return static_cast<std::size_t>(nodeId);
  }

  std::array<std::atomic<int>, kMaxMixerNodes> _note;
  std::array<std::atomic<float>, kMaxMixerNodes> _gain;
  std::array<std::atomic<bool>, kMaxMixerNodes> _enabled;
};

}  // namespace neurythmic
