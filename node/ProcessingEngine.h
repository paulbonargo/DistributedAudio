
/*
  ==============================================================================

    This file contains framework code for an node audio processing engine

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <utility>
#include <vector>
#include "DistributedAudioPacket.h"
#include "PluginHost.h"

//==============================================================================
/**
*/


class ProcessingEngine : public juce::Thread
{
    public:
        ProcessingEngine();
        ~ProcessingEngine() override;

        void prepare(double sampleRate);

        void run() override;

        void setPluginHost(PluginHost* host);

        void queueParameterChange(int index, float value);
        
        uint64_t getPacketsProcessed() const noexcept { return packetsProcessed.load(std::memory_order_relaxed); }

    private:
        void drainParameterChanges(PluginHost& host);
        
        // default sample rate for project
        double sampleRate = 48000.0; 
        
        std::atomic<uint64_t> packetsProcessed { 0 };
        
        std::vector<uint8_t> rxBuffer;
        std::vector<uint8_t> txBuffer;
        
        juce::CriticalSection pluginLock;
        PluginHost* pluginHost = nullptr; // plugin lock

        juce::CriticalSection paramLock;
        std::vector<std::pair<int, float>> pendingParams; // param lock

        juce::AudioBuffer<float> work;
        juce::MidiBuffer midi;


        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessingEngine)
};