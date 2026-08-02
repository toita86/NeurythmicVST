#include "../include/Neurythmic/NodeComponent.h"

namespace neurythmic {

void NodeComponent::setLabel(const juce::String& label) {
  _label = label;
}
void NodeComponent::setAmplitude(double amp) {
  _amplitude = amp;
}
void NodeComponent::setFiring(bool firing) {
  if (firing)
    _lastFireTime = juce::Time::currentTimeMillis();
  _firing = firing;
}
void NodeComponent::setFrequency(double hz) {
  _frequency = hz;
}
void NodeComponent::paint(juce::Graphics& g) {
  float radius = 20.0f + (_amplitude * 60.0f);

  auto msSinceFire = juce::Time::currentTimeMillis() - _lastFireTime;
  bool flashing = (msSinceFire < 150);

  juce::Colour colour = flashing ? juce::Colours::yellow.brighter(0.5f)
                                 : juce::Colour(40, 100, 180);

  g.setColour(colour);
  g.fillEllipse(_centerX - radius, _centerY - radius, radius * 2.0f,
                radius * 2.0f);

  g.setColour(juce::Colours::white);
  g.setFont(16.0f);
  g.drawText(_label, getLocalBounds().removeFromTop(25),
             juce::Justification::centred);

  g.setFont(14.0f);
  g.drawText(juce::String(_frequency, 1) + " Hz",
             getLocalBounds().removeFromBottom(25),
             juce::Justification::centred);
}

void NodeComponent::resized() {
  _centerX = getWidth() / 2.0f;
  _centerY = getHeight() / 2.0f;
}
}  // namespace neurythmic
