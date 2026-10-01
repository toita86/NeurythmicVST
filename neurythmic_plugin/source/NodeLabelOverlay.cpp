#include "../include/Neurythmic/NodeLabelOverlay.h"

#include <algorithm>
#include <cmath>

#include "NeurythmicPluginAssets.h"
#include "../include/Neurythmic/NetworkState.h"

namespace neurythmic {

namespace {

// Snap a frequency multiple to the legacy display increments (1/16 below 1,
// 1/8 above, with 1/3 special-cased) and format it as a trimmed decimal.
juce::String formatFreqMultiple(double m) {
  if (m < 0.0)
    m = 0.0;

  double snapped;
  if (std::fabs(m - 1.0 / 3.0) < 0.05) {
    snapped = 1.0 / 3.0;
  } else if (m < 1.0) {
    snapped = std::round(m * 16.0) / 16.0;
  } else {
    snapped = std::round(m * 8.0) / 8.0;
  }

  juce::String s(std::round(snapped * 1000.0) / 1000.0, 3);
  while (s.endsWith("0"))
    s = s.dropLastCharacters(1);
  if (s.endsWith("."))
    s = s.dropLastCharacters(1);
  return s;
}

}  // namespace

NodeLabelOverlay::NodeLabelOverlay(NetworkController& controller)
    : _controller(controller) {
  auto typeface = juce::Typeface::createSystemTypefaceFor(
      neurythmic::assets::InterMedium_ttf,
      neurythmic::assets::InterMedium_ttfSize);
  _font = juce::Font(juce::FontOptions(typeface).withHeight(
      ConfigManager::get().fontSizeNodeInfo));
  setInterceptsMouseClicks(false, false);
}

void NodeLabelOverlay::paint(juce::Graphics& g) {
  const auto& cfg = ConfigManager::get();
  const float minDim = static_cast<float>(std::min(getWidth(), getHeight()));
  const float scaling = std::min(1.0f, minDim / 1000.0f);
  const double rootFreq =
      _controller.getNodeFrequency(NetworkState::kRootNodeId);

  g.setColour(cfg.fontNodeInfoColour);
  g.setFont(_font);
  const float labelHeight = _font.getHeight();

  for (int id : _controller.getNodeIds()) {
    auto np = _controller.getNodePosition(id);
    const float cx = np.getX() * minDim;
    const float cy = np.getY() * minDim;
    const float visualRadius =
        (id == NetworkState::kRootNodeId)
            ? cfg.nodeRadius * cfg.node0HaloSize * scaling
            : cfg.nodeRadius * scaling;

    const double freq = _controller.getNodeFrequency(id);
    const double mult = (rootFreq > 0.0) ? freq / rootFreq : 1.0;
    const juce::String freqLabel = formatFreqMultiple(mult);
    g.drawText(freqLabel,
               juce::Rectangle<float>(cx - 40.0f,
                                      cy - visualRadius - labelHeight - 6.0f,
                                      80.0f, labelHeight),
               juce::Justification::centred);

    const int barDivision = _controller.getNodeBarDivision(id);
    if (barDivision > 0) {
      g.drawText(juce::String(barDivision),
                 juce::Rectangle<float>(cx - 40.0f, cy + visualRadius + 6.0f,
                                        80.0f, labelHeight),
                 juce::Justification::centred);
    }
  }
}

}  // namespace neurythmic
