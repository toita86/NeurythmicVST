#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

namespace neurythmic {

// Transport/tempo snapshot read once per audio block.
struct TempoInfo {
  bool hasHost = false;    // a playhead is available (host present)
  bool isPlaying = false;  // transport is rolling
  double bpm = 120.0;      // current tempo (beats per minute)
};

// Seam around tempo/transport so the audio-thread logic is unit-testable
// without a real DAW. The real implementation wraps juce::AudioPlayHead;
// tests inject a FakeTempoSource.
class TempoSource {
public:
  virtual ~TempoSource() = default;
  virtual TempoInfo read() const = 0;
};

class PlayheadTempoSource : public TempoSource {
public:
  explicit PlayheadTempoSource(juce::AudioPlayHead* playhead = nullptr);

  void setPlayhead(juce::AudioPlayHead* playhead);
  TempoInfo read() const override;

private:
  juce::AudioPlayHead* _playhead;
};

class FakeTempoSource : public TempoSource {
public:
  TempoInfo info;
  TempoInfo read() const override { return info; }
};

}  // namespace neurythmic
