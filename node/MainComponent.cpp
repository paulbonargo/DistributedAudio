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

    control.onHello = [this](double sr, int)
    {
        sessionSampleRate = sr;
    };

    control.buildPluginList = []() -> juce::var
    {
        juce::Array<juce::var> arr;
        juce::DynamicObject::Ptr p = new juce::DynamicObject();
        
        p -> setProperty("id", 0); p->setProperty("name", "(echo - no plugin yet)"); 
        p -> setProperty("format", "none");

        arr.add(juce::var(p.get()));
        return arr;
    };

    control.onSelectPlugin = [](int) -> juce::var
    {
        juce::DynamicObject::Ptr e = new juce::DynamicObject();
        
        e->setProperty("type", "ERROR"); 
        e->setProperty("message", "hosting not implemented yet");
        
        return juce::var(e.get());
    };

    control.onSetParam = [](int, float) {}; 
    control.start();

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
    statusLabel.setText("Audio in: UDP " + juce::String(DistributedAudio::kNodeAudioPort) + "\nPackets processed: " + juce::String(engine.getPacketsProcessed()) + "\nMode: echo (no plugin loaded)", juce::dontSendNotification);
}
