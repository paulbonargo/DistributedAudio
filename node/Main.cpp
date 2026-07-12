/*
  ==============================================================================

    Main implementation

  ==============================================================================
*/

#include <juce_gui_extra/juce_gui_extra.h>
#include "MainComponent.h"

//==============================================================================
/**
*/

class ProcessingNodeApplication : public juce::JUCEApplication
{
    public: 
        const juce::String getApplicationName() override 
        {
            return "Distributed Audio Processing Node";
        }

        const juce::String getApplicationVersion() override
        {
            return "0.3.0";
        }

        void initialise(const juce::String&) override 
        {
            mainWindow.reset(new MainWindow(getApplicationName()));
        }

        void shutdown() override
        {
            mainWindow = nullptr;
        }

        class MainWindow : public juce::DocumentWindow
        {
            public: 
                explicit MainWindow(const juce::String& name) : DocumentWindow(name, juce::Colours::black, DocumentWindow:: allButtons)
                {
                    setUsingNativeTitleBar(true);
                    setContentOwned(new MainComponent(), true);
                    setResizable(true, false);
                    centreWithSize(getWidth(), getHeight());
                    setVisible(true);
                }

                void closeButtonPressed() override
                {
                    juce::JUCEApplication::getInstance() -> systemRequestedQuit();
                }

            private: 
            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
        };
    
    private:
        std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(ProcessingNodeApplication)