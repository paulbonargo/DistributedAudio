/*
  ==============================================================================

    Main implementation

  ==============================================================================
*/

#include <juce_gui_extra/juce_gui_extra.h>
#include "MainComponent.h"
#include "DistributedAudioPacket.h"

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

        void initialise(const juce::String& commandLine) override 
        {
            // --slot=N sets the ports: 0 -> 9000/9001/9100 , 1 -> 9002/9003/9101 , etc.
            int slot = 0;

            const juce::String token = "--slot=";
            const int at = commandLine.indexOf(token);
            
            if (at >= 0)
            {
                slot = juce::jlimit(0, DistributedAudio::kMaxSlots - 1, commandLine.substring(at + token.length()).getIntValue());
            }

            mainWindow.reset(new MainWindow(getApplicationName() + "  -  Slot " + juce::String(slot + 1), slot));
        }

        void shutdown() override
        {
            mainWindow = nullptr;
        }

        class MainWindow : public juce::DocumentWindow
        {
            public: 
                MainWindow(const juce::String& name, int slot) : DocumentWindow(name, juce::Colours::black, DocumentWindow::allButtons)
                {
                    setUsingNativeTitleBar(true);
                    setContentOwned(new MainComponent(slot), true);
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