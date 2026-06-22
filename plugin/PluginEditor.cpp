/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginEditor.h"

//==============================================================================

AudioSenderEditor::AudioSenderEditor(AudioSenderProcessor& p) : AudioProcessorEditor(&p), processorRef(p)
{
	// basic sender ui
	setSize(300, 150);

	// advanced ui
	/*
	setSize(600, 300);
	...
	*/
}

void AudioSenderEditor::paint(juce::Graphics& g)
{
	// basic sender ui
	g.fillAll(juce::Colours::antiquewhite);
	g.setColour(juce::Colours::darkkhaki);
	g.setFont(17.0f);
	g.drawFittedText("Distributed Audio Sender", getLocalBounds(), juce::Justification::centred, 1);

	// advanced ui
	/*
	g.fillAll(juce::Colours::antiquewhite);
	g.setColour(juce::Colours::darkkhaki);
	g.setFont(17.0f);
	g.drawFittedText("Distributed Audio Sender", getLocalBounds(), juce::Justification::centred, 1);
	...
	*/
}

void AudioSenderEditor::resized()
{
	// subcomponent layout to be added here
}