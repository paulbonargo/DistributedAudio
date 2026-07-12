
/*
  ==============================================================================

    This file contains framework code for a visual plugin selector

  ==============================================================================
*/

#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "ProcessingEngine.h"

//==============================================================================
/**
*/

class MainComponent : public juce::Component, private juce::Timer
{
    public:
        MainComponent();
        ~MainComponent() override;

        void paint(juce::Graphics&) override;
        void resized() override;

    private:
        void timerCallback() override;
        ProcessingEngine engine;

        // default sample rate for project
        double sessionSampleRate = 48000.0;

        juce::Label titleLabel, statusLabel;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};