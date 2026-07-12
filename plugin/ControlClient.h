/*
  ==============================================================================

    This file contains framework code for TCP-based parameter plugin control

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

#include <atomic>
#include <functional>
#include <vector>

//==============================================================================
/**
*/

class AudioSenderProcessor;

struct RemotePluginInfo
{
    int id = 0;
    juce::String name;
    juce::String format;
};

struct RemoteParamInfo
{
    int index = 0;
    juce::String name; 
    float value = 0.0f; // value - normalized from 0 to 1
};

class ControlClient : public juce::InterprocessConnection, private juce::Timer
{
    public:
        explicit ControlClient(AudioSenderProcessor& owner);
        ~ControlClient() override;

        void start(const juce::String& host, double sampleRate, int blockFrames);
        void stop();
        void selectPlugin(int id);
        void setParameter(int index, float value); // from 0 to 1, SET_PARAM : fire and forget

        bool isConnectedToNode() const noexcept 
        {
            return nodeConnected.load(std::memory_order_relaxed);
        }

        std::vector<RemotePluginInfo> getPluginList() const;
        std::vector<RemoteParamInfo> getParameters() const;

        juce::String getSelectedPluginName() const;

        std::function<void()> onStateChanged; // invoke on message thread

        void connectionMade() override;
        void connectionLost() override;
        
        void messageReceived(const juce::MemoryBlock& message) override;

    private:
        void timerCallback() override;
        void sendJson(const juce::var& message);
        void notifyUi();

        AudioSenderProcessor& processor;
        juce::String nodeHost;
        
        // default project sample rate
        double hostSampleRate = 48000.0;
        int hostBlockFrames = 0;

        std::atomic<bool> nodeConnected
        {
            false
        };
        std::atomic<bool> running
        {
            false
        };
        
        mutable juce::CriticalSection stateLock;

        std::vector<RemotePluginInfo> pluginList;
        std::vector<RemoteParamInfo> params;

        juce::String selectedName;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ControlClient)
};