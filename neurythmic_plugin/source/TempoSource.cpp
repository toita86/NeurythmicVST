#include "../include/Neurythmic/TempoSource.h"

namespace neurythmic {

MatsuokaEngine::gridType mapTimeSignatureToGrid(int numerator,
                                                int denominator) {
  juce::ignoreUnused(numerator);
  if (denominator == 8)
    return MatsuokaEngine::gridType::_24th;
  // Default: x/4 (and anything unrecognised) uses the 32nd grid.
  return MatsuokaEngine::gridType::_32nd;
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
