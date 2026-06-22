/*
  ==============================================================================

        Audio thread (single-threaded) delay line to capture and hold the 
        signal before it is sent to a processing node for outputting the basic 
        signal when packets are lost to maintain playback without artifacts 
        for [P-L, ...)

  ==============================================================================
*/

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>

//==============================================================================
/**
*/

class DryDelay
{
    public:
        void prepare(int numChannels, int capacityFrames); // capacity > L + maxBlock
        void reset();
        void write(uint64_t blockStart, const juce::AudioBuffer<float>& src, int frames);
        void read(uint64_t firstSample, float* dest, int frames) const;

private:
    int channels = 0;
    int capacity = 0; 
    std::vector<float> ring; // (interleaved) capacity * channels
    std::vector<uint64_t> stamp; // per-frame absolute position, kInvalid = unwritten
    static constexpr uint64_t kInvalid = ~0ull;
};