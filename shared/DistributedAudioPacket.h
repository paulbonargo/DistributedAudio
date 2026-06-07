/*
  ==============================================================================

    This file contains framework code for an Audio UDP-based transport protocol.

  ==============================================================================
*/

#pragma once
#include <cstdint>

//==============================================================================
/**
*/

namespace DistributedAudio
{
	// protocol signature as 'DIST' , major version 1
	constexpr uint32_t kProtocolSignature = 0x44495354; 
	constexpr uint16_t kProtocolVersion = 1;

	// no padding
	#pragma pack(push, 1) 

	struct PacketHeader
	{
		uint32_t signature;
		uint16_t version;

		uint16_t numChannels;

		uint32_t sampleRate;
		uint32_t numSamples;

		uint64_t sequenceNumber;
	};

	// reset padding
	#pragma pack(pop)
}