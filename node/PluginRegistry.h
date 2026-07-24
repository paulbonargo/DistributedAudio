/*
  ==============================================================================

    This file contains framework code for a Registry of Plugins for Node processing

  ==============================================================================
*/

#pragma once

#include <juce_core/juce_core.h>
#include <vector>

//==============================================================================
/**
*/

struct RegisteredPlugin {
    int id = 0;
    juce::String name;
    juce::File path;
};

class PluginRegistry
{
    public:
        void loadFromFile(const juce::File& config);
        const std::vector<RegisteredPlugin>& plugins() const noexcept
        {
            return entries;
        }
        const RegisteredPlugin* byId(int id) const noexcept;

    private:
        std::vector<RegisteredPlugin> entries;
};