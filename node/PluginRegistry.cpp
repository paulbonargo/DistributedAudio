/*
  ==============================================================================

    PluginRegistry implementation

  ==============================================================================
*/

#include "PluginRegistry.h"

//==============================================================================
/**
*/


void PluginRegistry::loadFromFile(const juce::File& config)
{
    entries.clear();
    if (! config.existsAsFile()) 
    { 
        DBG("PluginRegistry: config not found at " << config.getFullPathName()); 
        return; 
    }

    juce::StringArray lines;
    config.readLines(lines);

    int nextId = 0;
    for (auto raw : lines)
    {
        const juce::String line = raw.trim();
        if (line.isEmpty() || line.startsWith("#")) continue;

        juce::File path(line);
        entries.push_back({ nextId++, path.getFileNameWithoutExtension(), path });
    }
}

const RegisteredPlugin* PluginRegistry::byId(int id) const noexcept
{
    for (auto& e : entries) if (e.id == id) return &e;
    return nullptr;
}