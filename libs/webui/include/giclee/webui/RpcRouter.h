#pragma once

#include <juce_core/juce_core.h>

#include <functional>
#include <map>

namespace giclee::webui
{

/** Outcome of one RPC call. Serialised for JS by toVar() as
    { ok: true, result } or { ok: false, error: { code, message } }.
*/
class RpcResult
{
public:
    static RpcResult ok (juce::var value = {});
    static RpcResult error (juce::String code, juce::String message);

    bool isOk() const noexcept { return errorCode.isEmpty(); }
    const juce::var& getValue() const noexcept { return value; }
    const juce::String& getErrorCode() const noexcept { return errorCode; }
    const juce::String& getErrorMessage() const noexcept { return errorMessage; }

    juce::var toVar() const;

private:
    RpcResult() = default;

    juce::var value;
    juce::String errorCode, errorMessage;
};

/** Maps method names such as "app.getInfo" to handlers.

    The JS side reaches it through a single native function (see GicleeWebView), so handlers
    can be added at any time, unlike WebBrowserComponent native functions, which are fixed at
    construction. Dispatch happens on the message thread. Async handlers may complete on any
    thread; GicleeWebView marshals the reply back to the message thread.
*/
class RpcRouter
{
public:
    using Completion = std::function<void (RpcResult)>;
    using SyncHandler = std::function<RpcResult (const juce::var& params)>;
    using AsyncHandler = std::function<void (const juce::var& params, Completion)>;

    RpcRouter();

    /** Adds or replaces a handler. */
    void add (const juce::String& method, SyncHandler handler);
    void addAsync (const juce::String& method, AsyncHandler handler);
    void remove (const juce::String& method);

    bool has (const juce::String& method) const;
    juce::StringArray getMethods() const;

    /** Runs the handler for method. Unknown methods and handlers that throw complete with an
        error result rather than propagating. The completion is always called exactly once,
        except by async handlers that drop it.
    */
    void dispatch (const juce::String& method, const juce::var& params, Completion completion) const;

private:
    std::map<juce::String, AsyncHandler> handlers;

    JUCE_DECLARE_NON_COPYABLE (RpcRouter) // built-in handlers capture this
};

} // namespace giclee::webui
