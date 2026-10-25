# Windows only: fetch the Microsoft.Web.WebView2 NuGet package at configure time so the
# WebView2 loader can be linked statically without a manual NuGet install. JUCE's
# FindWebView2.cmake searches JUCE_WEBVIEW2_PACKAGE_LOCATION for *Microsoft.Web.WebView2*.

if(NOT WIN32)
    return()
endif()

set(GICLEE_WEBVIEW2_VERSION 1.0.3485.44)
set(GICLEE_WEBVIEW2_SHA256 bc09150b179246ac90189649b13be8e6b11b3ac200e817e18df106e1f3cf489e)

set(_webview2_root "${CMAKE_BINARY_DIR}/_deps/webview2")
set(_webview2_pkg "${_webview2_root}/packages/Microsoft.Web.WebView2.${GICLEE_WEBVIEW2_VERSION}")
set(_webview2_nupkg "${_webview2_root}/download/webview2-${GICLEE_WEBVIEW2_VERSION}.nupkg")

if(NOT EXISTS "${_webview2_pkg}/build/native/include/WebView2.h")
    message(STATUS "Downloading WebView2 SDK ${GICLEE_WEBVIEW2_VERSION}")
    file(DOWNLOAD
        "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/${GICLEE_WEBVIEW2_VERSION}"
        "${_webview2_nupkg}"
        EXPECTED_HASH SHA256=${GICLEE_WEBVIEW2_SHA256}
        STATUS _webview2_status)
    list(GET _webview2_status 0 _webview2_code)

    if(NOT _webview2_code EQUAL 0)
        message(FATAL_ERROR "WebView2 download failed: ${_webview2_status}")
    endif()

    file(ARCHIVE_EXTRACT INPUT "${_webview2_nupkg}" DESTINATION "${_webview2_pkg}")
endif()

set(JUCE_WEBVIEW2_PACKAGE_LOCATION "${_webview2_root}/packages" CACHE PATH "WebView2 NuGet package folder" FORCE)
