#include <gtest/gtest.h>
#include <juce_core/juce_core.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "Neurythmic/PluginProcessor.h"

TEST(NeurythmicPlugin, PluginCanBeCreated)
{
    neurythmic::PluginProcessor processor;
    EXPECT_FALSE(processor.getName().isEmpty());
}

TEST(NeurythmicPlugin, IsInstrumentType)
{
    neurythmic::PluginProcessor processor;
    EXPECT_FALSE(processor.isMidiEffect());
    EXPECT_EQ(processor.getTotalNumInputChannels(), 0);
}

TEST(NeurythmicPlugin, HasEditor)
{
    neurythmic::PluginProcessor processor;
    EXPECT_TRUE(processor.hasEditor());
}

TEST(NeurythmicPlugin, AcceptsMidi)
{
    neurythmic::PluginProcessor processor;
    EXPECT_TRUE(processor.acceptsMidi());
}

TEST(NeurythmicPlugin, ProducesMidi)
{
    neurythmic::PluginProcessor processor;
    EXPECT_TRUE(processor.producesMidi());
}

TEST(NeurythmicPlugin, RejectsInputBuses)
{
    neurythmic::PluginProcessor processor;

    juce::AudioProcessor::BusesLayout inputLayout;
    inputLayout.inputBuses.add(juce::AudioChannelSet::stereo());

    EXPECT_FALSE(processor.isBusesLayoutSupported(inputLayout));
}

TEST(NeurythmicPlugin, SupportsStereoLayout)
{
    neurythmic::PluginProcessor processor;
    
    juce::AudioProcessor::BusesLayout stereoLayout;
    stereoLayout.outputBuses.add(juce::AudioChannelSet::stereo());
    
    EXPECT_TRUE(processor.isBusesLayoutSupported(stereoLayout));
}

TEST(NeurythmicPlugin, SupportsMonoLayout)
{
    neurythmic::PluginProcessor processor;
    
    juce::AudioProcessor::BusesLayout monoLayout;
    monoLayout.outputBuses.add(juce::AudioChannelSet::mono());
    
    EXPECT_TRUE(processor.isBusesLayoutSupported(monoLayout));
}

TEST(NeurythmicPlugin, ProcessBlockDoesNotCrash)
{
    neurythmic::PluginProcessor processor;
    
    processor.prepareToPlay(44100.0, 512);
    
    juce::AudioBuffer<float> buffer(2, 512);
    juce::MidiBuffer midi;
    
    EXPECT_NO_THROW(processor.processBlock(buffer, midi));
    
    processor.releaseResources();
}

TEST(NeurythmicPlugin, GetNumPrograms)
{
    neurythmic::PluginProcessor processor;
    EXPECT_GE(processor.getNumPrograms(), 1);
}

TEST(NeurythmicPlugin, TailLengthIsNonNegative)
{
    neurythmic::PluginProcessor processor;
    EXPECT_GE(processor.getTailLengthSeconds(), 0.0);
}
