/*
  ==============================================================================

    SenderThread drains audio from a lock-free FIFO and transmits it
    over UDP using the specified DistributedAudio packet format.

  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>
#include "DistributedAudioPacket.h"

//==============================================================================
/**
*/

class SenderThread : public juce::Thread
{
    public:
        SenderThread();
        ~SenderThread() override;

		void prepare(double sampleRate, int numChannels);

		void pushAudio(const juce::AudioBuffer<float>& buffer, int channelsToSend);

        void run() override;

        uint64_t getPacketsSent() const noexcept { return packetsSent.load(std::memory_order_relaxed); }
        uint64_t getFramesDropped() const noexcept { return framesDropped.load(std::memory_order_relaxed); }

    private:

        // about 340 ms headroom, can adjust as needed
		static constexpr int kFifoCapacityFrames = 16384;

        static constexpr int kDestinationPort = 9000;

        // localhost for LAN testing - change to endpoint's address 
        const juce::String destinationHost = "127.0.0.1";

		juce::AbstractFifo fifo{ kFifoCapacityFrames };
        
        std::vector<float> ringBuffer; // interleaved audio data buffer for FIFO storage
        std::vector<uint8_t> packetBuffer; // header + packet payload

        uint32_t sampleRate = 0;
		int numChannels = 0;
		uint64_t sequenceNumber = 0; // run() modifies

        std::atomic<uint64_t> packetsSent{ 0 };
        std::atomic<uint64_t> framesDropped{ 0 };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SenderThread)
};