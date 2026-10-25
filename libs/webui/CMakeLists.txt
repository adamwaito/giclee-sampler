# giclee_webui: the bridge between the C++ backend and the WebView frontend.
#
# giclee_webui_core: RPC routing, event batching, resource serving. Uses juce_core/juce_events
#                    only, so it can be unit tested without a GUI.
# giclee_webui:      adds GicleeWebView, the WebBrowserComponent wrapper both apps embed.
#
# Both are INTERFACE libraries for the same reason as giclee_engine (see libs/engine).

add_library(giclee_webui_core INTERFACE)
add_library(giclee::webui_core ALIAS giclee_webui_core)

target_sources(giclee_webui_core INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/RpcRouter.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/EventHub.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ResourceRouter.cpp")

target_include_directories(giclee_webui_core INTERFACE include)
target_link_libraries(giclee_webui_core INTERFACE juce::juce_core juce::juce_events)

add_library(giclee_webui INTERFACE)
add_library(giclee::webui ALIAS giclee_webui)

target_sources(giclee_webui INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/src/GicleeWebView.cpp")

target_link_libraries(giclee_webui INTERFACE giclee::webui_core juce::juce_gui_extra)

if(WIN32)
    target_compile_definitions(giclee_webui INTERFACE JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)
endif()

# Common target settings for anything that embeds a GicleeWebView.
function(giclee_configure_webview_target target)
    target_compile_definitions(${target} PRIVATE
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0)
endfunction()
