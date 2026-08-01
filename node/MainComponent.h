/*
  ==============================================================================

    This file contains framework code for a visual plugin selector

  ==============================================================================
*/

#pragma once
#include <juce_gui_extra/juce_gui_extra.h>
#include "ProcessingEngine.h"
#include "ControlServer.h"
#include "PluginRegistry.h"
#include "PluginHost.h"

//==============================================================================
/**
*/

class MainComponent : public juce::Component, private juce::Timer
{
    public:
        explicit MainComponent(int slot);
        ~MainComponent() override;

        void paint(juce::Graphics&) override;
        void resized() override;

        
    private:
        void timerCallback() override;
        void updateNetLabel();
        
        ProcessingEngine engine;
        
        ControlServer control;

        int mySlot = 0;
        
        // default sample rate for project
        double sessionSampleRate = 48000.0;
        
        juce::Label titleLabel, statusLabel;

        juce::ToggleButton netToggle { "Show IP addresses" };
        juce::Label netLabel;

        PluginRegistry registry;
        std::unique_ptr<PluginHost> host;

        juce::var buildPluginListVar();
        juce::var selectPlugin(int id);
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};