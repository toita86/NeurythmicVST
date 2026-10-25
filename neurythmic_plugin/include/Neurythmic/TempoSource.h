#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "MatsuokaEngine.h"

namespace neurythmic {

// Transport/tempo snapshot read once per audio block.
struct TempoInfo {
  bool hasHost = false;    // a playhead is available (host present)
  bool isPlaying = false;  // transport is rolling
  double bpm = 120.0;      // current tempo (beats per minute)

  // Musical position, used to detect loop jumps / seeks. `ppqPosition` is in
  // quarter notes since the start of the timeline.
  bool hasPpq = false;
  double ppqPosition = 0.0;

  // Host time signature (numerator/denominator).
  int timeSigNumerator = 4;
  int timeSigDenominator = 4;
};

// Maps a host time signature to one of CPGLib's two quantise grids. Both the
// numerator and denominator matter: the grids are 32nd-note steps per bar
// (x/4 -> x*8, x/8 -> x*4), so 3/4 -> 24, 4/4 -> 32, 6/8 -> 24. Other
// signatures choose the nearest grid; ties fall back to 32.
MatsuokaEngine::gridType mapTimeSignatureToGrid(int numerator, int denominator);

// Number of quarter-note beats in a bar for a time signature (6/8 -> 3.0).
// Used to convert host BPM into the quantiser's bars-per-minute tempo.
double beatsPerBarForSignature(int numerator, int denominator);

// True when the transport position moved in a way that a normal block of
// `expectedDelta` quarter notes cannot explain (a backward seek or an
// oversized forward jump), meaning the host looped or seeked.
bool isPositionJump(double previousPpq,
                    double currentPpq,
                    double expectedDelta);

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
