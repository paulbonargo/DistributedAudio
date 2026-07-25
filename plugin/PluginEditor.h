/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>
#include <vector>
#include "PluginProcessor.h"

//==============================================================================
/**
*/

class AudioSenderEditor : public juce::AudioProcessorEditor, private juce::Timer
{

public:
	explicit AudioSenderEditor(AudioSenderProcessor&);
	~AudioSenderEditor() override;

	void paint(juce::Graphics&) override;
	void resized() override;

private:
	void timerCallback() override;
	void rebuildFromControlState();
	AudioSenderProcessor& processorRef;

	juce::TextEditor hostField;
	juce::TextButton connectButton { "Connect" };
	juce::ComboBox pluginMenu;
	juce::Label statusLabel;

	struct ParamRow
	{
		std::unique_ptr<juce::Label> label;
		std::unique_ptr<juce::Slider> slider;
		int index = 0;
	};

    juce::Viewport paramViewport;
    juce::Component paramHolder;
    std::vector<ParamRow> paramRows;

    bool buildingUi = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioSenderEditor)

};