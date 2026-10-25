#pragma once

#include "giclee/webui/EventHub.h"
#include "giclee/webui/ResourceRouter.h"
#include "giclee/webui/RpcRouter.h"

#include <juce_gui_extra/juce_gui_extra.h>

namespace giclee::webui
{

/** The WebView both apps embed, wired to Giclee's bridge.

    - JS calls the single native function "giclee_rpc" (method, params), which is dispatched
      through the RpcRouter.
    - EventHub events are emitted to JS while this component exists.
    - Resource requests are served by the ResourceRouter (embedded frontend + /api/ binaries).
    - Debug builds load the frontend from $GICLEE_UI_DEV_URL when set (Vite dev server with
      hot reload); otherwise the embedded build is served.

    The router, resources and events must outlive this component.
*/
class GicleeWebView : public juce::Component
{
public:
    using OptionsCustomiser = std::function<juce::WebBrowserComponent::Options (juce::WebBrowserComponent::Options)>;

    struct Config
    {
        RpcRouter& rpc;
        ResourceRouter& resources;
        EventHub& events;

        /** Adds extra options, e.g. withOptionsFrom (sliderRelay) for plugin parameters. */
        OptionsCustomiser customiseOptions = nullptr;

        /** Background shown before the page paints; match the frontend's background colour. */
        juce::Colour background = juce::Colour (0xff15171a);
    };

    explicit GicleeWebView (Config config);
    ~GicleeWebView() override;

    void resized() override;
    void paint (juce::Graphics&) override;

    /** The name of the native function JS calls for RPC. */
    static constexpr const char* rpcFunctionName = "giclee_rpc";

private:
    class Browser;

    juce::WebBrowserComponent::Options makeOptions (const juce::String& devServerUrl);
    void handleRpc (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion);

    Config config;
    std::unique_ptr<Browser> browser;
    juce::Label unavailableMessage;
};

} // namespace giclee::webui
