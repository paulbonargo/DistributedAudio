
/*
  ==============================================================================

    This file contains framework code for an node audio processing engine

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>
#include "DistributedAudioPacket.h"

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
        
        uint64_t getPacketsProcessed() const noexcept { return packetsProcessed.load(std::memory_order_relaxed); }

    private:

        // default sample rate for project
        double sampleRate = 48000.0; 
        
        std::atomic<uint64_t> packetsProcessed { 0 };
        std::vector<uint8_t> rxBuffer;
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProcessingEngine)
};