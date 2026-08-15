#pragma once

#include <vector>

#include <juce_core/juce_core.h>
#include <juce_graphics/juce_graphics.h>

#include "NeurythmicPluginAssets.h"

namespace neurythmic {

// Global configuration loaded from an embedded settings.xml (baked in as
// binary data at build time). Singleton — access via ConfigManager::get().
//
// Every setting is exposed as a read-only `const&` bound to private storage.
// This mirrors the old MatsuGlobals pattern: callers can read values but can
// never write to them.
class ConfigManager {
public:
  static ConfigManager& get();

  ConfigManager(const ConfigManager&) = delete;
  ConfigManager& operator=(const ConfigManager&) = delete;

  //  Files & top-level flags
  const juce::String& defaultPresetFile;

  //  Node menu visibility flags
  const bool& showPhaseOffset;
  const bool& showSelfNoise;
  const bool& showSynchMode;
  const bool& showNodeMenu;

  //  Main menu
  const bool& showReloadSettings;
  const float& mainMenuOpacity;

  //  CPG behaviour: oscillator equation parameters
  const float& t1Overt2;
  const float& c;
  const float& b;
  const float& g;
  const float& freqCompensation;

  //  CPG behaviour: node/connection defaults
  const float& nodeIntensityFloor;
  const float& newNodeFreqMultiple;
  const float& newNodeVolumeInit;
  const float& newConnWeightScale;
  const float& newParentChildConnWeightScale;
  const float& nodeDistWeightScalingLimit;
  const float& nodeDistWeightScalingStart;
  const float& nodeDistWeightScalingExp;
  const bool& positionMapsToPan;
  const float& connectionWeightMax;
  const bool& connectionWeightScalingOn;
  const float& connectionWeightScalingUnity;

  // Note (open point): the connection-weight scaling curve is a
  // calibration table tied to the oscillator equation tuning (t1Overt2, c, b,
  // g). It compensates for the entrainment threshold depending on the
  // frequency ratio between connected nodes. The equation params are GLOBAL
  // config here, so the curve is kept global too,
  // parsed from the embedded scalingCurve.txt. If a future Phase 2 moves c/b/g into
  // per-preset storage (which the old matsuoka_frontend app supported), this
  // curve must move into the preset alongside them — a curve is only valid for
  // the tuning it was generated for.
  std::vector<float> getWeightScalingCurveX() const;
  std::vector<float> getWeightScalingCurveY() const;

  //  CPG layout: interaction geometry (normalised 0-1)
  const float& nodeClickableRadius;
  const float& nodeCollideDistance;
  const float& nodeSpawnDistance;

  //  CPG layout: node visuals
  const float& nodeRadius;
  const float& node0HaloSize;
  const float& nodeFreqIndicatorSize;
  const float& fontSizeNodeInfo;
  const float& fontNodeInfoAboveOffset;
  const float& fontNodeInfoBelowOffset;
  const juce::Colour& fontNodeInfoColour;
  const float& nodeInfoTriHeight;
  const float& nodeInfoTriWidth;
  const float& nodeInfoTriXOffset;
  const float& nodeInfoTriYOffset;

  //  CPG layout: connection visuals
  const float& connectionClickableWidth;
  const int& pointsInCircle;
  const int& pointsInArc;
  const float& arrowHeadSize;
  const float& arrowHeadWidthMultiplier;
  const float& arrowHeadWidthMin;
  const float& arrowHeadWidthMax;
  const float& arcAdjust;
  const float& minArcAdjustRad;
  const float& maxArcAdjustRad;
  const float& defaultCurveAmount;
  const float& minLineThickness;
  const float& maxLineThickness;
  const float& nodeLineThickness;
  const float& nodeFlashSpread;
  const float& nodeFlashExponent;
  const float& minLineWeight;
  const float& maxLineWeight;
  const float& connBrightnessParentChild;
  const float& connBrightnessOther;
  const float& inactiveBrightnessMult;
  const float& connectionColourStartsAtLevel;
  const juce::Colour& connColour;
  const juce::Colour& selectedColour;

  //  Convenience accessors
  juce::Colour getNodeColour(int i) const;
  float getNodeFreq(int i) const;
  float getNodePitch(int node) const;
  float getNodePitch(int set, int node) const;
  std::vector<float> getNodeFreqs() const;
  std::vector<juce::String> getNodeFreqNames() const;
  juce::String getNodeFreqName(int i) const;
  int getNodeFreqCount() const;

private:
  ConfigManager();
  ~ConfigManager() = default;

  //  Files & top-level flags
  juce::String _defaultPresetFile = "./matsuPresets.pre";

  //  Node menu visibility flags
  bool _showPhaseOffset = true;
  bool _showSelfNoise = true;
  bool _showSynchMode = true;
  bool _showNodeMenu = true;

  //  Main menu
  bool _showReloadSettings = true;
  float _mainMenuOpacity = 0.2f;

  //  CPG behaviour: oscillator equation parameters
  float _t1Overt2 = 4.0f;
  float _c = 1.0f;
  float _b = 7.0f;
  float _g = 7.0f;
  float _freqCompensation = 0.9732f;

  //  CPG behaviour: node/connection defaults
  float _nodeIntensityFloor = 0.7f;
  float _newNodeFreqMultiple = 2.0f;
  float _newNodeVolumeInit = 0.6f;
  float _newConnWeightScale = 0.0f;
  float _newParentChildConnWeightScale = 1.0f;
  float _nodeDistWeightScalingLimit = 0.85f;
  float _nodeDistWeightScalingStart = 0.01f;
  float _nodeDistWeightScalingExp = 2.0f;
  bool _positionMapsToPan = false;
  float _connectionWeightMax = 10.0f;
  bool _connectionWeightScalingOn = true;
  float _connectionWeightScalingUnity = 3.0f;

  //  CPG layout: interaction geometry
  float _nodeClickableRadius = 0.035f;
  float _nodeCollideDistance = 0.07f;
  float _nodeSpawnDistance = 0.12f;

  //  CPG layout: node visuals
  float _nodeRadius = 34.0f;
  float _node0HaloSize = 1.35f;
  float _nodeFreqIndicatorSize = 40.0f;
  float _fontSizeNodeInfo = 16.0f;
  float _fontNodeInfoAboveOffset = 1.9f;
  float _fontNodeInfoBelowOffset = 0.9f;
  juce::Colour _fontNodeInfoColour = juce::Colour(170, 170, 170);
  float _nodeInfoTriHeight = 6.0f;
  float _nodeInfoTriWidth = 3.0f;
  float _nodeInfoTriXOffset = 0.0f;
  float _nodeInfoTriYOffset = 0.0f;

  //  CPG layout: connection visuals
  float _connectionClickableWidth = 15.0f;
  int _pointsInCircle = 30;
  int _pointsInArc = 30;
  float _arrowHeadSize = 12.0f;
  float _arrowHeadWidthMultiplier = 2.0f;
  float _arrowHeadWidthMin = 5.0f;
  float _arrowHeadWidthMax = 13.0f;
  float _arcAdjust = 1.0f;
  float _minArcAdjustRad = 0.0f;
  float _maxArcAdjustRad = 50.0f;
  float _defaultCurveAmount = 0.01f;
  float _minLineThickness = 1.0f;
  float _maxLineThickness = 8.0f;
  float _nodeLineThickness = 2.4f;
  float _nodeFlashSpread = 40.0f;
  float _nodeFlashExponent = 8.0f;
  float _minLineWeight = 0.1f;
  float _maxLineWeight = 7.0f;
  float _connBrightnessParentChild = 1.0f;
  float _connBrightnessOther = 1.0f;
  float _inactiveBrightnessMult = 0.4f;
  float _connectionColourStartsAtLevel = 0.9f;
  juce::Colour _connColour = juce::Colour(255, 150, 20);
  juce::Colour _selectedColour = juce::Colour(0, 255, 29);

  //  Collections parsed from XML
  std::vector<juce::Colour> _nodeColours;
  std::vector<float> _nodeFrequencies;
  std::vector<std::vector<float>> _nodePitches;
  std::vector<juce::String> _nodeFrequencyNames;

  // Connection-weight scaling curve (frequency ratio -> weight multiplier),
  // parsed from the embedded scalingCurve.txt. See the note on
  // getWeightScalingCurveX() above.
  std::vector<float> _weightScalingCurveX;
  std::vector<float> _weightScalingCurveY;
};

}  // namespace neurythmic
