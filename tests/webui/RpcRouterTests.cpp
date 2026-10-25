#include "giclee/webui/RpcRouter.h"

#include <catch2/catch_test_macros.hpp>

#include <optional>
#include <stdexcept>

using giclee::webui::RpcResult;
using giclee::webui::RpcRouter;

namespace
{
    /** Dispatches and captures the (synchronous) result. */
    std::optional<RpcResult> callNow (const RpcRouter& router, const juce::String& method, const juce::var& params = {})
    {
        std::optional<RpcResult> out;
        int calls = 0;

        router.dispatch (method, params, [&] (RpcResult r)
        {
            ++calls;
            out = std::move (r);
        });

        REQUIRE (calls <= 1);
        return out;
    }
}

TEST_CASE ("sync handler result is returned")
{
    RpcRouter router;
    router.add ("math.double", [] (const juce::var& p) { return RpcResult::ok (static_cast<int> (p["x"]) * 2); });

    auto* params = new juce::DynamicObject();
    params->setProperty ("x", 21);

    const auto r = callNow (router, "math.double", juce::var (params));
    REQUIRE (r.has_value());
    CHECK (r->isOk());
    CHECK (static_cast<int> (r->getValue()) == 42);
}

TEST_CASE ("unknown methods complete with an error")
{
    RpcRouter router;
    const auto r = callNow (router, "nope");
    REQUIRE (r.has_value());
    CHECK_FALSE (r->isOk());
    CHECK (r->getErrorCode() == "unknown_method");
}

TEST_CASE ("throwing handlers complete with an internal error")
{
    RpcRouter router;
    router.add ("boom", [] (const juce::var&) -> RpcResult { throw std::runtime_error ("kaboom"); });

    const auto r = callNow (router, "boom");
    REQUIRE (r.has_value());
    CHECK (r->getErrorCode() == "internal");
    CHECK (r->getErrorMessage() == "kaboom");
}

TEST_CASE ("a handler that completes and then throws completes only once")
{
    RpcRouter router;
    router.addAsync ("twice", [] (const juce::var&, RpcRouter::Completion done)
    {
        done (RpcResult::ok (1));
        throw std::runtime_error ("late");
    });

    const auto r = callNow (router, "twice");
    REQUIRE (r.has_value());
    CHECK (r->isOk());
}

TEST_CASE ("async handlers may complete later")
{
    RpcRouter router;
    RpcRouter::Completion stored;
    router.addAsync ("later", [&] (const juce::var&, RpcRouter::Completion done) { stored = std::move (done); });

    std::optional<RpcResult> out;
    router.dispatch ("later", {}, [&] (RpcResult r) { out = std::move (r); });
    CHECK_FALSE (out.has_value());

    REQUIRE (stored != nullptr);
    stored (RpcResult::ok ("done"));
    REQUIRE (out.has_value());
    CHECK (out->getValue().toString() == "done");
}

TEST_CASE ("methods can be listed, replaced and removed")
{
    RpcRouter router;
    router.add ("a.one", [] (const juce::var&) { return RpcResult::ok (1); });
    router.add ("a.one", [] (const juce::var&) { return RpcResult::ok (2); });

    CHECK (static_cast<int> (callNow (router, "a.one")->getValue()) == 2);

    const auto listed = callNow (router, "system.listMethods")->getValue();
    REQUIRE (listed.isArray());
    CHECK (listed.getArray()->contains ("a.one"));
    CHECK (listed.getArray()->contains ("system.listMethods"));

    router.remove ("a.one");
    CHECK_FALSE (router.has ("a.one"));
}

TEST_CASE ("results serialise to the JS response shape")
{
    const auto ok = RpcResult::ok (7).toVar();
    CHECK (static_cast<bool> (ok["ok"]));
    CHECK (static_cast<int> (ok["result"]) == 7);

    const auto err = RpcResult::error ("bad_params", "x is required").toVar();
    CHECK_FALSE (static_cast<bool> (err["ok"]));
    CHECK (err["error"]["code"].toString() == "bad_params");
    CHECK (err["error"]["message"].toString() == "x is required");

    // Round-trips through JSON, as it does on its way to the WebView.
    const auto parsed = juce::JSON::parse (juce::JSON::toString (err));
    CHECK (parsed["error"]["code"].toString() == "bad_params");
}
