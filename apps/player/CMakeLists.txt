set(player_formats VST3 Standalone)
set(player_options)

if(APPLE)
    list(APPEND player_formats AU)
endif()

if(WIN32)
    list(APPEND player_options NEEDS_WEBVIEW2 TRUE)
endif()

juce_add_plugin(GicleePlayer
    PRODUCT_NAME "Giclee Player"
    COMPANY_NAME "${GICLEE_COMPANY_NAME}"
    BUNDLE_ID "${GICLEE_BUNDLE_ID_PREFIX}.player"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_MANUFACTURER_CODE ${GICLEE_MANUFACTURER_CODE}
    PLUGIN_CODE GcPl
    FORMATS ${player_formats}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    VST3_CATEGORIES Instrument Sampler
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    COPY_PLUGIN_AFTER_BUILD ${GICLEE_COPY_PLUGINS}
    NEEDS_WEB_BROWSER TRUE
    ${player_options})

target_sources(GicleePlayer PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

giclee_configure_webview_target(GicleePlayer)

target_compile_definitions(GicleePlayer PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(GicleePlayer PRIVATE
    giclee::engine
    giclee::webui
    giclee_ui_player
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

set_target_properties(GicleePlayer PROPERTIES FOLDER "apps")
