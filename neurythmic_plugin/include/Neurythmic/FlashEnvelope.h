#pragma once

#include <cmath>

namespace neurythmic {

// Visual attack-decay envelope for a node "fire" flash. Port of the old
// matsuoka_frontend GUI_AD_Ramp.
//
// Stepped once per editor frame; construct with the timer rate (30 Hz) as
// `sampleRate` so attack/decay are in real milliseconds.
class FlashEnvelope {
public:
  explicit FlashEnvelope(double sampleRate,
                         double attackMs = 50.0,
                         double decayMs = 500.0,
                         double curveExpo = 1.0);

  void trigger(double velocity = 1.0);  // begin RISING
  void step();                          // advance one frame
  double getValue() const;              // current 0..1 value
  bool getChanged() const;              // true while not IDLE
  void reset();                         // back to zero / IDLE

private:
  enum class State : char { Idle, Rising, Falling };

  double _currVal = 0.0;
  double _attack;
  double _decay;
  double _curve;
  double _velocity = 1.0;
  double _sampleRate;
  State _state = State::Idle;

  double attackChange() const { return 1000.0 / (_attack * _sampleRate); }
  double decayChange() const { return -1000.0 / (_decay * _sampleRate); }
};

}  // namespace neurythmic
