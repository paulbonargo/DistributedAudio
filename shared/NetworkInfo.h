/*
  ==============================================================================

  Local network address helper used by host plugin and remote node for 
  easier address configuration

  ==============================================================================
*/

#pragma once
#include <juce_core/juce_core.h>

//==============================================================================
/**
*/

namespace DistributedAudio
{
    inline bool isReachableLocalAddress(const juce::IPAddress& addr)
    {
        if (addr.isNull() || addr.isIPv6)
            return false;

        const int a = addr.address[0]; // IPv4 octets = address[0..3]
        const int b = addr.address[1];

        if (a == 10)
            return true; // 10.0.0.0/8 ----- private

        if (a == 172 && b >= 16 && b <= 31)
            return true; // 172.16.0.0/12 -- private

        if (a == 192 && b == 168)
            return true; // 192.168.0.0/16 - private

        if (a == 100 && b >= 64 && b <= 127)
            return true; // 100.64.0.0/10 -- CGNAT (Tailscale)

        return false;
    }

    // local IPv4 addresses, 1 per line
    inline juce::String getLocalAddressList()
    {
        const juce::IPAddress primary = juce::IPAddress::local(false);
        juce::StringArray lines;

        for (const auto& addr : juce::IPAddress::getAllAddresses(false))
        {
            if (! isReachableLocalAddress(addr))
                continue;

            juce::String line = addr.toString();

            if (! primary.isNull() && addr == primary)
                line << "   (primary)";

            lines.addIfNotAlreadyThere(line);
        }

        return lines.isEmpty() ? juce::String("(No Network Address)") : lines.joinIntoString("\n");
    } 
}