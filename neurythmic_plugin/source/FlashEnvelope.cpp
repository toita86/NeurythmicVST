#include "../include/Neurythmic/FlashEnvelope.h"

#include <algorithm>

namespace neurythmic {
FlashEnvelope::FlashEnvelope(double frameRate,
                             double attackMs,
                             double decayMs,
                             double curveExpo)
    : _attack(attackMs),
      _decay(decayMs),
      _curve(curveExpo),
      _frameRate(frameRate) {}

void FlashEnvelope::trigger(double velocity) {
  _velocity = std::clamp(velocity, 0.0, 1.0);
  _state = State::Rising;
  _currVal = 0.0;
};

void FlashEnvelope::step() {
  switch (_state) {
    case State::Idle:
      break;
    case State::Rising:
      _currVal += attackChange();
      if (_currVal >= 1.0) {
        _currVal = 2.0 - _currVal;  // reflect the overshoot -> decay
        _state = State::Falling;
      }
    case State::Falling:
      _currVal += decayChange();
      if (_currVal <= 0.0) {
        _currVal = 0.0;
        _state = State::Idle;
      }
      break;
  }
}

double FlashEnvelope::getValue() const {
  return std::pow(_currVal * _velocity, _curve);
}

bool FlashEnvelope::getChanged() const {
  return _state != State::Idle;
}

void FlashEnvelope::reset() {
  _currVal = 0.0;
  _state = State::Idle;
}
}  // namespace neurythmic
