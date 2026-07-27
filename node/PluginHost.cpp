/*
  ==============================================================================

    PluginHost implementation

  ==============================================================================
*/

#include "PluginHost.h"

//==============================================================================
/**
*/

PluginHost::PluginHost() 
{
    juce::addDefaultFormatsToManager(formatManager);
}

bool PluginHost::load(const juce::File& vst3Path, double sampleRate, int blockSize, juce::String& error)
{
    juce::VST3PluginFormat format;
    juce::OwnedArray<juce::PluginDescription> found;
    format.findAllTypesForFile(found, vst3Path.getFullPathName());
    
    if (found.isEmpty())
    {
        error = "no VST3 found at " + vst3Path.getFullPathName();
        return false;
    }

    juce::String err;
    auto inst = formatManager.createPluginInstance(*found[0], sampleRate, blockSize, err);
    
    if (inst == nullptr)
    {
        error = err;
        return false;
    }

    inst -> enableAllBuses();

    juce::AudioProcessor::BusesLayout stereoLayout;
    stereoLayout.inputBuses.add(juce::AudioChannelSet::stereo());
    stereoLayout.outputBuses.add(juce::AudioChannelSet::stereo());

    if (! inst -> setBusesLayout(stereoLayout))
    {
        inst -> setChannelLayoutOfBus(true,  0, juce::AudioChannelSet::stereo());
        inst -> setChannelLayoutOfBus(false, 0, juce::AudioChannelSet::stereo());
    }

    inst -> setRateAndBufferSizeDetails(sampleRate, blockSize);
    inst -> prepareToPlay(sampleRate, blockSize);

    plugin = std::move(inst);

    DBG("PluginHost: " << plugin -> getName() << "   main in=" << plugin -> getMainBusNumInputChannels() << "  out=" << plugin -> getMainBusNumOutputChannels());

    return true;
}

juce::String PluginHost::getName() const 
{
    return plugin != nullptr ? plugin->getName() : juce::String();
}

int PluginHost::getLatencySamples() const 
{
    return plugin != nullptr ? plugin->getLatencySamples() : 0;
}

int PluginHost::getMainInputChannels() const
{
    return plugin != nullptr ? plugin -> getMainBusNumInputChannels() : 0;
}

int PluginHost::getMainOutputChannels() const
{
    return plugin != nullptr ? plugin -> getMainBusNumOutputChannels() : 0;
}

int PluginHost::getRequiredChannels() const
{
    if (plugin == nullptr) return 0;
        return juce::jmax(plugin -> getTotalNumInputChannels(), plugin -> getTotalNumOutputChannels());
}

int PluginHost::getNumParameters() const 
{
    return plugin != nullptr ? plugin->getParameters().size() : 0;
}

juce::String PluginHost::getParameterName(int index) const
{
    if (plugin == nullptr) return {};
    if (auto* p = plugin->getParameters()[index]) return p -> getName(64);
    return {};
}

float PluginHost::getParameterValue(int index) const
{
    if (plugin == nullptr) return 0.0f;
    if (auto* p = plugin->getParameters()[index]) return p -> getValue();
    return 0.0f;
}

void PluginHost::setParameterValue(int index, float value)
{
    if (plugin == nullptr) return;
    if (auto* p = plugin -> getParameters()[index]) p -> setValueNotifyingHost( juce::jlimit(0.0f, 1.0f, value) );
}