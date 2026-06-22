/*
  ==============================================================================

    ReceiverThread implementation

  ==============================================================================
*/

#include "ReceiverThread.h"
#include <cstring>

//==============================================================================
/**
*/

ReceiverThread::ReceiverThread(PlaybackBuffer& playbackToFill):juce::Thread("DistAudio Receiver"), playback(playbackToFill) 
{

}

ReceiverThread::~ReceiverThread()
{
	stopThread(2000);
}


void ReceiverThread::prepare(int newNumChannels)
{
	jassert(!isThreadRunning()); // allocation below is not thread safe, so should only be called before starting the thread

	numChannels = juce::jmax(1, newNumChannels);

	rxBuffer.assign(sizeof(DistributedAudio::PacketHeader) + (size_t) DistributedAudio::kFramesPerPacket * (size_t) numChannels * sizeof(float), 0);
	
	packetsReceived.store(0, std::memory_order_relaxed);
	packetsDropped.store(0, std::memory_order_relaxed);
}

void ReceiverThread::run()
{
	juce::DatagramSocket socket;

	if (!socket.bindToPort(DistributedAudio::kHostAudioPort))
	{
		DBG("ReceiverThread: failed to bind UDP " << DistributedAudio::kHostAudioPort);
		return;
	}

	const int headerSize = (int) sizeof(DistributedAudio::PacketHeader);

	// receive audio data from sender
	while (!threadShouldExit())
	{
		// 100ms timeout for exit
		if (socket.waitUntilReady(true, 100) <= 0) 
			continue; 

		const int bytes = socket.read(rxBuffer.data(), (int)rxBuffer.size(), false);

		if (bytes < headerSize)
		{
			packetsDropped.fetch_add(1, std::memory_order_relaxed); 
			continue;
		}

		DistributedAudio::PacketHeader header;
		std::memcpy(&header, rxBuffer.data(), sizeof(header));

		const int payloadBytes = bytes - headerSize;
		const int expected = (int)header.numSamples * (int)header.numChannels * (int) sizeof(float);

		if (header.signature != DistributedAudio::kProtocolSignature
			|| header.version != DistributedAudio::kProtocolVersion
			|| (header.flags & DistributedAudio::kFlagProcessed) == 0
			|| (int)header.numChannels != numChannels
			|| payloadBytes != expected)
		{
			packetsDropped.fetch_add(1, std::memory_order_relaxed);
			continue;
		}

		const float* payload = reinterpret_cast<const float*>(rxBuffer.data() + headerSize);
		
		playback.write(header.startSample, payload, (int)header.numSamples);
		packetsReceived.fetch_add(1, std::memory_order_relaxed);
	}
}