#pragma once

#include <juce_core/juce_core.h>

namespace giclee::engine
{

struct BuildInfo
{
    juce::String version;     // Giclee version, e.g. "0.1.0"
    juce::String juceVersion; // e.g. "JUCE v9.0.4"
    juce::String platform;    // "macOS" or "Windows"
    juce::String buildType;   // "Debug" or "Release"
};

BuildInfo getBuildInfo();

/** BuildInfo as a JSON-friendly object for the UI bridge. */
juce::var toVar (const BuildInfo&);

} // namespace giclee::engine
