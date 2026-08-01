/*
  ==============================================================================

    MainComponent implementation

  ==============================================================================
*/

#include "MainComponent.h"
#include "NetworkInfo.h"

//==============================================================================
/**
*/

MainComponent::MainComponent(int slot) : mySlot(juce::jlimit(0, DistributedAudio::kMaxSlots - 1, slot))
{
    titleLabel.setText("Distributed Audio Processing Node", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    statusLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(statusLabel);

    netLabel.setJustificationType(juce::Justification::topLeft);
    netLabel.setFont(juce::FontOptions("Consolas", 13.0f, juce::Font::plain));
    
    netLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addChildComponent(netLabel); // not visible by default

    netToggle.onClick = [this]
    {
        const bool show = netToggle.getToggleState();

        if (show)
            updateNetLabel();

        netLabel.setVisible(show);
        resized();
    };

    netToggle.setColour(juce::ToggleButton::tickColourId, juce::Colours::aqua);
    netToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(netToggle);

    engine.prepare(sessionSampleRate, mySlot);
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
    
    control.start(DistributedAudio::controlPortForSlot(mySlot));

    setSize(480,260);
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
    netToggle.setBounds(r.removeFromTop(24).removeFromLeft(160));

    if (netLabel.isVisible())
    {
        r.removeFromTop(4);
        
        const int lineCount = juce::StringArray::fromLines(netLabel.getText()).size();
        netLabel.setBounds(r.removeFromTop(juce::jlimit(64, 240, lineCount * 17 + 6)));
    }

    r.removeFromTop(6);
    statusLabel.setBounds(r);
}

void MainComponent::updateNetLabel()
{
    const juce::String peer = engine.getAudioPeerAddress();

    netLabel.setText("This node\n  " + DistributedAudio::getLocalAddressList()
               + "\n\nAudio peer\n  " + (peer.isNotEmpty() ? peer : juce::String("(none yet)")), juce::dontSendNotification);
}

void MainComponent::timerCallback()
{
    if (netLabel.isVisible())
    {
        updateNetLabel();
        resized();
    }

    statusLabel.setText("Slot " + juce::String(mySlot + 1)
                   + "   dry in: UDP "  + juce::String(DistributedAudio::nodeAudioPortForSlot(mySlot))
                   + "   processed out: UDP " + juce::String(DistributedAudio::hostAudioPortForSlot(mySlot))
                   +  "\nControl: TCP "  + juce::String(DistributedAudio::controlPortForSlot(mySlot))
                   +  "\nPackets processed: " + juce::String(engine.getPacketsProcessed())
                   +  "\nMode: " + (host != nullptr ? "hosting " + host -> getName() : juce::String("echo (no plugin loaded)"))
                   +     (engine.isBound() ? "" : "\n** SLOT IN USE - another node owns this slot **"), juce::dontSendNotification);
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