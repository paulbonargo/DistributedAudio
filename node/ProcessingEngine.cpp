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

    const size_t maxBytes = sizeof(DistributedAudio::PacketHeader) + (size_t) DistributedAudio::kFramesPerPacket * 2 * sizeof(float);
    
    rxBuffer.assign(maxBytes, 0);
    txBuffer.assign(maxBytes, 0);
    
    work.setSize(2, DistributedAudio::kFramesPerPacket);
    packetsProcessed.store(0, std::memory_order_relaxed);
}

void ProcessingEngine::setPluginHost(PluginHost* host)
{
    // waits for blocks in-flight to finish
    const juce::ScopedLock sl(pluginLock); 
    pluginHost = host;
}

void ProcessingEngine::queueParameterChange(int index, float value)
{
    const juce::ScopedLock sl(paramLock);
    pendingParams.emplace_back(index, value);
}

void ProcessingEngine::drainParameterChanges(PluginHost& host)
{
    const juce::ScopedLock sl(paramLock);

    for (auto& pc : pendingParams) host.setParameterValue(pc.first, pc.second);
    pendingParams.clear();
}

void ProcessingEngine::run()
{
    juce::ScopedNoDenormals noDenormals;
    juce::DatagramSocket rx, tx;

    if (!rx.bindToPort(DistributedAudio::kNodeAudioPort))
    {
        return;
    }

    const int headerSize = (int) sizeof(DistributedAudio::PacketHeader);

    while (!threadShouldExit())
    {
        if (rx.waitUntilReady(true, 100) <= 0) continue;
        
        juce::String ip; int port = 0;
        const int bytes = rx.read(rxBuffer.data(), (int) rxBuffer.size(), false, ip, port);
        
        if (bytes < headerSize) continue;

        DistributedAudio::PacketHeader h;
        std::memcpy(&h, rxBuffer.data(), sizeof(h));

        if (h.signature != DistributedAudio::kProtocolSignature || h.version != DistributedAudio::kProtocolVersion) continue;

        const int ch = (int) h.numChannels;
        const int nf = (int) h.numSamples;

        if (bytes - headerSize != nf * ch * (int) sizeof(float) || ch < 1 || ch > 2) continue;

        const juce::ScopedLock sl(pluginLock);

        if (pluginHost != nullptr && pluginHost->instance() != nullptr)
        {
            // between blocks, apply set params
            drainParameterChanges(*pluginHost);

            const float* in = reinterpret_cast<const float*>(rxBuffer.data() + headerSize);
            work.setSize(ch, nf, false, false, true);
            
            for (int i = 0; i < nf; ++i)
                for (int c = 0; c < ch; ++c)
                    work.getWritePointer(c)[i] = in[i * ch + c];

            midi.clear();
            pluginHost->instance()->processBlock(work, midi);

            float* out = reinterpret_cast<float*>(txBuffer.data() + headerSize);
            
            for (int i = 0; i < nf; ++i)
                for (int c = 0; c < ch; ++c)
                    out[i * ch + c] = work.getReadPointer(c)[i];

            h.flags |= DistributedAudio::kFlagProcessed;
            std::memcpy(txBuffer.data(), &h, sizeof(h));
            
            tx.write(ip, DistributedAudio::kHostAudioPort, txBuffer.data(), bytes);
        } 
        else
        {   
            // echo when nothing loaded
            h.flags |= DistributedAudio::kFlagProcessed; 
            std::memcpy(rxBuffer.data(), &h, sizeof(h));
            
            tx.write(ip, DistributedAudio::kHostAudioPort, rxBuffer.data(), bytes);
        }
        packetsProcessed.fetch_add(1, std::memory_order_relaxed);
    }
}

