#pragma once

#include "PluginProcessor.h"

#include "giclee/webui/GicleeWebView.h"

class GicleePlayerEditor : public juce::AudioProcessorEditor
{
public:
    explicit GicleePlayerEditor (GicleePlayerProcessor&);
    ~GicleePlayerEditor() override;

    void resized() override;

private:
    // Relays must be constructed before the web view, which registers them.
    juce::WebSliderRelay gainRelay { ParamIDs::gain.getParamID() };

    giclee::webui::GicleeWebView webView;

    juce::WebSliderParameterAttachment gainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GicleePlayerEditor)
};
