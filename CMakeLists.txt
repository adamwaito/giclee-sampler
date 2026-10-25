cmake_minimum_required(VERSION 3.25)

set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")

project(Giclee VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(GICLEE_BUILD_APPS "Build the Sampler app and the Player plugin" ON)
option(GICLEE_BUILD_TESTS "Build the unit tests" ON)
option(GICLEE_BUILD_UI "Build the web frontend with npm (OFF embeds a placeholder page)" ON)
option(GICLEE_COPY_PLUGINS "Copy the Player plugin into the system plugin folders after building" OFF)

set(GICLEE_COMPANY_NAME "Adam Waito")
set(GICLEE_BUNDLE_ID_PREFIX "com.adamwaito.giclee")
set(GICLEE_MANUFACTURER_CODE "AdWa")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CPM)
include(GicleeDependencies)
include(GicleeWebView2)

add_subdirectory(libs/dsp)
add_subdirectory(libs/engine)
add_subdirectory(libs/webui)

if(GICLEE_BUILD_APPS)
    add_subdirectory(ui)
    add_subdirectory(apps/sampler)
    add_subdirectory(apps/player)
endif()

if(GICLEE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
