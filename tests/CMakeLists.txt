include(Catch)

# Pure DSP tests: no JUCE.
add_executable(giclee_dsp_tests
    dsp/LevelTests.cpp
    dsp/PeaksTests.cpp)

target_link_libraries(giclee_dsp_tests PRIVATE giclee::dsp giclee::warnings Catch2::Catch2WithMain)
set_target_properties(giclee_dsp_tests PROPERTIES FOLDER "tests")
catch_discover_tests(giclee_dsp_tests DISCOVERY_MODE PRE_TEST TEST_PREFIX "dsp: ")

# Bridge core tests: juce_core/juce_events, no GUI.
juce_add_console_app(giclee_webui_tests PRODUCT_NAME "giclee_webui_tests")

target_sources(giclee_webui_tests PRIVATE
    webui/RpcRouterTests.cpp
    webui/EventHubTests.cpp
    webui/ResourceRouterTests.cpp)

target_compile_definitions(giclee_webui_tests PRIVATE JUCE_USE_CURL=0 JUCE_WEB_BROWSER=0)

target_link_libraries(giclee_webui_tests PRIVATE
    giclee::webui_core
    Catch2::Catch2WithMain
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

set_target_properties(giclee_webui_tests PROPERTIES FOLDER "tests")
catch_discover_tests(giclee_webui_tests DISCOVERY_MODE PRE_TEST TEST_PREFIX "webui: ")
