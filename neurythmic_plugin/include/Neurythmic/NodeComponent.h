#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace neurythmic {
class NodeComponent : public juce::Component {
public:
  void setLabel(const juce::String& label);
  void setAmplitude(double amp);
  void setFiring(bool firing);
  void setFrequency(double hz);

  void paint(juce::Graphics& g) override;
  void resized() override;

private:
  juce::String _label;
  double _amplitude = 0.0;
  bool _firing = false;
  double _frequency = 0.0;
  float _centerX = 0.0f, _centerY = 0.0f;
  int64_t _lastFireTime = 0;
};
}  // namespace neurythmic
