#include "AppController.h"
#include "MainWindow.h"

#include <juce_gui_basics/juce_gui_basics.h>

class GicleeSamplerApplication : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override    { return JUCE_APPLICATION_NAME_STRING; }
    const juce::String getApplicationVersion() override { return JUCE_APPLICATION_VERSION_STRING; }
    bool moreThanOneInstanceAllowed() override          { return false; }

    void initialise (const juce::String&) override
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "Giclee Sampler";
        options.folderName = "Giclee";
        options.filenameSuffix = ".settings";
        options.osxLibrarySubFolder = "Application Support";
        properties.setStorageParameters (options);

        controller = std::make_unique<AppController>();
        mainWindow = std::make_unique<MainWindow> (getApplicationName(), *controller, *properties.getUserSettings());
    }

    void shutdown() override
    {
        mainWindow = nullptr;
        controller = nullptr;
        properties.closeFiles();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

    void anotherInstanceStarted (const juce::String&) override
    {
        if (mainWindow != nullptr)
            mainWindow->toFront (true);
    }

private:
    juce::ApplicationProperties properties;
    std::unique_ptr<AppController> controller;
    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (GicleeSamplerApplication)
