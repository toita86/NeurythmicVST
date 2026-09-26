#include <gtest/gtest.h>

#include <juce_audio_processors/juce_audio_processors.h>

// Custom main so JUCE's event system (MessageManager) is initialised for the
// whole test run and torn down in the right order at the end of main(). This
// avoids the "MessageManager missing" timer assertion and the ShutdownDetector
// leak that otherwise fire when constructing an AudioProcessorValueTreeState
// in a headless gtest process.
int main(int argc, char** argv) {
  juce::ScopedJuceInitialiser_GUI juceInitialiser;

  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
