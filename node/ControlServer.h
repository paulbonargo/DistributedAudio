/*
  ==============================================================================

    This file contains framework code for server-side parameter plugin control

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include <functional>

//==============================================================================
/**
*/

class ControlConnection;

class ControlServer : public juce::InterprocessConnectionServer
{
    public:
        ControlServer();
        ~ControlServer() override;

        void start(int port);

        std::function<void(double sampleRate, int blockFrames)> onHello;
        std::function<juce::var()> buildPluginList;
        std::function<juce::var(int id)> onSelectPlugin;
        std::function<void(int index, float value)> onSetParam; // from 0 to 1

        juce::InterprocessConnection* createConnectionObject() override;
         
    private:
        juce::OwnedArray<ControlConnection> connections;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlServer)
};