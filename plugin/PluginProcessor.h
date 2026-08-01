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
#include "ControlClient.h"

//==============================================================================
/**
*/

class AudioSenderProcessor : public juce::AudioProcessor, private juce::Timer
{

public:
    AudioSenderProcessor();
    ~AudioSenderProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;

    ControlClient& getControlClient() noexcept { return controlClient; }
    const ControlClient& getControlClient() const noexcept { return controlClient; }
    void setRemoteLatency(int hostPluginLatencySamples);
    void connectControl(const juce::String& host);

    void setSlot(int newSlot);
    int getSlot() const noexcept { return slot; }
    bool isReceiverBound() const noexcept { return receiverThread.isBound(); }

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

    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    void resetMetrics() noexcept;

    uint64_t getBlocksProcessed() const noexcept { return blocksProcessed.load(std::memory_order_relaxed); }
    uint64_t getPacketsLost() const noexcept { return receiverThread.getPacketsLost(); }
    uint64_t getPacketsDropped() const noexcept { return receiverThread.getFramesDropped(); }

    // % output blocks that reverted to dry audio signal
    double getUnderrunRatePercent() const noexcept
    {
        const uint64_t blocks = blocksProcessed.load(std::memory_order_relaxed);

        if (blocks == 0)
            return 0.0;

        return 100.0 * (double) underruns.load(std::memory_order_relaxed) / (double) blocks;
    }

    // % return packets missing at read deadline
    double getReturnLossPercent() const noexcept
    {
        const uint64_t received = receiverThread.getPacketsReceived();
        const uint64_t lost = receiverThread.getPacketsLost();
        const uint64_t expected = received + lost;

        if (expected == 0)
            return 0.0;

        return 100.0 * (double) lost / (double) expected;
    }

    uint64_t getUnderruns() const noexcept { return underruns.load(std::memory_order_relaxed); }
    uint64_t getPacketsSent() const noexcept { return senderThread.getPacketsSent(); }
    uint64_t getPacketsReceived() const noexcept { return receiverThread.getPacketsReceived(); }

    // user-selected base budget in samples
    void setLatencyBudget(int samples); 

    int getLatencyBudgetBase() const noexcept { return latencyBudgetBase.load(std::memory_order_relaxed); }
    int getLatencyTotal() const noexcept { return currentLatencySamples.load(std::memory_order_relaxed); }
    int getRemotePluginLatency() const noexcept { return remotePluginLatency.load(std::memory_order_relaxed); }

    const juce::String& getNodeHost() const noexcept { return lastNodeHost; }

private:
    void timerCallback() override; // temporary - logs counters until later milestone editor shows them

    static constexpr int kBaseLatencySamples = 4096;  // 4096: about 85 ms at 48 kHz
    static constexpr int kMaxLatencySamples = 16384;

    SenderThread senderThread;
    PlaybackBuffer playbackBuffer;
    ReceiverThread receiverThread{ playbackBuffer };
    DryDelay dryDelay;

    ControlClient controlClient { *this };
    double lastSampleRate = 48000.0; // default project sample value
    int lastBlockFrames = 0;
    
    juce::String lastNodeHost = "127.0.0.1";
    int slot = 0;
    int lastNumChannels = 2;

    std::vector<float> scratchInterleaved; // audio-thread scratch: maxBlock * channels, 
    uint64_t freeRunningPos = 0; // fallback timeline when host gives no playhead

    std::atomic<int> currentLatencySamples { kBaseLatencySamples };
    std::atomic<uint64_t> underruns { 0 };

    std::atomic<uint64_t> blocksProcessed { 0 };

    void applyLatency(); // republish to host: base latency + remote latency
    int publishLatency(); // store and declare to host

    std::atomic<int> latencyBudgetBase { kBaseLatencySamples }; // set by user
    std::atomic<int> remotePluginLatency { 0 };  // from node

    bool restoredFromState = false; // true when a saved session is loaded

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSenderProcessor)
};