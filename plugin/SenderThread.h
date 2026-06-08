/*
  ==============================================================================

    This file contains the basic framework code for a Sender Thread

  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>

//==============================================================================
/**
*/

class SenderThread : public juce::Thread
{
    public:
        SenderThread();

        void run() override;
};