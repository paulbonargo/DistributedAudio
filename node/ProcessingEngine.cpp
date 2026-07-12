/*
  ==============================================================================

    ProcessingEngine implementation

  ==============================================================================
*/

#include "ProcessingEngine.h"
#include <cstring>

//==============================================================================
/**
*/

ProcessingEngine::ProcessingEngine() : juce::Thread("DistributedAudio Engine")
{

}

ProcessingEngine::~ProcessingEngine() 
{
    stopThread(2000);
}

void ProcessingEngine::prepare(double newSampleRate)
{
    jassert(!isThreadRunning());
    sampleRate = newSampleRate;

    rxBuffer.assign(sizeof(DistributedAudio::PacketHeader) * (size_t) DistributedAudio::kFramesPerPacket + 2 * sizeof(float), 0);
    packetsProcessed.store(0, std::memory_order_relaxed);
}

void ProcessingEngine::run()
{
    juce::DatagramSocket rx, tx;

    if (!rx.bindToPort(DistributedAudio::kNodeAudioPort))
    {
        DBG("bind fail");
        return ;
    }

    const int headerSize = (int) sizeof(DistributedAudio::PacketHeader);

    while (!threadShouldExit())
    {
        if (rx.waitUntilReady(true, 100) <= 0) continue;
        
        juce::String senderIp;
        int senderPort = 0;
        
        const int bytes = rx.read(rxBuffer.data(), (int) rxBuffer.size(), false, senderIp, senderPort);
        if (bytes < headerSize) continue;

        DistributedAudio::PacketHeader header;
        std::memcpy(&header, rxBuffer.data(), sizeof(header));

        if (header.signature != DistributedAudio::kProtocolSignature || header.version != DistributedAudio::kProtocolVersion) continue;

        // echo
        header.flags |= DistributedAudio::kFlagProcessed;
        
        std::memcpy(rxBuffer.data(), &header, sizeof(header));
        tx.write(senderIp, DistributedAudio::kHostAudioPort, rxBuffer.data(), bytes);

        packetsProcessed.fetch_add(1, std::memory_order_relaxed);
    }
}

