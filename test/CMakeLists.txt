cmake_minimum_required(VERSION 3.15)

include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/cpm.cmake)

cpmaddpackage(
  NAME
    googletest
  GIT_TAG
    v1.14.0
  GITHUB_REPOSITORY
    google/googletest
  SOURCE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/googletest
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Plugin tests (compile processor directly, link only audio modules)
add_executable(NeurythmicTests
  source/NeurythmicTest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../neurythmic_plugin/source/PluginProcessor.cpp
)

target_include_directories(NeurythmicTests PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../neurythmic_plugin/include
  ${CMAKE_CURRENT_SOURCE_DIR}/../CPGLib 
)

target_link_libraries(NeurythmicTests
  PRIVATE
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_audio_devices
    MatsuokaEngine
    GTest::gtest_main
)

target_compile_definitions(NeurythmicTests PRIVATE
  NEURYTHMIC_PLUGIN_NAME="Neurythmic"
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
)

include(GoogleTest)
gtest_discover_tests(NeurythmicTests)

# CPGLib tests (standalone, no JUCE dependency)
add_executable(CPGLibTests
  source/CPGLibTest.cpp
)

target_link_libraries(CPGLibTests
  PRIVATE
    MatsuokaEngine
    GTest::gtest_main
)

gtest_discover_tests(CPGLibTests)
