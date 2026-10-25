#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "giclee/engine/BuildInfo.h"

#include <GicleeUI.h>

GicleePlayerProcessor::GicleePlayerProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      state (*this, nullptr, "GicleePlayer", createParameterLayout())
{
    resources.setStaticAssets (GicleeUI::ui_zip, static_cast<size_t> (GicleeUI::ui_zipSize));

    rpc.add ("app.getInfo", [] (const juce::var&)
    {
        auto info = giclee::engine::toVar (giclee::engine::getBuildInfo());
        info.getDynamicObject()->setProperty ("product", "player");
        return giclee::webui::RpcResult::ok (info);
    });
}

GicleePlayerProcessor::~GicleePlayerProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout GicleePlayerProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        ParamIDs::gain, "Volume",
        juce::NormalisableRange<float> (-60.0f, 12.0f, 0.1f, 2.0f),
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    return layout;
}

void GicleePlayerProcessor::prepareToPlay (double, int) {}

void GicleePlayerProcessor::releaseResources() {}

bool GicleePlayerProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void GicleePlayerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // No sample playback yet (Phase 3).
    buffer.clear();
}

void GicleePlayerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = state.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void GicleePlayerProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (state.state.getType()))
            state.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* GicleePlayerProcessor::createEditor()
{
    return new GicleePlayerEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GicleePlayerProcessor();
}
