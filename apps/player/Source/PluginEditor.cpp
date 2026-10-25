#include "PluginEditor.h"

GicleePlayerEditor::GicleePlayerEditor (GicleePlayerProcessor& p)
    : AudioProcessorEditor (&p),
      webView ({ p.getRpc(),
                 p.getResources(),
                 p.getEvents(),
                 [this] (auto options) { return options.withOptionsFrom (gainRelay); } }),
      gainAttachment (*p.state.getParameter (ParamIDs::gain.getParamID()), gainRelay, nullptr)
{
    addAndMakeVisible (webView);

    setResizable (true, true);
    setResizeLimits (480, 320, 1600, 1200);
    setSize (720, 480);
}

GicleePlayerEditor::~GicleePlayerEditor() = default;

void GicleePlayerEditor::resized()
{
    webView.setBounds (getLocalBounds());
}
