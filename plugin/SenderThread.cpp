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

	packetsSent.store(0, std::memory_order_relaxed);
	framesDropped.store(0, std::memory_order_relaxed);
}

void SenderThread::pushAudio(const juce::AudioBuffer<float>& buffer, int channelsToSend)
{
	const int numFrames = buffer.getNumSamples();
	const int srcChannels = juce::jmin(channelsToSend, numChannels);

	// send whole block or nothing - a full FIFO means sender is behind
	if (fifo.getFreeSpace() < numFrames)
	{
		framesDropped.fetch_add((uint64_t) numFrames, std::memory_order_relaxed);
		return;
	}

	int start1, size1, start2, size2;
	fifo.prepareToWrite(numFrames, start1, size1, start2, size2);

	auto writeRegion = [&](int ringStart, int regionSize, int srcOffset)
	{
		for (int ch = 0; ch < srcChannels; ++ch)
		{
			const float* src = ch < srcChannels ? buffer.getReadPointer(ch) + srcOffset : nullptr;
			float* dst = ringBuffer.data() + (size_t)ringStart * (size_t)numChannels + (size_t)ch;

			for (int i = 0; i < regionSize; ++i)
			{
				dst[(size_t)i * (size_t)numChannels] = (src != nullptr ? src[i] : 0.0f);
			}
		}
	};

	writeRegion(start1, size1, 0);
	writeRegion(start2, size2, size1);
	fifo.finishedWrite(size1 + size2);
}

void SenderThread::run()
{
	while (!threadShouldExit())
	{
		// send audio data to receiver complete implementation to be modified here
		// this is to demonstrate thread is running and samples are flowing through the FIFO
		DBG("FIFO frames ready: " << fifo.getNumReady() << " dropped " << (int) getFramesDropped());

		wait(500); // wait for 0.5 second before sending the next batch of audio data
	}
}