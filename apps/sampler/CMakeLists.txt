set(sampler_options)

if(WIN32)
    list(APPEND sampler_options NEEDS_WEBVIEW2 TRUE)
endif()

juce_add_gui_app(GicleeSampler
    PRODUCT_NAME "Giclee Sampler"
    COMPANY_NAME "${GICLEE_COMPANY_NAME}"
    BUNDLE_ID "${GICLEE_BUNDLE_ID_PREFIX}.sampler"
    VERSION "${PROJECT_VERSION}"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Giclee Sampler records your instruments through the audio input you choose."
    NEEDS_WEB_BROWSER TRUE
    ${sampler_options})

target_sources(GicleeSampler PRIVATE
    Source/Main.cpp
    Source/MainWindow.cpp
    Source/AppController.cpp)

giclee_configure_webview_target(GicleeSampler)

target_compile_definitions(GicleeSampler PRIVATE
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:GicleeSampler,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:GicleeSampler,JUCE_VERSION>")

target_link_libraries(GicleeSampler PRIVATE
    giclee::engine
    giclee::webui
    giclee_ui_sampler
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

set_target_properties(GicleeSampler PROPERTIES FOLDER "apps")
