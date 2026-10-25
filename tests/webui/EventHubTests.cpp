#include "giclee/webui/EventHub.h"

#include <catch2/catch_test_macros.hpp>

#include <vector>

using giclee::webui::EventHub;

namespace
{
    struct Received
    {
        juce::String name;
        juce::var payload;
    };

    struct Fixture
    {
        // The hub asserts it flushes on the message thread.
        juce::ScopedJuceInitialiser_GUI juceInit;
        EventHub hub { 0 };
        std::vector<Received> received;

        Fixture()
        {
            hub.setSink ([this] (const juce::Identifier& id, const juce::var& payload)
            {
                received.push_back ({ id.toString(), payload });
            });
        }
    };
}

TEST_CASE ("queued events arrive in order on flush")
{
    Fixture f;
    f.hub.emit ("a", 1);
    f.hub.emit ("b", 2);
    f.hub.emit ("a", 3);
    CHECK (f.received.empty());

    f.hub.flush();
    REQUIRE (f.received.size() == 3);
    CHECK (f.received[0].name == "a");
    CHECK (f.received[1].name == "b");
    CHECK (static_cast<int> (f.received[2].payload) == 3);

    f.hub.flush();
    CHECK (f.received.size() == 3);
}

TEST_CASE ("emitLatest keeps only the newest payload per topic")
{
    Fixture f;

    for (int i = 0; i < 100; ++i)
        f.hub.emitLatest ("meter", i);

    f.hub.emitLatest ("progress", 0.5);
    f.hub.flush();

    REQUIRE (f.received.size() == 2);
    CHECK (f.received[0].name == "meter");
    CHECK (static_cast<int> (f.received[0].payload) == 99);
    CHECK (f.received[1].name == "progress");
}

TEST_CASE ("events are dropped while no sink is attached")
{
    Fixture f;
    f.hub.emit ("before", 1);
    f.hub.setSink (nullptr);
    f.hub.emit ("during", 2);
    f.hub.emitLatest ("during.latest", 3);

    f.hub.setSink ([&] (const juce::Identifier& id, const juce::var& p) { f.received.push_back ({ id.toString(), p }); });
    f.hub.flush();

    CHECK (f.received.empty());
}
