#include <gtest/gtest.h>

#include <juce_audio_processors/juce_audio_processors.h>

#include "Neurythmic/Neurythmic.h"

// ---------------------------------------------------------------------------
// Plugin version macro
// ---------------------------------------------------------------------------

TEST(VersionMacro, IsDefinedAndSemver) {
#ifdef NEURYTHMIC_VERSION
  const juce::String version{NEURYTHMIC_VERSION};
#else
  const juce::String version{};
#endif
  EXPECT_FALSE(version.isEmpty());

  auto parts = juce::StringArray::fromTokens(version, ".", "");
  ASSERT_EQ(parts.size(), 3);
  for (auto& part : parts)
    EXPECT_FALSE(part.isEmpty());
}

// ---------------------------------------------------------------------------
// State schema version
// ---------------------------------------------------------------------------

TEST(StateVersion, InitEmptyNetworkCarriesCurrentVersion) {
  auto network = neurythmic::NetworkState::initEmptyNetwork();
  EXPECT_EQ(static_cast<int>(
                network.getProperty(neurythmic::NetworkState::Props::stateVersion)),
            neurythmic::NetworkState::kStateVersion);
}

TEST(StateVersion, RebuildDefaultsLegacyTreeWithoutVersion) {
  neurythmic::PluginProcessor p;
  auto legacy = neurythmic::NetworkState::initEmptyNetwork();
  legacy.removeProperty(neurythmic::NetworkState::Props::stateVersion, nullptr);

  p.getController().rebuild(legacy);

  EXPECT_EQ(static_cast<int>(p.getController().getTree().getProperty(
                neurythmic::NetworkState::Props::stateVersion)),
            neurythmic::NetworkState::kStateVersion);
}

TEST(StateVersion, StateRoundTripPreservesVersion) {
  neurythmic::PluginProcessor a;
  a.getController().createChild(0);

  juce::MemoryBlock mb;
  a.getStateInformation(mb);

  neurythmic::PluginProcessor b;
  b.setStateInformation(mb.getData(), static_cast<int>(mb.getSize()));

  EXPECT_EQ(static_cast<int>(b.getController().getTree().getProperty(
                neurythmic::NetworkState::Props::stateVersion)),
            neurythmic::NetworkState::kStateVersion);
}
