/*
  ==============================================================================

    This file contains the basic framework code for a Sender Thread

  ==============================================================================
*/

#include "SenderThread.h"

//==============================================================================
/**
*/

SenderThread::SenderThread() : Thread("Sender Thread")
{

}

void SenderThread::run()
{
	while (!threadShouldExit())
	{
		DBG("SenderThread is running...");

		// send audio data to receiver implementation to be added here

		wait(1000); // wait for 1 second before sending the next batch of audio data
	}
}