/*
  ==============================================================================

    ReceiverThread receives processed audio packets from a remote processing 
    node and writes them into PlaybackBuffer, referenced by startSample

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>

#include <atomic>
#include <vector>

#include "DistributedAudioPacket.h"
#include "PlaybackBuffer.h"

//==============================================================================
/**
*/

class ReceiverThread : public juce::Thread
{
    public:
        explicit ReceiverThread(PlaybackBuffer& playbackToFill);
        ~ReceiverThread() override;

        void prepare(int numChannels);
        void run() override;

        uint64_t getPacketsReceived() const noexcept { return packetsReceived.load(std::memory_order_relaxed); }
        uint64_t getFramesDropped()  const noexcept { return packetsDropped.load(std::memory_order_relaxed); }

    private:
        PlaybackBuffer& playback;
        int numChannels = 0;

        std::vector<uint8_t> rxBuffer;
        std::atomic<uint64_t> packetsReceived { 0 };
        std::atomic<uint64_t> packetsDropped { 0 };

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReceiverThread)
};