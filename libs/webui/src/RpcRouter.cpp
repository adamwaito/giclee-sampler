#include "giclee/webui/RpcRouter.h"

namespace giclee::webui
{

RpcResult RpcResult::ok (juce::var value)
{
    RpcResult r;
    r.value = std::move (value);
    return r;
}

RpcResult RpcResult::error (juce::String code, juce::String message)
{
    jassert (code.isNotEmpty());

    RpcResult r;
    r.errorCode = code.isNotEmpty() ? std::move (code) : juce::String ("error");
    r.errorMessage = std::move (message);
    return r;
}

juce::var RpcResult::toVar() const
{
    auto* obj = new juce::DynamicObject();
    obj->setProperty ("ok", isOk());

    if (isOk())
    {
        obj->setProperty ("result", value);
    }
    else
    {
        auto* err = new juce::DynamicObject();
        err->setProperty ("code", errorCode);
        err->setProperty ("message", errorMessage);
        obj->setProperty ("error", juce::var (err));
    }

    return juce::var (obj);
}

RpcRouter::RpcRouter()
{
    add ("system.listMethods", [this] (const juce::var&)
    {
        juce::Array<juce::var> names;

        for (const auto& name : getMethods())
            names.add (name);

        return RpcResult::ok (names);
    });
}

void RpcRouter::add (const juce::String& method, SyncHandler handler)
{
    jassert (handler != nullptr);

    addAsync (method, [h = std::move (handler)] (const juce::var& params, Completion done)
    {
        done (h (params));
    });
}

void RpcRouter::addAsync (const juce::String& method, AsyncHandler handler)
{
    jassert (method.isNotEmpty() && handler != nullptr);
    handlers[method] = std::move (handler);
}

void RpcRouter::remove (const juce::String& method)
{
    handlers.erase (method);
}

bool RpcRouter::has (const juce::String& method) const
{
    return handlers.find (method) != handlers.end();
}

juce::StringArray RpcRouter::getMethods() const
{
    juce::StringArray names;

    for (const auto& [name, handler] : handlers)
        names.add (name);

    return names;
}

void RpcRouter::dispatch (const juce::String& method, const juce::var& params, Completion completion) const
{
    jassert (completion != nullptr);

    const auto it = handlers.find (method);

    if (it == handlers.end())
    {
        completion (RpcResult::error ("unknown_method", "No handler for '" + method + "'"));
        return;
    }

    // If the handler throws after already completing, don't complete a second time.
    auto completed = std::make_shared<bool> (false);
    auto once = [completion, completed] (RpcResult r)
    {
        if (std::exchange (*completed, true))
            return;

        completion (std::move (r));
    };

    try
    {
        it->second (params, once);
    }
    catch (const std::exception& e)
    {
        once (RpcResult::error ("internal", e.what()));
    }
    catch (...)
    {
        once (RpcResult::error ("internal", "Unknown exception in '" + method + "'"));
    }
}

} // namespace giclee::webui
