/*
  ==============================================================================

    PlaybackBuffer offers position-indexed store for processed audio returned 
    from the node. Producer: ReceiverThread, Consumer: audio thread 
    [firstSample, firstSample + frames]

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

#include "PlaybackBuffer.h"
#include <DistributedAudioPacket.h>

#include <atomic>
#include <vector>

//==============================================================================
/**
*/

class PlaybackBuffer
{
public:
    void prepare(int numChannels, int capacityFrames);
    void reset();
    void write(uint64_t numChannels, const float* interleaved, int frames);
    bool read(uint64_t firstSample, float* dest, int frames) const;

private:
    int channels = 0;
    int numSlots = 0;
    std::vector<float> audio;
    std::vector<std::atomic<uint64_t>> slotTag;
    static constexpr uint32_t  kInvalid = ~0ull;
};