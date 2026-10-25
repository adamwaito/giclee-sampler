#include "MainWindow.h"

#include "giclee/webui/GicleeWebView.h"

namespace
{
    constexpr auto windowStateKey = "mainWindowState";
}

MainWindow::MainWindow (const juce::String& name, AppController& controller, juce::PropertiesFile& settingsIn)
    : DocumentWindow (name, juce::Colour (0xff15171a), DocumentWindow::allButtons),
      settings (settingsIn)
{
    setUsingNativeTitleBar (true);
    setContentOwned (new giclee::webui::GicleeWebView ({ controller.getRpc(),
                                                         controller.getResources(),
                                                         controller.getEvents() }),
                     false);

    setResizable (true, false);
    setResizeLimits (720, 480, 10000, 10000);

    if (! restoreWindowStateFromString (settings.getValue (windowStateKey)))
        centreWithSize (1200, 800);

    setVisible (true);
}

MainWindow::~MainWindow()
{
    settings.setValue (windowStateKey, getWindowStateAsString());
    clearContentComponent();
}

void MainWindow::closeButtonPressed()
{
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}
