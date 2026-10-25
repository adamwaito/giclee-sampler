#pragma once

#include "giclee/webui/EventHub.h"
#include "giclee/webui/ResourceRouter.h"
#include "giclee/webui/RpcRouter.h"

#include <juce_events/juce_events.h>

#include <vector>

/** Owns the Sampler's backend state and exposes it to the UI over the bridge.
    Lives for the whole application; the main window's web view attaches to it.
*/
class AppController : private juce::Timer
{
public:
    AppController();
    ~AppController() override;

    giclee::webui::RpcRouter& getRpc() noexcept { return rpc; }
    giclee::webui::ResourceRouter& getResources() noexcept { return resources; }
    giclee::webui::EventHub& getEvents() noexcept { return events; }

private:
    void registerRpcMethods();
    void registerResources();
    void timerCallback() override;

    giclee::webui::RpcRouter rpc;
    giclee::webui::ResourceRouter resources;
    giclee::webui::EventHub events;

    std::vector<float> demoSignal;
    const juce::uint32 startedAtMs = juce::Time::getMillisecondCounter();

    JUCE_DECLARE_NON_COPYABLE (AppController)
};
