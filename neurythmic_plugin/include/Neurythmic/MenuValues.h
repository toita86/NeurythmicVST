#pragma once

#include <juce_core/juce_core.h>

#include <cmath>
#include <vector>

#include "ConfigManager.h"
#include "MatsuokaEngine.h"

/*
 * Pure value-mapping helpers shared by the Phase 5 menus. Kept free of any
 * juce::Component so the mapping logic is unit-testable headlessly (the
 * "agentic UI component" core: which button maps to which engine value).
 */

namespace neurythmic::MenuValues {

// Frequency matrix (Node tab): the configured named multiples of the root
// frequency, e.g. 1/4, 1/3, 1/2, 1, 2, 3, 4, 6, 8, 16.
inline int frequencyCount() {
  return ConfigManager::get().getNodeFreqCount();
}

inline double frequencyMultiple(int index) {
  return static_cast<double>(ConfigManager::get().getNodeFreq(index));
}

inline juce::String frequencyLabel(int index) {
  return ConfigManager::get().getNodeFreqName(index);
}

// Snap a ratio (nodeFreq / rootFreq) to the nearest configured multiple index.
inline int nearestFrequencyIndex(double ratio) {
  auto& cfg = ConfigManager::get();
  int best = 0;
  double bestDist = std::abs(static_cast<double>(cfg.getNodeFreq(0)) - ratio);
  for (int i = 1; i < cfg.getNodeFreqCount(); ++i) {
    const double dist =
        std::abs(static_cast<double>(cfg.getNodeFreq(i)) - ratio);
    if (dist < bestDist) {
      bestDist = dist;
      best = i;
    }
  }
  return best;
}

// Sync mode matrix (Node tab): NONE / 1X / LOCK.
inline MatsuNode::synchMode synchModeFromIndex(int index) {
  switch (index) {
    case 0:
      return MatsuNode::synchMode::free;
    case 1:
      return MatsuNode::synchMode::synchOnce;
    case 2:
      return MatsuNode::synchMode::synchLock;
    default:
      return MatsuNode::synchMode::free;
  }
}

inline int synchModeToIndex(MatsuNode::synchMode mode) {
  return static_cast<int>(mode);
}

// Grid type matrix (Constraint tab): OFF / 24th / 32nd.
inline MatsuokaEngine::gridType gridFromIndex(int index) {
  switch (index) {
    case 0:
      return MatsuokaEngine::gridType::unQuantised;
    case 1:
      return MatsuokaEngine::gridType::_24th;
    case 2:
      return MatsuokaEngine::gridType::_32nd;
    default:
      return MatsuokaEngine::gridType::unQuantised;
  }
}

inline int gridToIndex(MatsuokaEngine::gridType grid) {
  return static_cast<int>(grid);
}

// Resolution selector (Constraint tab): the selectable quantise multiples,
// in the legacy order {8, 4, 2, 1}.
inline std::vector<float> resolutionMultiples() {
  return {8.0f, 4.0f, 2.0f, 1.0f};
}

// The display label for each resolution button is the bar division divided by
// the multiple (24 for _24th, 32 for _32nd), matching the legacy labels.
inline std::vector<juce::String> resolutionLabels(
    MatsuokaEngine::gridType grid) {
  int barDivision = 0;
  if (grid == MatsuokaEngine::gridType::_24th)
    barDivision = 24;
  else if (grid == MatsuokaEngine::gridType::_32nd)
    barDivision = 32;
  else
    return {};

  std::vector<juce::String> labels;
  for (float mult : resolutionMultiples())
    labels.push_back(juce::String(static_cast<int>(barDivision / mult)));
  return labels;
}

}  // namespace neurythmic::MenuValues
