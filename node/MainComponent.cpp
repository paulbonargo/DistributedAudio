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

    registry.loadFromFile(juce::File::getSpecialLocation(juce::File::currentExecutableFile).getSiblingFile("plugins.config"));

    control.onHello = [this](double sr, int)
    {
        sessionSampleRate = sr;
    };

    control.buildPluginList = [this] 
    {
        return buildPluginListVar();
    };
    
    control.onSelectPlugin = [this](int id) 
    {
        return selectPlugin(id);
    };
    
    control.onSetParam = [this](int index, float value) 
    {
        engine.queueParameterChange(index, value);
    };
    
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
    statusLabel.setText("Audio in: UDP " + juce::String(DistributedAudio::kNodeAudioPort) + "\nPackets processed: " + juce::String(engine.getPacketsProcessed()) + "\nMode: " + (host != nullptr ? "hosting " + host->getName() : juce::String("echo (no plugin loaded)")), juce::dontSendNotification);
}

juce::var MainComponent::buildPluginListVar()
{
    juce::Array<juce::var> arr;

    for (auto& p : registry.plugins())
    {
        juce::DynamicObject::Ptr o = new juce::DynamicObject();
        
        o -> setProperty("id", p.id); 
        o -> setProperty("name", p.name); 
        o -> setProperty("format", "VST3");

        arr.add(juce::var(o.get()));
    }
    return arr;
}

juce::var MainComponent::selectPlugin(int id)
{
    const RegisteredPlugin* entry = registry.byId(id);

    if (entry == nullptr)
    {
        juce::DynamicObject::Ptr e = new juce::DynamicObject();

        e -> setProperty("type", "ERROR"); 
        e -> setProperty("message", "unknown plugin id " + juce::String(id));
        
        return juce::var(e.get());
    }

    auto newHost = std::make_unique<PluginHost>();
    juce::String error;

    if (! newHost -> load(entry -> path, sessionSampleRate, DistributedAudio::kFramesPerPacket, error))
    {
        juce::DynamicObject::Ptr e = new juce::DynamicObject();
        
        e -> setProperty("type", "ERROR");
        e -> setProperty("message", "load failed: " + error);
        
        return juce::var(e.get());
    }

    engine.setPluginHost(nullptr);
    PluginHost* raw = newHost.get();
    host = std::move(newHost);
    engine.setPluginHost(raw);

    juce::Array<juce::var> params;
    for (int i = 0; i < host -> getNumParameters(); ++i)
    {
        juce::DynamicObject::Ptr p = new juce::DynamicObject();

        p -> setProperty("index", i);
        p -> setProperty("name", host -> getParameterName(i));
        p -> setProperty("value", (double) host -> getParameterValue(i));
        
        params.add(juce::var(p.get()));
    }

    juce::DynamicObject::Ptr sel = new juce::DynamicObject();

    sel -> setProperty("type", "PLUGIN_SELECTED");
    sel -> setProperty("name", host -> getName());
    sel -> setProperty("latencySamples", host -> getLatencySamples());
    sel -> setProperty("params", params);

    return juce::var(sel.get());
}