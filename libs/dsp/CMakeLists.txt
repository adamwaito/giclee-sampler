# giclee_dsp: pure C++20 analysis and signal-processing code. No JUCE, no UI.

add_library(giclee_dsp STATIC
    src/Level.cpp
    src/Peaks.cpp)
add_library(giclee::dsp ALIAS giclee_dsp)

target_include_directories(giclee_dsp PUBLIC include)
target_compile_features(giclee_dsp PUBLIC cxx_std_20)
target_link_libraries(giclee_dsp PRIVATE giclee::warnings)
set_target_properties(giclee_dsp PROPERTIES POSITION_INDEPENDENT_CODE ON FOLDER "libs")
