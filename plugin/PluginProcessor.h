/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "SenderThread.h"
#include "ReceiverThread.h"
#include "PlaybackBuffer.h"
#include "DryDelay.h"

//==============================================================================
/**
*/

class AudioSenderProcessor : public juce::AudioProcessor, private juce::Timer
{

public:
    AudioSenderProcessor();
    ~AudioSenderProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Distributed Audio Sender"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }

    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock&) override {}
    void setStateInformation(const void*, int) override {}

    uint64_t getUnderruns() const noexcept { return underruns.load(std::memory_order_relaxed); }
    uint64_t getPacketsSent() const noexcept { return senderThread.getPacketsSent(); }
    uint64_t getPacketsReceived() const noexcept { return receiverThread.getPacketsReceived(); }

private:
    void timerCallback() override; // temporary - logs counters until later milestone editor shows them

    static constexpr int kBaseLatencySamples = 4096;  // about 85 ms at 48 kHz - TODO: tune down after measuring
    static constexpr int kMaxLatencySamples = 16384;

    SenderThread senderThread;
    PlaybackBuffer playbackBuffer;
    ReceiverThread receiverThread{ playbackBuffer };
    DryDelay dryDelay;

    std::vector<float> scratchInterleaved; // audio-thread scratch: maxBlock * channels, 
    uint64_t freeRunningPos = 0; // fallback timeline when host gives no playhead

    std::atomic<int> currentLatencySamples{ kBaseLatencySamples };
    std::atomic<uint64_t> underruns{ 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSenderProcessor)
};