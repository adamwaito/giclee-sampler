#pragma once

#include <juce_events/juce_events.h>

#include <functional>
#include <map>
#include <vector>

namespace giclee::webui
{

/** Collects backend → UI events and delivers them in batches on the message thread.

    - emit() queues every event in order (e.g. "sample finished").
    - emitLatest() keeps only the newest payload per topic between flushes (e.g. meters,
      progress), so fast producers can't flood the WebView.

    Both may be called from any thread except the audio thread (they take a lock and allocate).
    While no sink is attached (e.g. a plugin editor is closed), events are dropped; the UI asks
    for current state over RPC when it loads.
*/
class EventHub : private juce::Timer
{
public:
    using Sink = std::function<void (const juce::Identifier& event, const juce::var& payload)>;

    /** flushHz = 0 disables the timer; call flush() yourself (used by tests). */
    explicit EventHub (int flushHz = 30);
    ~EventHub() override;

    void setSink (Sink newSink);

    void emit (const juce::String& event, juce::var payload = {});
    void emitLatest (const juce::String& topic, juce::var payload);

    /** Delivers everything pending to the sink. Message thread only. */
    void flush();

private:
    void timerCallback() override { flush(); }

    struct Event
    {
        juce::String name;
        juce::var payload;
    };

    juce::CriticalSection lock;
    Sink sink;
    std::vector<Event> queued;
    std::map<juce::String, juce::var> latest;
};

} // namespace giclee::webui
