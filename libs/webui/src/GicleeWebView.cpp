#include "giclee/webui/GicleeWebView.h"

namespace giclee::webui
{

namespace
{
    juce::String getDevServerUrl()
    {
       #if JUCE_DEBUG
        return juce::SystemStats::getEnvironmentVariable ("GICLEE_UI_DEV_URL", {}).trim();
       #else
        return {};
       #endif
    }

    juce::WebBrowserComponent::Resource toJuceResource (Resource r)
    {
        return { std::move (r.data), std::move (r.mimeType) };
    }
}

/** Keeps navigation inside the app: only the frontend's own origin loads in the view;
    anything else (e.g. a docs link) opens in the system browser.
*/
class GicleeWebView::Browser : public juce::WebBrowserComponent
{
public:
    Browser (const Options& options, juce::String devServerUrlIn)
        : WebBrowserComponent (options), devServerUrl (std::move (devServerUrlIn))
    {
    }

    bool pageAboutToLoad (const juce::String& newURL) override
    {
        if (isAppUrl (newURL))
            return true;

        juce::URL (newURL).launchInDefaultBrowser();
        return false;
    }

    void newWindowAttemptingToLoad (const juce::String& newURL) override
    {
        juce::URL (newURL).launchInDefaultBrowser();
    }

private:
    bool isAppUrl (const juce::String& url) const
    {
        if (url.startsWith (getResourceProviderRoot()))
            return true;

        return devServerUrl.isNotEmpty() && url.startsWith (devServerUrl);
    }

    juce::String devServerUrl;
};

GicleeWebView::GicleeWebView (Config configIn)
    : config (std::move (configIn))
{
    const auto devServerUrl = getDevServerUrl();
    const auto options = makeOptions (devServerUrl);

    if (! juce::WebBrowserComponent::areOptionsSupported (options))
    {
       #if JUCE_WINDOWS
        unavailableMessage.setText ("Giclee needs the Microsoft Edge WebView2 Runtime.\n"
                                    "Install it from https://developer.microsoft.com/microsoft-edge/webview2/ "
                                    "and restart Giclee.",
                                    juce::dontSendNotification);
       #else
        unavailableMessage.setText ("The system web view is not available.", juce::dontSendNotification);
       #endif
        unavailableMessage.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (unavailableMessage);
        return;
    }

    browser = std::make_unique<Browser> (options, devServerUrl);
    addAndMakeVisible (*browser);

    config.events.setSink ([safeThis = juce::Component::SafePointer<GicleeWebView> (this)] (const juce::Identifier& event, const juce::var& payload)
    {
        if (safeThis != nullptr && safeThis->browser != nullptr)
            safeThis->browser->emitEventIfBrowserIsVisible (event, payload);
    });

    browser->goToURL (devServerUrl.isNotEmpty() ? devServerUrl
                                                : juce::WebBrowserComponent::getResourceProviderRoot());
}

GicleeWebView::~GicleeWebView()
{
    if (browser != nullptr)
        config.events.setSink (nullptr);
}

juce::WebBrowserComponent::Options GicleeWebView::makeOptions (const juce::String& devServerUrl)
{
    using Options = juce::WebBrowserComponent::Options;

    auto options = Options{}
                       .withNativeIntegrationEnabled()
                       .withKeepPageLoadedWhenBrowserIsHidden()
                       .withNativeFunction (rpcFunctionName, [this] (const juce::Array<juce::var>& args, auto completion)
                                            {
                                                handleRpc (args, std::move (completion));
                                            })
                       .withResourceProvider ([this] (const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
                                              {
                                                  if (auto r = config.resources.serve (url))
                                                      return toJuceResource (std::move (*r));

                                                  DBG ("GicleeWebView: no resource for " << url);
                                                  return std::nullopt;
                                              },
                                              devServerUrl.isNotEmpty() ? std::optional (juce::URL (devServerUrl).getOrigin())
                                                                        : std::nullopt);

   #if JUCE_WINDOWS
    const auto userData = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                              .getChildFile ("Giclee")
                              .getChildFile ("WebView2");
    options = options.withBackend (Options::Backend::webview2)
                  .withWinWebView2Options (Options::WinWebView2{}
                                               .withUserDataFolder (userData)
                                               .withBackgroundColour (config.background)
                                               .withStatusBarDisabled());
   #endif

    if (config.customiseOptions != nullptr)
        options = config.customiseOptions (std::move (options));

    return options;
}

void GicleeWebView::handleRpc (const juce::Array<juce::var>& args,
                               juce::WebBrowserComponent::NativeFunctionCompletion completion)
{
    const auto method = args.isEmpty() ? juce::String() : args[0].toString();
    const auto params = args.size() > 1 ? args[1] : juce::var();

    DBG ("GicleeWebView: rpc " << method);

    // The completion talks to the WebView, so it must run on the message thread while the
    // view still exists. Async handlers may finish on worker threads or after the view closes.
    auto reply = [safeThis = juce::Component::SafePointer<GicleeWebView> (this),
                  respond = std::move (completion)] (RpcResult result)
    {
        auto deliver = [safeThis, respond, value = result.toVar()]
        {
            if (safeThis != nullptr && safeThis->browser != nullptr)
                respond (value);
        };

        if (juce::MessageManager::getInstance()->isThisTheMessageThread())
            deliver();
        else
            juce::MessageManager::callAsync (std::move (deliver));
    };

    config.rpc.dispatch (method, params, std::move (reply));
}

void GicleeWebView::resized()
{
    if (browser != nullptr)
        browser->setBounds (getLocalBounds());

    unavailableMessage.setBounds (getLocalBounds().reduced (24));
}

void GicleeWebView::paint (juce::Graphics& g)
{
    g.fillAll (config.background);
}

} // namespace giclee::webui
