# giclee_engine: audio/MIDI devices, plugin hosting, file IO and playback. Uses JUCE, no GUI code.
#
# This is an INTERFACE library: its sources compile into each final target along with the
# JUCE modules it uses. JUCE modules are themselves source libraries, so compiling them into
# a shared static library as well would produce duplicate symbols.

add_library(giclee_engine INTERFACE)
add_library(giclee::engine ALIAS giclee_engine)

target_sources(giclee_engine INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/BuildInfo.cpp")

target_include_directories(giclee_engine INTERFACE include)

target_compile_definitions(giclee_engine INTERFACE
    GICLEE_VERSION_STRING="${PROJECT_VERSION}")

target_link_libraries(giclee_engine INTERFACE
    giclee::dsp
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors)
