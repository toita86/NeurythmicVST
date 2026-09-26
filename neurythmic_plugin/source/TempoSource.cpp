#include "../include/Neurythmic/TempoSource.h"

namespace neurythmic {

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
  const auto bpm = pos->getBpm();
  if (bpm.hasValue() && *bpm > 0.0)
    info.bpm = *bpm;
  return info;
}

}  // namespace neurythmic
