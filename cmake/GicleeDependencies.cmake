# Third-party dependencies, pinned to exact versions and verified by hash.
# Set CPM_SOURCE_CACHE (env or cache var) to share downloads between build trees.

CPMAddPackage(
    NAME JUCE
    VERSION 9.0.4
    URL https://github.com/juce-framework/JUCE/archive/refs/tags/9.0.4.tar.gz
    URL_HASH SHA256=e2bee25d5c776118aea550ad168a3dc9520c0669a79bd9f5a72b9f404ee1c40a
)

if(GICLEE_BUILD_TESTS)
    CPMAddPackage(
        NAME Catch2
        VERSION 3.16.1
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.16.1.tar.gz
        URL_HASH SHA256=d16bd1b8b2364918bd472cb216394acb93bde6784890df0d91bccfb3f28d777d
    )
    list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
endif()

# Warning flags for Giclee's own non-JUCE targets.
add_library(giclee_warnings INTERFACE)
add_library(giclee::warnings ALIAS giclee_warnings)

if(MSVC)
    target_compile_options(giclee_warnings INTERFACE /W4 /permissive- /utf-8)
else()
    target_compile_options(giclee_warnings INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion
        -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual)
endif()
