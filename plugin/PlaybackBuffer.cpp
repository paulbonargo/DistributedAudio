/*
  ==============================================================================

    PlaybackBuffer implementation

  ==============================================================================
*/

#include "PlaybackBuffer.h"
#include <cstring>


//==============================================================================
/**
*/

void PlaybackBuffer::prepare(int numChannels, int capacityFrames)
{
    channels = juce::jmax(1, numChannels);
    numSlots = juce::jmax(1, capacityFrames / DistributedAudio::kFramesPerPacket);
    audio.assign((size_t)numSlots * (size_t)DistributedAudio::kFramesPerPacket * (size_t)channels, 0.0f);
    slotTag = std::vector<std::atomic<uint64_t>>((size_t) numSlots);
    reset();
}

void PlaybackBuffer::reset()
{
    for (auto& t : slotTag) 
        t.store(kInvalid, std::memory_order_relaxed);
}

void PlaybackBuffer::write(uint64_t startSample, const float* interleaved, int frames)
{
    const int perSlot = DistributedAudio::kFramesPerPacket * channels;
    const int slot = (int) ( (startSample / (uint64_t) DistributedAudio::kFramesPerPacket) % (uint64_t) numSlots);
    float* dst = audio.data() + (size_t) slot * (size_t) perSlot;

    const int n = juce::jmin(frames, DistributedAudio::kFramesPerPacket) * channels;
    std::memcpy(dst, interleaved, (size_t)n * sizeof(float));

    for (int i = n; i < perSlot; i++)
        dst[i] = 0.0f;
    slotTag[(size_t)slot].store(startSample, std::memory_order_release);

}

bool PlaybackBuffer::read(uint64_t firstSample, float* dest, int frames) const
{
    const int framesPerPacket = DistributedAudio::kFramesPerPacket;
    for (int i = 0; i < frames; ++i)
    {
        const uint64_t positionOfSample = firstSample + (uint64_t)i;
        const uint64_t blockStart = (positionOfSample / (uint64_t)framesPerPacket) * (uint64_t)framesPerPacket;
        const int slot = (int)(int)((positionOfSample / (uint64_t)framesPerPacket) % (uint64_t)numSlots);

        if (slotTag[(size_t)slot].load(std::memory_order_acquire) != blockStart)
            return false;

        const int frameInSlot = (int)(positionOfSample - blockStart);
        const float* src = audio.data() +
            (size_t)slot * (size_t)framesPerPacket * (size_t)channels +
            (size_t)frameInSlot * (size_t)channels;
        float* d = dest + (size_t)i * (size_t)channels;

        for (int channel = 0; channel < channels; channel++)
            d[channel] = src[channel];

    }

    return true;
}