#include "AppController.h"

#include "giclee/dsp/Peaks.h"
#include "giclee/engine/BuildInfo.h"

#include <GicleeUI.h>

#include <cmath>

namespace
{
    /** One second of a decaying, slightly detuned saw pair at 48 kHz, so the demo waveform
        has visible attack, decay and beating. Replaced by real recordings in later phases.
    */
    std::vector<float> makeDemoSignal()
    {
        constexpr double sampleRate = 48000.0;
        constexpr double f1 = 110.0, f2 = 110.6;
        std::vector<float> s (static_cast<size_t> (sampleRate));

        for (size_t i = 0; i < s.size(); ++i)
        {
            const auto t = static_cast<double> (i) / sampleRate;
            const auto saw = [t] (double f) { return 2.0 * (t * f - std::floor (0.5 + t * f)); };
            const auto env = std::min (1.0, t * 200.0) * std::exp (-4.0 * t);
            s[i] = static_cast<float> (0.45 * env * (saw (f1) + saw (f2)));
        }

        return s;
    }
}

AppController::AppController()
    : demoSignal (makeDemoSignal())
{
    resources.setStaticAssets (GicleeUI::ui_zip, static_cast<size_t> (GicleeUI::ui_zipSize));
    registerRpcMethods();
    registerResources();
    startTimer (1000);
}

AppController::~AppController()
{
    stopTimer();
}

void AppController::registerRpcMethods()
{
    using giclee::webui::RpcResult;

    rpc.add ("app.getInfo", [] (const juce::var&)
    {
        auto info = giclee::engine::toVar (giclee::engine::getBuildInfo());
        info.getDynamicObject()->setProperty ("product", "sampler");
        return RpcResult::ok (info);
    });

    rpc.add ("app.ping", [] (const juce::var& params)
    {
        auto* result = new juce::DynamicObject();
        result->setProperty ("echo", params["payload"]);
        result->setProperty ("serverTimeMs", juce::Time::currentTimeMillis());
        return RpcResult::ok (juce::var (result));
    });
}

void AppController::registerResources()
{
    using giclee::webui::Resource;

    resources.addPrefix ("/api/demo/peaks", [this] (const giclee::webui::Request& req) -> std::optional<Resource>
    {
        const auto bins = juce::jlimit (1, 1 << 16, req.query.getValue ("bins", "1024").getIntValue());

        std::vector<giclee::dsp::MinMax> peaks (static_cast<size_t> (bins));
        giclee::dsp::computeMinMaxPeaks (demoSignal, peaks);

        static_assert (sizeof (giclee::dsp::MinMax) == 2 * sizeof (float));
        return Resource::fromFloats ({ reinterpret_cast<const float*> (peaks.data()), peaks.size() * 2 });
    });
}

void AppController::timerCallback()
{
    auto* hb = new juce::DynamicObject();
    hb->setProperty ("uptimeSeconds", (juce::Time::getMillisecondCounter() - startedAtMs) / 1000.0);
    events.emitLatest ("app.heartbeat", juce::var (hb));
}
