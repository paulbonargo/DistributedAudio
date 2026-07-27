/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

static inline int nextMultipleOfPacket(int x)
{
    const int fpp = DistributedAudio::kFramesPerPacket;
    return ((x + fpp - 1) / fpp) * fpp;
}


AudioSenderProcessor::AudioSenderProcessor() : juce::AudioProcessor(BusesProperties()
    .withInput("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    startTimerHz(1); // metrics log
}

void AudioSenderProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) 
{
    const int numChannels = juce::jmax(1, getTotalNumInputChannels());
    
    senderThread.stopThread(2000);
    receiverThread.stopThread(2000);

    publishLatency();

    const int playbackCapacity = nextMultipleOfPacket(kMaxLatencySamples * 2 + samplesPerBlock);
    const int dryCapacity = juce::nextPowerOfTwo(kMaxLatencySamples + samplesPerBlock * 2);

    playbackBuffer.prepare(numChannels, playbackCapacity);
    dryDelay.prepare(numChannels, dryCapacity);
    scratchInterleaved.assign((size_t)samplesPerBlock * (size_t) numChannels, 0.0f);
    freeRunningPos = 0;

    resetMetrics();

    senderThread.prepare(sampleRate, numChannels);
    receiverThread.prepare(numChannels);
    senderThread.startThread();
    receiverThread.startThread();

    lastSampleRate = sampleRate;
    lastBlockFrames = samplesPerBlock;
}

void AudioSenderProcessor::setRemoteLatency(int hostedPluginLatencySaples)
{
    remotePluginLatency.store(juce::jmax(0, hostedPluginLatencySaples), std::memory_order_relaxed);
    applyLatency();
}

void AudioSenderProcessor::setLatencyBudget(int samples)
{
    const int clamped = juce::jlimit(DistributedAudio::kFramesPerPacket, kMaxLatencySamples, samples);

    if (clamped == latencyBudgetBase.load(std::memory_order_relaxed))
        return;
    
        latencyBudgetBase.store(clamped, std::memory_order_relaxed);
        applyLatency();

        resetMetrics();
}

void AudioSenderProcessor::applyLatency()
{
    publishLatency();

    suspendProcessing(true);
    playbackBuffer.reset();
    suspendProcessing(false);

    updateHostDisplay();
}

int AudioSenderProcessor::publishLatency()
{
    const int total = juce::jlimit(DistributedAudio::kFramesPerPacket, kMaxLatencySamples, 
                                   latencyBudgetBase.load(std::memory_order_relaxed)
                                 + remotePluginLatency.load(std::memory_order_relaxed));

    currentLatencySamples.store(total, std::memory_order_relaxed);
    setLatencySamples(total);  // re-declare so the host re-aligns PDC (delay comp)

    return total;
}

void AudioSenderProcessor::connectControl(const juce::String& host)
{
    lastNodeHost = host;
    senderThread.setDestinationHost(host);
    controlClient.start(host, lastSampleRate, lastBlockFrames);
}

bool AudioSenderProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // accept mono or stereo - as long as if output type matches input type
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;

    return layouts.getMainInputChannelSet() == out;
}

void AudioSenderProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = getTotalNumInputChannels();

    // clear any input channel higher than max output channel count
    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
		buffer.clear(channel, 0, numSamples);

    // absolute timeline position of block
    uint64_t blockStart = freeRunningPos;
    freeRunningPos += (uint64_t) numSamples;

    // ship dry input to the node and retain it for underrun fallback
    senderThread.pushAudio(buffer, numChannels, blockStart);
    dryDelay.write(blockStart, buffer, numSamples);

    // output processed audio for [blockStart - L, ...) or dry on underrun
    const uint64_t L = (uint64_t)currentLatencySamples.load(std::memory_order_relaxed);
    if (blockStart < L) { buffer.clear(); return; } // pre-roll

    const uint64_t readPos = blockStart - L;
    blocksProcessed.fetch_add(1, std::memory_order_relaxed);
    float* scratch = scratchInterleaved.data();

    if (!playbackBuffer.read(readPos, scratch, numSamples))
    {
        dryDelay.read(readPos, scratch, numSamples);
        underruns.fetch_add(1, std::memory_order_relaxed);
    }

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* out = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            out[i] = scratch[(size_t) i * (size_t) numChannels + (size_t) ch];
    }
}

void AudioSenderProcessor::resetMetrics() noexcept
{
    underruns.store(0, std::memory_order_relaxed);
    blocksProcessed.store(0, std::memory_order_relaxed);

    senderThread.resetCounters();
    receiverThread.resetCounters();
}

void AudioSenderProcessor::releaseResources()
{
    senderThread.stopThread(2000);
    receiverThread.stopThread(2000);
}

void AudioSenderProcessor::timerCallback()
{
    DBG("Milestone 2 metrics: sent=" << (juce::int64)getPacketsSent() << "  received=" << (juce::int64)getPacketsReceived() << "  underruns=" << (juce::int64)getUnderruns());
}

juce::AudioProcessorEditor* AudioSenderProcessor::createEditor()
{
	return new AudioSenderEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioSenderProcessor();
}