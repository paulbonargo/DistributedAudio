/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================

AudioSenderProcessor::AudioSenderProcessor() : juce::AudioProcessor(BusesProperties()
    .withInput("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
}

void AudioSenderProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) 
{
	juce::ignoreUnused(samplesPerBlock);

    senderThread.stopThread(2000);
	senderThread.prepare(sampleRate, getTotalNumInputChannels());
    senderThread.startThread();
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

    // clear any input channel higher than max output channel count
    for (int channel = getTotalNumInputChannels(); channel < getTotalNumOutputChannels(); ++channel)
		buffer.clear(channel, 0, buffer.getNumSamples());

	senderThread.pushAudio(buffer, getTotalNumInputChannels());
}

void AudioSenderProcessor::releaseResources()
{
    senderThread.stopThread(2000);
}


juce::AudioProcessorEditor* AudioSenderProcessor::createEditor()
{
	return new AudioSenderEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new AudioSenderProcessor();
}