#pragma once

#include "giclee/webui/EventHub.h"
#include "giclee/webui/ResourceRouter.h"
#include "giclee/webui/RpcRouter.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace ParamIDs
{
    inline const juce::ParameterID gain { "gain", 1 };
}

class GicleePlayerProcessor : public juce::AudioProcessor
{
public:
    GicleePlayerProcessor();
    ~GicleePlayerProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState state;

    giclee::webui::RpcRouter& getRpc() noexcept { return rpc; }
    giclee::webui::ResourceRouter& getResources() noexcept { return resources; }
    giclee::webui::EventHub& getEvents() noexcept { return events; }

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    giclee::webui::RpcRouter rpc;
    giclee::webui::ResourceRouter resources;
    giclee::webui::EventHub events;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GicleePlayerProcessor)
};
