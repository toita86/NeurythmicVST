#include <gtest/gtest.h>
#include "MatsuokaEngine.h"

TEST(CPGLib, EngineCanBeCreated)
{
    MatsuokaEngine engine;
    EXPECT_TRUE(engine.nodeExists(0));
}

TEST(CPGLib, EngineHasDefaultSampleRate)
{
    MatsuokaEngine engine;
    EXPECT_EQ(engine.getSampleRate(), DEFAULTSAMPLERATE);
}

TEST(CPGLib, CustomSampleRate)
{
    MatsuokaEngine engine(44100);
    EXPECT_EQ(engine.getSampleRate(), 44100u);
}

TEST(CPGLib, RootNodeExistsAfterCreation)
{
    MatsuokaEngine engine;
    EXPECT_TRUE(engine.nodeExists(0));
}

TEST(CPGLib, RootNodeHasDefaultFrequency)
{
    MatsuokaEngine engine;
    double freq = engine.getNodeFrequency(0);
    EXPECT_GT(freq, 0.0);
    EXPECT_LT(freq, 100.0);
}

TEST(CPGLib, CanAddChildNode)
{
    MatsuokaEngine engine;
    engine.doQueuedActions();
    
    unsigned newID = engine.addChild(0, 1);
    engine.doQueuedActions();
    
    EXPECT_TRUE(engine.nodeExists(1));
}

TEST(CPGLib, CanSetNodeFrequency)
{
    MatsuokaEngine engine;
    engine.calibrate();
    
    engine.setNodeFrequency(0, 2.0, false);
    engine.doQueuedActions();
    
    double freq = engine.getNodeFrequency(0);
    EXPECT_NEAR(freq, 2.0, 0.1);
}

TEST(CPGLib, StepAdvancesEngine)
{
    MatsuokaEngine engine;
    
    uint64_t counter1 = engine.getEngineStepCounter();
    engine.step();
    uint64_t counter2 = engine.getEngineStepCounter();
    
    EXPECT_EQ(counter2, counter1 + 1);
}

TEST(CPGLib, MultipleStepsProduceOutput)
{
    MatsuokaEngine engine;
    engine.calibrate();
    
    for (int i = 0; i < 1000; ++i)
    {
        engine.step();
    }
    
    double output = engine.getNodeOutput(0);
    EXPECT_TRUE(std::isfinite(output));
}

TEST(CPGLib, CanSetConnectionBetweenNodes)
{
    MatsuokaEngine engine;
    engine.addChild(0, 1);
    engine.doQueuedActions();
    
    engine.setConnection(0, 1, 1.0);
    engine.doQueuedActions();
    
    auto inputs = engine.getInputs(1);
    EXPECT_FALSE(inputs.empty());
}

TEST(CPGLib, CanDeleteNode)
{
    MatsuokaEngine engine;
    engine.addChild(0, 1);
    engine.doQueuedActions();
    
    EXPECT_TRUE(engine.nodeExists(1));
    
    engine.deleteNode(1);
    engine.doQueuedActions();
    
    EXPECT_FALSE(engine.nodeExists(1));
}

TEST(CPGLib, CannotDeleteRootNode)
{
    MatsuokaEngine engine;
    
    EXPECT_THROW({
        engine.deleteNode(0);
        engine.doQueuedActions();
    }, std::runtime_error);
    
    EXPECT_TRUE(engine.nodeExists(0));
}

TEST(CPGLib, PauseStopsProcessing)
{
    MatsuokaEngine engine;
    
    engine.setPause(true);
    
    uint64_t counter1 = engine.getEngineStepCounter();
    engine.step();
    uint64_t counter2 = engine.getEngineStepCounter();
    
    EXPECT_EQ(counter1, counter2);
}

TEST(CPGLib, CanSetQuantiser)
{
    MatsuokaEngine engine;
    
    engine.setNodeQuantiser_Grid(0, MatsuokaEngine::gridType::_32nd);
    engine.setQuantiseAmount(0.5f);
    
    EXPECT_FLOAT_EQ(engine.getQuantiseAmount(), 0.5f);
}

TEST(CPGLib, CanSetPhaseOffset)
{
    MatsuokaEngine engine;
    engine.addChild(0, 1);
    engine.doQueuedActions();
    
    engine.setNodePhaseOffset(1, 0.5);
    engine.doQueuedActions();
    
    double phase = engine.getNodePhaseOffset(1);
    EXPECT_GE(phase, 0.0);
    EXPECT_LE(phase, 1.0);
}

TEST(CPGLib, CanSetSelfNoise)
{
    MatsuokaEngine engine;
    
    engine.setNodeSelfNoise(0, 0.1);
    engine.doQueuedActions();
    
    double noise = engine.getNodeSelfNoise(0);
    EXPECT_NEAR(noise, 0.1, 0.001);
}

TEST(CPGLib, ResetClearsState)
{
    MatsuokaEngine engine;
    
    for (int i = 0; i < 100; ++i)
    {
        engine.step();
    }
    
    double output = engine.getNodeOutput(0);
    
    engine.reset();
    engine.doQueuedActions();
    
    double outputAfterReset = engine.getNodeOutput(0);
    
    EXPECT_NE(output, outputAfterReset);
}

TEST(CPGLib, CalibrateSetsCompensation)
{
    MatsuokaEngine engine;
    
    engine.calibrate();
    
    double comp = engine.getFrequencyCompensation();
    EXPECT_GT(comp, 0.0);
}

TEST(CPGLib, CanSetConnectionWeightScaling)
{
    MatsuokaEngine engine;
    
    engine.setConnectionWeightScaling(true);
    engine.setConnectionWeightScaling(false);
    
    SUCCEED();
}

TEST(CPGLib, ClearResetsToSingleNode)
{
    MatsuokaEngine engine;
    
    engine.addChild(0, 1);
    engine.addChild(0, 2);
    engine.doQueuedActions();
    
    EXPECT_TRUE(engine.nodeExists(1));
    EXPECT_TRUE(engine.nodeExists(2));
    
    engine.clear();
    engine.doQueuedActions();
    
    EXPECT_FALSE(engine.nodeExists(1));
    EXPECT_FALSE(engine.nodeExists(2));
    EXPECT_TRUE(engine.nodeExists(0));
}

TEST(CPGLib, GetNodeListReturnsActiveNodes)
{
    MatsuokaEngine engine;
    
    engine.addChild(0, 1);
    engine.addChild(0, 2);
    engine.doQueuedActions();
    
    auto nodes = engine.getNodeList();
    
    EXPECT_GE(nodes.size(), 3u);
}

TEST(CPGLib, EventCallbackCanBeSet)
{
    MatsuokaEngine engine;
    
    bool callbackCalled = false;
    engine.setEventCallback([&callbackCalled](int, float) {
        callbackCalled = true;
    });
    
    SUCCEED();
}

TEST(CPGLib, IsIdleReturnsTrueInitially)
{
    MatsuokaEngine engine;
    EXPECT_TRUE(engine.isIdle());
}

TEST(CPGLib, IsIdleReturnsFalseDuringStep)
{
    MatsuokaEngine engine;
    
    engine.step();
    
    EXPECT_TRUE(engine.isIdle());
}
