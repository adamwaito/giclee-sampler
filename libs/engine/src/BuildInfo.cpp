#include "giclee/engine/BuildInfo.h"

namespace giclee::engine
{

BuildInfo getBuildInfo()
{
    BuildInfo info;
    info.version = GICLEE_VERSION_STRING;
    info.juceVersion = juce::SystemStats::getJUCEVersion();

   #if JUCE_MAC
    info.platform = "macOS";
   #elif JUCE_WINDOWS
    info.platform = "Windows";
   #else
    info.platform = juce::SystemStats::getOperatingSystemName();
   #endif

   #if JUCE_DEBUG
    info.buildType = "Debug";
   #else
    info.buildType = "Release";
   #endif

    return info;
}

juce::var toVar (const BuildInfo& info)
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("version", info.version);
    obj->setProperty ("juceVersion", info.juceVersion);
    obj->setProperty ("platform", info.platform);
    obj->setProperty ("buildType", info.buildType);
    return juce::var (obj);
}

} // namespace giclee::engine
