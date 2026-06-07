/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/

class AudioSenderEditor : public juce::AudioProcessorEditor
{

public:
	explicit AudioSenderEditor(AudioSenderProcessor&);
	~AudioSenderEditor() override = default;

	void paint(juce::Graphics&) override;
	void resized() override;

private:
	AudioSenderProcessor& processorRef;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSenderEditor)

};