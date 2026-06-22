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

    currentLatencySamples.store(kBaseLatencySamples, std::memory_order_relaxed);
    setLatencySamples(kBaseLatencySamples);

    const int playbackCapacity = nextMultipleOfPacket(kMaxLatencySamples * 2 + samplesPerBlock);
    const int dryCapacity = juce::nextPowerOfTwo(kMaxLatencySamples + samplesPerBlock * 2);

    playbackBuffer.prepare(numChannels, playbackCapacity);
    dryDelay.prepare(numChannels, dryCapacity);
    scratchInterleaved.assign((size_t)samplesPerBlock * (size_t) numChannels, 0.0f);
    freeRunningPos = 0;

    senderThread.prepare(sampleRate, numChannels);
    receiverThread.prepare(numChannels);
    senderThread.startThread();
    receiverThread.startThread();
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

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto s = pos->getTimeInSamples())
                blockStart = (uint64_t)juce::jmax((int64_t)0, *s);

    freeRunningPos = blockStart + (uint64_t) numSamples;

    // ship dry input to the node and retain it for underrun fallback
    senderThread.pushAudio(buffer, numChannels, blockStart);
    dryDelay.write(blockStart, buffer, numSamples);

    // output processed audio for [blockStart - L, ...) or dry on underrun
    const uint64_t L = (uint64_t)currentLatencySamples.load(std::memory_order_relaxed);
    if (blockStart < L) { buffer.clear(); return; } // pre-roll

    const uint64_t readPos = blockStart - L;
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