#include "../include/Neurythmic/ConfigManager.h"

namespace neurythmic {

namespace {

// Safe typed accessors: read the text of a child element by name, falling
// back to a default when the parent or child is missing. The old
// ofxXmlSettings code could return a default; these helpers avoid the
// nullptr dereference risk of `getChildByName(...)->...` when a tag is absent.

float getFloat(const juce::XmlElement* parent, const char* name, float fallback) {
  if (parent == nullptr) return fallback;
  if (auto* child = parent->getChildByName(name))
    return child->getAllSubText().getFloatValue();
  return fallback;
}

int getInt(const juce::XmlElement* parent, const char* name, int fallback) {
  if (parent == nullptr) return fallback;
  if (auto* child = parent->getChildByName(name))
    return child->getAllSubText().getIntValue();
  return fallback;
}

bool getBool(const juce::XmlElement* parent, const char* name, bool fallback) {
  return getInt(parent, name, fallback ? 1 : 0) != 0;
}

juce::String getString(const juce::XmlElement* parent, const char* name,
                       const juce::String& fallback) {
  if (parent == nullptr) return fallback;
  if (auto* child = parent->getChildByName(name))
    return child->getAllSubText();
  return fallback;
}

// Reads an <r>/<g>/<b> child triple from `parent`.
juce::Colour getColour(const juce::XmlElement* parent) {
  if (parent == nullptr) return juce::Colours::white;
  auto r = static_cast<juce::uint8>(getInt(parent, "r", 255));
  auto g = static_cast<juce::uint8>(getInt(parent, "g", 255));
  auto b = static_cast<juce::uint8>(getInt(parent, "b", 255));
  return juce::Colour(r, g, b);
}

}  // namespace

// PUBLIC
ConfigManager& ConfigManager::get() {
  static ConfigManager instance;  // thread-safe in C++11+
  return instance;
}

// PRIVATE
ConfigManager::ConfigManager()
    : defaultPresetFile(_defaultPresetFile),
      showPhaseOffset(_showPhaseOffset),
      showSelfNoise(_showSelfNoise),
      showSynchMode(_showSynchMode),
      showNodeMenu(_showNodeMenu),
      showReloadSettings(_showReloadSettings),
      mainMenuOpacity(_mainMenuOpacity),
      t1Overt2(_t1Overt2),
      c(_c),
      b(_b),
      g(_g),
      freqCompensation(_freqCompensation),
      nodeIntensityFloor(_nodeIntensityFloor),
      newNodeFreqMultiple(_newNodeFreqMultiple),
      newNodeVolumeInit(_newNodeVolumeInit),
      newConnWeightScale(_newConnWeightScale),
      newParentChildConnWeightScale(_newParentChildConnWeightScale),
      nodeDistWeightScalingLimit(_nodeDistWeightScalingLimit),
      nodeDistWeightScalingStart(_nodeDistWeightScalingStart),
      nodeDistWeightScalingExp(_nodeDistWeightScalingExp),
      positionMapsToPan(_positionMapsToPan),
      connectionWeightMax(_connectionWeightMax),
      connectionWeightScalingOn(_connectionWeightScalingOn),
      connectionWeightScalingUnity(_connectionWeightScalingUnity),
      nodeClickableRadius(_nodeClickableRadius),
      nodeCollideDistance(_nodeCollideDistance),
      nodeSpawnDistance(_nodeSpawnDistance),
      nodeRadius(_nodeRadius),
      node0HaloSize(_node0HaloSize),
      nodeFreqIndicatorSize(_nodeFreqIndicatorSize),
      fontSizeNodeInfo(_fontSizeNodeInfo),
      fontNodeInfoAboveOffset(_fontNodeInfoAboveOffset),
      fontNodeInfoBelowOffset(_fontNodeInfoBelowOffset),
      fontNodeInfoColour(_fontNodeInfoColour),
      nodeInfoTriHeight(_nodeInfoTriHeight),
      nodeInfoTriWidth(_nodeInfoTriWidth),
      nodeInfoTriXOffset(_nodeInfoTriXOffset),
      nodeInfoTriYOffset(_nodeInfoTriYOffset),
      connectionClickableWidth(_connectionClickableWidth),
      pointsInCircle(_pointsInCircle),
      pointsInArc(_pointsInArc),
      arrowHeadSize(_arrowHeadSize),
      arrowHeadWidthMultiplier(_arrowHeadWidthMultiplier),
      arrowHeadWidthMin(_arrowHeadWidthMin),
      arrowHeadWidthMax(_arrowHeadWidthMax),
      arcAdjust(_arcAdjust),
      minArcAdjustRad(_minArcAdjustRad),
      maxArcAdjustRad(_maxArcAdjustRad),
      defaultCurveAmount(_defaultCurveAmount),
      minLineThickness(_minLineThickness),
      maxLineThickness(_maxLineThickness),
      nodeLineThickness(_nodeLineThickness),
      nodeFlashSpread(_nodeFlashSpread),
      nodeFlashExponent(_nodeFlashExponent),
      minLineWeight(_minLineWeight),
      maxLineWeight(_maxLineWeight),
      connBrightnessParentChild(_connBrightnessParentChild),
      connBrightnessOther(_connBrightnessOther),
      inactiveBrightnessMult(_inactiveBrightnessMult),
      connectionColourStartsAtLevel(_connectionColourStartsAtLevel),
      connColour(_connColour),
      selectedColour(_selectedColour) {
  // Load the settings from embedded binary data (no filesystem dependency).
  juce::String xmlContent(neurythmic::assets::settings_xml,
                          neurythmic::assets::settings_xmlSize);
  auto xml = juce::XmlDocument::parse(xmlContent);
  auto* root = xml.get();
  if (root == nullptr)
    return;  // parse failure — keep all defaults

  // Top-level scalar settings.
  _defaultPresetFile = getString(root, "defaultPresetFile", _defaultPresetFile);

  // nodeFrequencies: alternating <name>/<value> pairs.
  if (auto* nf = root->getChildByName("nodeFrequencies")) {
    _nodeFrequencies.clear();
    _nodeFrequencyNames.clear();
    for (auto* child = nf->getFirstChildElement(); child != nullptr;
         child = child->getNextElement()) {
      if (child->hasTagName("name"))
        _nodeFrequencyNames.push_back(child->getAllSubText());
      else if (child->hasTagName("value"))
        _nodeFrequencies.push_back(child->getAllSubText().getFloatValue());
    }
  }

  // nodePitches: <set> of <pitch> values.
  if (auto* np = root->getChildByName("nodePitches")) {
    _nodePitches.clear();
    for (auto* set = np->getFirstChildElement(); set != nullptr;
         set = set->getNextElement()) {
      if (!set->hasTagName("set")) continue;
      std::vector<float> thisSet;
      for (auto* pitch = set->getFirstChildElement(); pitch != nullptr;
           pitch = pitch->getNextElement()) {
        if (pitch->hasTagName("pitch"))
          thisSet.push_back(pitch->getAllSubText().getFloatValue());
      }
      _nodePitches.push_back(std::move(thisSet));
    }
  }

  // nodeMenu.
  if (auto* nodeMenu = root->getChildByName("nodeMenu")) {
    _showPhaseOffset = getBool(nodeMenu, "showPhaseOffset", _showPhaseOffset);
    _showSelfNoise = getBool(nodeMenu, "showSelfNoise", _showSelfNoise);
    _showSynchMode = getBool(nodeMenu, "showSynchMode", _showSynchMode);
    _showNodeMenu = getBool(nodeMenu, "showNodeMenu", _showNodeMenu);
  }

  // mainMenu.
  if (auto* mainMenu = root->getChildByName("mainMenu")) {
    _showReloadSettings =
        getBool(mainMenu, "showReloadSettings", _showReloadSettings);
    _mainMenuOpacity = getFloat(mainMenu, "mainMenuOpacity", _mainMenuOpacity);
  }

  // CPG_behaviour.
  if (auto* behaviour = root->getChildByName("CPG_behaviour")) {
    _t1Overt2 = getFloat(behaviour, "t1Overt2", _t1Overt2);
    _c = getFloat(behaviour, "c", _c);
    _b = getFloat(behaviour, "b", _b);
    _g = getFloat(behaviour, "g", _g);
    _freqCompensation =
        getFloat(behaviour, "freqCompensation", _freqCompensation);
    _nodeIntensityFloor =
        getFloat(behaviour, "nodeIntensityFloor", _nodeIntensityFloor);
    _newNodeFreqMultiple =
        getFloat(behaviour, "newNodeFreqMultiple", _newNodeFreqMultiple);
    _newNodeVolumeInit =
        getFloat(behaviour, "newNodeVolumeInit", _newNodeVolumeInit);
    _newConnWeightScale =
        getFloat(behaviour, "newConnWeightScale", _newConnWeightScale);
    _newParentChildConnWeightScale = getFloat(
        behaviour, "newParentChildConnWeightScale", _newParentChildConnWeightScale);
    _nodeDistWeightScalingLimit = getFloat(
        behaviour, "nodeDistWeightScalingLimit", _nodeDistWeightScalingLimit);
    _nodeDistWeightScalingStart = getFloat(
        behaviour, "nodeDistWeightScalingStart", _nodeDistWeightScalingStart);
    _nodeDistWeightScalingExp =
        getFloat(behaviour, "nodeDistWeightScalingExp", _nodeDistWeightScalingExp);
    _positionMapsToPan =
        getBool(behaviour, "positionMapsToPan", _positionMapsToPan);
    _connectionWeightMax =
        getFloat(behaviour, "connectionWeightMax", _connectionWeightMax);
    _connectionWeightScalingOn = getBool(
        behaviour, "connectionWeightScalingOn", _connectionWeightScalingOn);
    _connectionWeightScalingUnity = getFloat(
        behaviour, "connectionWeightScalingUnity", _connectionWeightScalingUnity);
  }

  // CPG_layout.
  if (auto* layout = root->getChildByName("CPG_layout")) {
    _nodeClickableRadius =
        getFloat(layout, "nodeClickableRadius", _nodeClickableRadius);
    _nodeCollideDistance =
        getFloat(layout, "nodeCollideDistance", _nodeCollideDistance);
    _nodeSpawnDistance =
        getFloat(layout, "defaultNodeSpawnDistance", _nodeSpawnDistance);
    _connectionClickableWidth =
        getFloat(layout, "connectionClickableWidth", _connectionClickableWidth);
    _nodeRadius = getFloat(layout, "defaultNodeRadius", _nodeRadius);
    _node0HaloSize = getFloat(layout, "node0HaloSize", _node0HaloSize);
    _nodeFreqIndicatorSize =
        getFloat(layout, "nodeFreqIndicatorSize", _nodeFreqIndicatorSize);
    _pointsInCircle = getInt(layout, "pointsInCircle", _pointsInCircle);
    _pointsInArc = getInt(layout, "pointsInArc", _pointsInArc);
    _arrowHeadSize = getFloat(layout, "arrowHeadSize", _arrowHeadSize);
    _arrowHeadWidthMultiplier = getFloat(
        layout, "arrowHeadWidthMultiplier", _arrowHeadWidthMultiplier);
    _arrowHeadWidthMin =
        getFloat(layout, "arrowHeadWidthMin", _arrowHeadWidthMin);
    _arrowHeadWidthMax =
        getFloat(layout, "arrowHeadWidthMax", _arrowHeadWidthMax);
    _arcAdjust = getFloat(layout, "arcAdjust", _arcAdjust);
    _minArcAdjustRad =
        getFloat(layout, "minArcAdjustRad", _minArcAdjustRad);
    _maxArcAdjustRad =
        getFloat(layout, "maxArcAdjustRad", _maxArcAdjustRad);
    _defaultCurveAmount =
        getFloat(layout, "defaultCurveAmount", _defaultCurveAmount);
    _minLineThickness =
        getFloat(layout, "minLineThickness", _minLineThickness);
    _maxLineThickness =
        getFloat(layout, "maxLineThickness", _maxLineThickness);
    _nodeLineThickness =
        getFloat(layout, "nodeLineThickness", _nodeLineThickness);
    _nodeFlashSpread = getFloat(layout, "nodeFlashSpread", _nodeFlashSpread);
    _nodeFlashExponent =
        getFloat(layout, "nodeFlashExponent", _nodeFlashExponent);
    _minLineWeight = getFloat(layout, "minLineWeight", _minLineWeight);
    _maxLineWeight = getFloat(layout, "maxLineWeight", _maxLineWeight);
    _connBrightnessParentChild = getFloat(
        layout, "ConnBrightnessParentChild", _connBrightnessParentChild);
    _connBrightnessOther =
        getFloat(layout, "ConnBrightnessOther", _connBrightnessOther);
    _inactiveBrightnessMult =
        getFloat(layout, "inactiveBrightnessMult", _inactiveBrightnessMult);
    _connectionColourStartsAtLevel = getFloat(
        layout, "lineBrightnessScalingBegins", _connectionColourStartsAtLevel);

    _nodeInfoTriWidth =
        getFloat(layout, "nodeInfoTriWidth", _nodeInfoTriWidth);
    _nodeInfoTriHeight =
        getFloat(layout, "nodeInfoTriHeight", _nodeInfoTriHeight);
    _nodeInfoTriXOffset =
        getFloat(layout, "nodeInfoTriXOffset", _nodeInfoTriXOffset);
    _nodeInfoTriYOffset =
        getFloat(layout, "nodeInfoTriYOffset", _nodeInfoTriYOffset);
    _fontSizeNodeInfo =
        getFloat(layout, "fontSizeNodeInfo", _fontSizeNodeInfo);
    _fontNodeInfoAboveOffset = getFloat(
        layout, "fontNodeInfoAboveOffset", _fontNodeInfoAboveOffset);
    _fontNodeInfoBelowOffset = getFloat(
        layout, "fontNodeInfoBelowOffset", _fontNodeInfoBelowOffset);
    // Colours stored as <r>/<g>/<b> child triples.
    _fontNodeInfoColour = getColour(layout->getChildByName("fontNodeInfoColour"));
    _connColour = juce::Colour(
        static_cast<juce::uint8>(getInt(layout, "ConnColour_R", 255)),
        static_cast<juce::uint8>(getInt(layout, "ConnColour_G", 150)),
        static_cast<juce::uint8>(getInt(layout, "ConnColour_B", 20)));
    _selectedColour = juce::Colour(
        static_cast<juce::uint8>(getInt(layout, "SelectedColour_R", 0)),
        static_cast<juce::uint8>(getInt(layout, "SelectedColour_G", 255)),
        static_cast<juce::uint8>(getInt(layout, "SelectedColour_B", 29)));

    // NodeColours: iterate <nColour> children.
    _nodeColours.clear();
    if (auto* colours = layout->getChildByName("NodeColours")) {
      for (auto* child = colours->getFirstChildElement(); child != nullptr;
           child = child->getNextElement()) {
        if (child->hasTagName("nColour")) _nodeColours.push_back(getColour(child));
      }
    }
  }

  // Connection-weight scaling curve: embedded scalingCurve.txt parsed into
  // parallel x/y vectors (frequency ratio -> weight multiplier). Loaded into
  // the engine via loadConnectionWeightCurve(x, y) during plugin setup.
  _weightScalingCurveX.clear();
  _weightScalingCurveY.clear();
  {
    juce::String curveContent(neurythmic::assets::scalingCurve_txt,
                              neurythmic::assets::scalingCurve_txtSize);
    juce::StringArray lines;
    lines.addLines(curveContent);
    for (const auto& line : lines) {
      juce::StringArray tokens;
      tokens.addTokens(line, false);
      if (tokens.size() < 2) continue;
      _weightScalingCurveX.push_back(tokens[0].getFloatValue());
      _weightScalingCurveY.push_back(tokens[1].getFloatValue());
    }
  }
}

// getters

juce::Colour ConfigManager::getNodeColour(int i) const {
  if (_nodeColours.empty()) return juce::Colours::white;
  if (i < 0) i = 0;
  i = i % static_cast<int>(_nodeColours.size());
  return _nodeColours[static_cast<size_t>(i)];
}

float ConfigManager::getNodeFreq(int i) const {
  if (i < static_cast<int>(_nodeFrequencies.size()))
    return _nodeFrequencies[static_cast<size_t>(i)];
  return 1.0f;
}

float ConfigManager::getNodePitch(int node) const {
  return getNodePitch(0, node);
}

float ConfigManager::getNodePitch(int set, int node) const {
  if (_nodePitches.empty()) return 440.0f;
  if (set >= static_cast<int>(_nodePitches.size()))
    set = static_cast<int>(_nodePitches.size()) - 1;
  if (set < 0) set = 0;
  if (node < 0) node = 0;
  const auto& thisSet = _nodePitches[static_cast<size_t>(set)];
  if (thisSet.empty()) return 440.0f;
  node = node % static_cast<int>(thisSet.size());
  return thisSet[static_cast<size_t>(node)];
}

std::vector<float> ConfigManager::getNodeFreqs() const {
  return _nodeFrequencies;
}

std::vector<juce::String> ConfigManager::getNodeFreqNames() const {
  return _nodeFrequencyNames;
}

juce::String ConfigManager::getNodeFreqName(int i) const {
  if (i < static_cast<int>(_nodeFrequencyNames.size()))
    return _nodeFrequencyNames[static_cast<size_t>(i)];
  return "1";
}

int ConfigManager::getNodeFreqCount() const {
  return static_cast<int>(_nodeFrequencies.size());
}

std::vector<float> ConfigManager::getWeightScalingCurveX() const {
  return _weightScalingCurveX;
}

std::vector<float> ConfigManager::getWeightScalingCurveY() const {
  return _weightScalingCurveY;
}

}  // namespace neurythmic
