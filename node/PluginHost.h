/*
  ==============================================================================

    This file loads and owns a single VST3 instance of a plugin and runs its processBlock

  ==============================================================================
*/

#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

//==============================================================================
/**
*/

class PluginHost
{
    public:
        PluginHost();
        bool load(const juce::File& vst3Path, double sampleRate, int blockSize, juce::String& error);

        juce::AudioPluginInstance* instance() noexcept 
        {
            return plugin.get();
        }

        juce::String getName() const;
        int getLatencySamples() const;
        
        int getNumParameters() const;
        juce::String getParameterName(int index) const;
        float getParameterValue(int index) const; // from 0 to 1
        void setParameterValue(int index, float value);
        
    private:
        juce::AudioPluginFormatManager formatManager;
        std::unique_ptr<juce::AudioPluginInstance> plugin;
};