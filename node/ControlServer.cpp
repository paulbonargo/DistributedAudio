/*
  ==============================================================================

    MainComponent implementation

  ==============================================================================
*/

#include "ControlServer.h"
#include "DistributedAudioPacket.h"

//==============================================================================
/**
*/



class ControlConnection : public juce::InterprocessConnection
{
    public:
        explicit ControlConnection(ControlServer& s) : juce::InterprocessConnection(true), server(s) {}
        ~ControlConnection() override { disconnect(); }
        
        void connectionMade() override {}
        void connectionLost() override {}
        
        void messageReceived(const juce::MemoryBlock& message) override
        {
            const juce::var msg = juce::JSON::parse(message.toString());
            if (! msg.isObject()) return;
            const juce::String type = msg["type"].toString();
            
            if (type == "HELLO")
            {
                if (server.onHello) 
                {
                    server.onHello((double) msg["sampleRate"], (int) msg["blockFrames"]);
                }

                juce::DynamicObject::Ptr ack = new juce::DynamicObject();

                ack -> setProperty("type", "HELLO_ACK");
                ack -> setProperty("nodeName", juce::SystemStats::getComputerName());
                
                sendJson(juce::var(ack.get()));
                sendPluginList();
            } else if (type == "LIST_PLUGINS") 
            { 
                sendPluginList(); 
            } else if (type == "SELECT_PLUGIN") 
            { 
                if (server.onSelectPlugin) 
                {
                    sendJson(server.onSelectPlugin((int) msg["id"])); 
                }
            } else if (type == "SET_PARAM")
            { 
                if (server.onSetParam) 
                {
                    server.onSetParam((int) msg["index"], (float) (double) msg["value"]); 
                }
            }
        }

    private:
        void sendPluginList()
        {
            juce::DynamicObject::Ptr o = new juce::DynamicObject();

            o -> setProperty("type", "PLUGIN_LIST");
            o -> setProperty("plugins", server.buildPluginList ? server.buildPluginList() : juce::var(juce::Array<juce::var>()));
            sendJson(juce::var(o.get()));
        }

        void sendJson(const juce::var& message)
        {
            const juce::String text = juce::JSON::toString(message, true);
            juce::MemoryBlock block(text.toRawUTF8(), text.getNumBytesAsUTF8());
            
            sendMessage(block);
        }

        ControlServer& server;
};

ControlServer::ControlServer() 
{

}

ControlServer::~ControlServer() 
{
    stop();
}

void ControlServer::start()
{
    if (! beginWaitingForSocket(DistributedAudio::kControlPort))
    {
        DBG("ControlServer: failed to listen on TCP " << DistributedAudio::kControlPort);
    }
}

juce::InterprocessConnection* ControlServer::createConnectionObject()
{
    auto* c = new ControlConnection(*this);
    connections.add(c);
    return c;
}