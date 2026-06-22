/*
  ==============================================================================

    DryDelay implementation

  ==============================================================================
*/

#include "DryDelay.h"
#include <algorithm>


//==============================================================================
/**
*/

void DryDelay::prepare(int numChannels, int capacityFrames)
{
    channels = juce::jmax(1, numChannels);
    capacity = juce::jmax(1, capacityFrames);

    ring.assign((size_t) capacity * (size_t) channels, 0.0f);
    stamp.assign((size_t) capacity, kInvalid);
}

void DryDelay::reset()
{
    std::fill(ring.begin(), ring.end(), 0.0f);
    std::fill(stamp.begin(), stamp.end(), kInvalid);
}

void DryDelay::write(uint64_t blockStart, const juce::AudioBuffer<float>& src, int frames)
{
    const int srcCh = src.getNumChannels();
    for (int i = 0; i < frames; ++i)
    {
        const uint64_t position = blockStart + (uint64_t) i;
        const int index = (int)(position % (uint64_t)capacity);
        float* dst = ring.data() + (size_t) index * (size_t)channels;

        for (int ch = 0; ch < channels; ++ch)
            dst[ch] = ch < srcCh ? src.getReadPointer(ch)[i] : 0.0f;

        stamp[(size_t) index] = position;
    }
}

void DryDelay::read(uint64_t firstSample, float* dest, int frames) const
{
    for (int i = 0; i < frames; ++i)
    {
        const uint64_t position = firstSample + (uint64_t) i;
        const int index = (int) (position % (uint64_t) capacity);
        float* dst = dest + (size_t) i * (size_t) channels;

        if (stamp[(size_t) index] == position)
        {
            const float* src = ring.data() + (size_t) index * (size_t) channels;
            for (int channel = 0; channel < channels; ++channel) dst[channel] = src[channel];
        }

        else
        {
            for (int channel = 0; channel < channels; ++channel)
                dst[channel] = 0.0f;
        }
    }
}