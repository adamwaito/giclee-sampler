#pragma once

#include "AppController.h"

#include <juce_gui_basics/juce_gui_basics.h>

class MainWindow : public juce::DocumentWindow
{
public:
    MainWindow (const juce::String& name, AppController& controller, juce::PropertiesFile& settings);
    ~MainWindow() override;

    void closeButtonPressed() override;

private:
    juce::PropertiesFile& settings;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};
