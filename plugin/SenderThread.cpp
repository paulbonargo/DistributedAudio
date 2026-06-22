/*
  ==============================================================================

    SenderThread implementation

  ==============================================================================
*/

#include "SenderThread.h"
#include <cstring>

//==============================================================================
/**
*/

SenderThread::SenderThread() : Thread("Distributed Audio Sender Thread")
{

}

SenderThread::~SenderThread()
{
	stopThread(2000);
}

void SenderThread::prepare(double newSampleRate, int newNumChannels)
{
	jassert(!isThreadRunning()); // allocation below is not thread safe, so should only be called before starting the thread

	sampleRate = static_cast<uint32_t>(newSampleRate);
	numChannels = juce::jmax(1,newNumChannels);

	ringBuffer.assign((size_t) kFifoCapacityFrames * (size_t) numChannels, 0.0f);

	packetBuffer.assign(sizeof(DistributedAudio::PacketHeader) + 
		(size_t)DistributedAudio::kFramesPerPacket * (size_t) numChannels * sizeof(float), 0);

	fifo.reset();
	sequenceNumber = 0;
	framesSentTotal = 0;

	baseSample.store(0, std::memory_order_relaxed);
	haveBase.store(false, std::memory_order_release);

	packetsSent.store(0, std::memory_order_relaxed);
	framesDropped.store(0, std::memory_order_relaxed);
}

void SenderThread::pushAudio(const juce::AudioBuffer<float>& buffer, int channelsToSend, uint64_t blockStartSample)
{
	const int numFrames = buffer.getNumSamples();
	const int srcChannels = juce::jmin(channelsToSend, numChannels);

	// send whole block or nothing - a full FIFO means sender is behind
	if (fifo.getFreeSpace() < numFrames)
	{
		framesDropped.fetch_add((uint64_t) numFrames, std::memory_order_relaxed);
		return;
	}

	if (!haveBase.load(std::memory_order_acquire))
	{
		baseSample.store(blockStartSample, std::memory_order_relaxed);
		haveBase.store(true, std::memory_order_release);
	}

	int start1, size1, start2, size2;
	fifo.prepareToWrite(numFrames, start1, size1, start2, size2);

	auto writeRegion = [&] (int ringStart, int regionSize, int srcOffset)
	{
		for (int channel = 0; channel < numChannels; ++channel)
		{
			const float* src = channel < srcChannels ? buffer.getReadPointer(channel) + srcOffset : nullptr;
			float* dst = ringBuffer.data() + (size_t) ringStart * (size_t) numChannels + (size_t) channel;

			for (int i = 0; i < regionSize; ++i)
			{
				dst[(size_t) i * (size_t) numChannels] = (src != nullptr ? src[i] : 0.0f);
			}
		}
	};

	writeRegion(start1, size1, 0);
	writeRegion(start2, size2, size1);
	fifo.finishedWrite(size1 + size2);
}

void SenderThread::run()
{
	juce::DatagramSocket socket;

	const size_t headerSize = sizeof(DistributedAudio::PacketHeader);

	// send audio data to receiver
	while (!threadShouldExit())
	{
		if (fifo.getNumReady() < DistributedAudio::kFramesPerPacket)
		{
			// 128 frames arrive approx every 2.7ms at 48kHz sample rate, so a 1ms poll adds negligible latency while keeping CPU usage low when idle
			wait(1); 
			continue;
		}

		int size1, size2, start1, start2;
		fifo.prepareToRead(DistributedAudio::kFramesPerPacket, start1, size1, start2, size2);
	
		const uint32_t framesThisPacket = (uint32_t) (size1 + size2);

		DistributedAudio::PacketHeader header {};

		header.signature = DistributedAudio::kProtocolSignature;
		header.version = DistributedAudio::kProtocolVersion;

		header.numChannels = (uint16_t) numChannels;

		header.sampleRate = sampleRate;
		header.numSamples =  framesThisPacket;

		header.sequenceNumber = sequenceNumber;
		header.startSample = baseSample.load(std::memory_order_relaxed) + framesSentTotal;
		header.flags = 0;

		std::memcpy(packetBuffer.data(), &header, headerSize);

		uint8_t* payload = packetBuffer.data() + headerSize;

		const size_t bytes1 = (size_t) size1 * (size_t) numChannels * sizeof(float);
		const size_t bytes2 = (size_t) size2 * (size_t) numChannels * sizeof(float);

		std::memcpy(payload, ringBuffer.data() + (size_t) start1 * (size_t) numChannels, bytes1);
		std::memcpy(payload + bytes1, ringBuffer.data() + (size_t) start2 * (size_t) numChannels, bytes2);

		fifo.finishedRead((int)framesThisPacket);
		const int packetBytes = (int)(headerSize + bytes1 + bytes2);
		
		if (socket.write(destinationHost, DistributedAudio::kNodeAudioPort, packetBuffer.data(), packetBytes) == packetBytes)
		{
			++sequenceNumber;
			framesSentTotal += framesThisPacket;
			packetsSent.fetch_add(1, std::memory_order_relaxed);
		}
	}
}