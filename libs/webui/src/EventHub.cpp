#include "giclee/webui/EventHub.h"

namespace giclee::webui
{

EventHub::EventHub (int flushHz)
{
    if (flushHz > 0)
        startTimerHz (flushHz);
}

EventHub::~EventHub()
{
    stopTimer();
}

void EventHub::setSink (Sink newSink)
{
    const juce::ScopedLock sl (lock);
    sink = std::move (newSink);

    if (sink == nullptr)
    {
        queued.clear();
        latest.clear();
    }
}

void EventHub::emit (const juce::String& event, juce::var payload)
{
    const juce::ScopedLock sl (lock);

    if (sink != nullptr)
        queued.push_back ({ event, std::move (payload) });
}

void EventHub::emitLatest (const juce::String& topic, juce::var payload)
{
    const juce::ScopedLock sl (lock);

    if (sink != nullptr)
        latest[topic] = std::move (payload);
}

void EventHub::flush()
{
    JUCE_ASSERT_MESSAGE_THREAD

    std::vector<Event> events;
    std::map<juce::String, juce::var> latestEvents;
    Sink target;

    {
        const juce::ScopedLock sl (lock);
        events.swap (queued);
        latestEvents.swap (latest);
        target = sink;
    }

    if (target == nullptr)
        return;

    for (const auto& e : events)
        target (juce::Identifier (e.name), e.payload);

    for (const auto& [topic, payload] : latestEvents)
        target (juce::Identifier (topic), payload);
}

} // namespace giclee::webui
