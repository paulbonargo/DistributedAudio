/*
  ==============================================================================

    MainComponent implementation

  ==============================================================================
*/

#include "MainComponent.h"

//==============================================================================
/**
*/

MainComponent::MainComponent()
{
    titleLabel.setText("Distributed Audio Processing Node", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel);

    engine.prepare(sessionSampleRate);
    engine.startThread();

    setSize(440,220);
    startTimerHz(4);
}

MainComponent::~MainComponent() 
{
    stopTimer();
    engine.stopThread(2000);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void MainComponent::resized()
{
    auto r = getLocalBounds().reduced(12);
    titleLabel.setBounds(r.removeFromTop(28));

    r.removeFromTop(8);
    statusLabel.setBounds(r);
}

void MainComponent::timerCallback()
{
    statusLabel.setText("Audio in: UDP" + juce::String(DistributedAudio::kNodeAudioPort) + "\nPackets processed: " + juce::String(engine.getPacketsProcessed()) + "\nMode: echo (no plugin loaded)", juce::dontSendNotification);
}
