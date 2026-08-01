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


void ReceiverThread::prepare(int newNumChannels, int slot)
{
	jassert(!isThreadRunning()); // allocation below is not thread safe, so should only be called before starting the thread

	numChannels = juce::jmax(1, newNumChannels);
	mySlot = juce::jlimit(0, DistributedAudio::kMaxSlots - 1, slot);

	lastSequence = 0;
	haveLastSequence = false;

	rxBuffer.assign(sizeof(DistributedAudio::PacketHeader) + (size_t) DistributedAudio::kFramesPerPacket * (size_t) numChannels * sizeof(float), 0);
	
	bound.store(false, std::memory_order_relaxed);

	packetsReceived.store(0, std::memory_order_relaxed);
	packetsDropped.store(0, std::memory_order_relaxed);
	packetsLost.store(0, std::memory_order_relaxed);
}

void ReceiverThread::run()
{
	juce::DatagramSocket socket;

	const int listenPort = DistributedAudio::hostAudioPortForSlot(mySlot);

	if (!socket.bindToPort(listenPort))
	{
		DBG("ReceiverThread: failed to bind UDP " << listenPort << " (Slot in use)");
		bound.store(false, std::memory_order_relaxed);
		return;
	}
	bound.store(true, std::memory_order_relaxed);

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

		if (haveLastSequence && header.sequenceNumber > lastSequence + 1)
		{
			packetsLost.fetch_add(header.sequenceNumber - lastSequence - 1, std::memory_order_relaxed);
		}

		if (!haveLastSequence || header.sequenceNumber > lastSequence)
		{
			lastSequence = header.sequenceNumber;
			haveLastSequence = true;
		}

		const float* payload = reinterpret_cast<const float*>(rxBuffer.data() + headerSize);
		
		playback.write(header.startSample, payload, (int)header.numSamples);
		packetsReceived.fetch_add(1, std::memory_order_relaxed);
	}

}

void ReceiverThread::resetCounters() noexcept
{
	packetsReceived.store(0, std::memory_order_relaxed);
	packetsDropped.store(0, std::memory_order_relaxed);
	packetsLost.store(0, std::memory_order_relaxed);
}