// The ordering rule: Source #includes go AFTER headers. PluginEditor.cpp uses
// PluginProcessor.h, so PluginProcessor.h must already be included, which it
// is, via Neurythmic.h → Neurythmic.h already includes it. As long as
// source/PluginEditor.cpp comes after the header includes, it works.

#include "neurythmic_plugin.h"
#include "source/PluginProcessor.cpp"
#include "source/PluginEditor.cpp"
#include "source/ConfigManager.cpp"
#include "source/NetworkState.cpp"
#include "source/FlashEnvelope.cpp"
#include "source/NetworkController.cpp"
#include "source/PresetManager.cpp"
#include "source/MidiOutput.cpp"
#include "source/TempoSource.cpp"
#include "source/GraphGeometry.cpp"
#include "source/NetworkViewComponent.cpp"
