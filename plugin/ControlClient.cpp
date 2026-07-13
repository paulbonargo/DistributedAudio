/*
  ==============================================================================

    ControlClient implementation

  ==============================================================================
*/

#include "ControlClient.h"
#include "PluginProcessor.h"
#include "DistributedAudioPacket.h"

//==============================================================================
/**
*/

ControlClient::ControlClient(AudioSenderProcessor& owner) : juce::InterprocessConnection(true), processor(owner)
{

}

ControlClient::~ControlClient()
{
    stop();
}

void ControlClient::start(const juce::String& host, double sampleRate, int blockFrames)
{
    nodeHost = host;
    hostSampleRate = sampleRate;
    hostBlockFrames = blockFrames;

    running.store(true, std::memory_order_relaxed);
    
    startTimer(1000);
    timerCallback();
}

void ControlClient::stop()
{
    running.store(false, std::memory_order_relaxed);

    stopTimer();
    disconnect();
    nodeConnected.store(false, std::memory_order_relaxed);
}

void ControlClient::timerCallback()
{
    if (running.load(std::memory_order_relaxed) && !isConnected())
    {
        connectToSocket(nodeHost, DistributedAudio::kControlPort, 500);
    }
}

void ControlClient::connectionMade()
{
    nodeConnected.store(true, std::memory_order_relaxed);
    juce::DynamicObject::Ptr hello = new juce::DynamicObject();

    hello -> setProperty("type", "HELLO");
    hello -> setProperty("protoVer", (int) DistributedAudio::kProtocolVersion);
    hello -> setProperty("sampleRate", hostSampleRate);
    hello -> setProperty("blockFrames", hostBlockFrames);
    hello -> setProperty("audioReturnPort", DistributedAudio::kHostAudioPort);
    
    sendJson(juce::var(hello.get()));
    notifyUi();
}

void ControlClient::connectionLost()
{
    nodeConnected.store(false, std::memory_order_relaxed);
    notifyUi();
}

void ControlClient::sendJson(const juce::var& message)
{
    const juce::String text = juce::JSON::toString(message, true);
    juce::MemoryBlock block(text.toRawUTF8(), text.getNumBytesAsUTF8());

    sendMessage(block);
}

void ControlClient::messageReceived(const juce::MemoryBlock& message)
{
    const juce::var msg = juce::JSON::parse(message.toString());
    if (!msg.isObject()) 
    {
        return;
    }
    
    const juce::String type = msg["type"].toString();

    if (type == "PLUGIN_LIST")
        const juce::ScopedLock sl(stateLock);
        pluginList.clear();

        if (auto* arr = msg["plugins"].getArray())
        {
            for (auto& p : *arr)
            {
                pluginList.push_back({ (int) p["id"], p["name"].toString(), p["format"].toString() });
            }
        } 
    else if (type == "PLUGIN_SELECTED")
    {
        {
            const juce::ScopedLock sl(stateLock);
            selectedName = msg["name"].toString();
            params.clear();
            if (auto* arr = msg["params"].getArray())
            {
                for (auto& p : *arr)
                {
                    params.push_back({ (int) p["index"], p["name"].toString(), (float) (double) p["value"] });
                }
            }
        }
        processor.setRemoteLatency((int) msg["latancySamples"]);
    }
    else if (type == "ERROR")
    {
        DBG("ControlClient ERROR: " << msg["message"].toString());
    }

    notifyUi();
}

void ControlClient::selectPlugin(int id)
{
    juce::DynamicObject::Ptr o = new juce::DynamicObject();

    o -> setProperty("type", "SELECT_PLUGIN");
    o -> setProperty("id", id);
    
    sendJson(juce::var(o.get()));
}

void ControlClient::setParameter(int index, float value)
{
    juce::DynamicObject::Ptr o = new juce::DynamicObject();

    o -> setProperty("type", "SET_PARAM");
    o -> setProperty("index", index);
    o -> setProperty("value", (double) value); // saves as double, conversion to float handled in node

    sendJson(juce::var(o.get()));
}

std::vector<RemotePluginInfo> ControlClient::getPluginList() const
{
    const juce::ScopedLock sl(stateLock);
    return pluginList;
}

std::vector<RemoteParamInfo> ControlClient::getParameters() const
{
    const juce::ScopedLock sl(stateLock);
    return params;
}

juce::String ControlClient::getSelectedPluginName() const
{
    const juce::ScopedLock sl(stateLock);
    return selectedName;
}

void ControlClient::notifyUi()
{
    if(onStateChanged)
    {
        juce::MessageManager::callAsync([cb = onStateChanged]
        {
            cb();
        });
    }
}