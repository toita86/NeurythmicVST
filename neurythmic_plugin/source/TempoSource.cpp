#include "../include/Neurythmic/TempoSource.h"

#include <cstdlib>

namespace neurythmic {

MatsuokaEngine::gridType mapTimeSignatureToGrid(int numerator,
                                                int denominator) {
  if (numerator <= 0 || denominator <= 0)
    return MatsuokaEngine::gridType::_32nd;

  // Number of 32nd-note steps in one bar: numerator * 32 / denominator.
  const int steps = (numerator * 32) / denominator;
  const int distance24 = std::abs(steps - 24);
  const int distance32 = std::abs(steps - 32);
  return distance32 <= distance24 ? MatsuokaEngine::gridType::_32nd
                                  : MatsuokaEngine::gridType::_24th;
}

double beatsPerBarForSignature(int numerator, int denominator) {
  if (numerator <= 0 || denominator <= 0)
    return 4.0;
  return (static_cast<double>(numerator) * 4.0) /
         static_cast<double>(denominator);
}

bool isPositionJump(double previousPpq,
                    double currentPpq,
                    double expectedDelta) {
  const double delta = currentPpq - previousPpq;
  if (delta < -1e-6)
    return true;  // backward jump (loop / seek backwards)
  // A forward jump larger than one block plus a generous tolerance (half a
  // beat) cannot be normal playback.
  return delta > expectedDelta + 0.5;
}

PlayheadTempoSource::PlayheadTempoSource(juce::AudioPlayHead* playhead)
    : _playhead(playhead) {}

void PlayheadTempoSource::setPlayhead(juce::AudioPlayHead* playhead) {
  _playhead = playhead;
}

TempoInfo PlayheadTempoSource::read() const {
  TempoInfo info;
  if (_playhead == nullptr)
    return info;  // no host: fall back to the internal clock

  const auto pos = _playhead->getPosition();
  if (!pos.hasValue())
    return info;  // host present but position unavailable

  info.hasHost = true;
  info.isPlaying = pos->getIsPlaying();
  if (const auto bpm = pos->getBpm(); bpm.hasValue() && *bpm > 0.0)
    info.bpm = *bpm;

  if (const auto ppq = pos->getPpqPosition(); ppq.hasValue()) {
    info.hasPpq = true;
    info.ppqPosition = *ppq;
  }

  if (const auto sig = pos->getTimeSignature(); sig.hasValue()) {
    info.timeSigNumerator = sig->numerator;
    info.timeSigDenominator = sig->denominator;
  }

  return info;
}

}  // namespace neurythmic
