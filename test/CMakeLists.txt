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
  source/main.cpp
  source/NeurythmicTest.cpp
  source/MidiOutputTest.cpp
  source/GraphGeometryTest.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../neurythmic_plugin/source/PluginProcessor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../neurythmic_plugin/source/PluginEditor.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../neurythmic_plugin/source/ConfigManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../neurythmic_plugin/source/NetworkState.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../neurythmic_plugin/source/FlashEnvelope.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../neurythmic_plugin/source/NetworkController.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../neurythmic_plugin/source/PresetManager.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../neurythmic_plugin/source/MidiOutput.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../neurythmic_plugin/source/TempoSource.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../neurythmic_plugin/source/GraphGeometry.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/../neurythmic_plugin/source/NetworkViewComponent.cpp
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
    neurythmic::neurythmic_plugin_assets
    GTest::gtest
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
