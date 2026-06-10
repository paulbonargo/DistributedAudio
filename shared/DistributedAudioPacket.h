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

	// restores padding
	#pragma pack(pop)

	static_assert(sizeof(PacketHeader) == 24, "Packet header must be 24 bytes");

	// can adjust based on network MTU, but 128 frames at 32 bit float * 2 channel is
	// 1024 bytes + 24 byte header = 1048 bytes, which is under typical MTU of 1500
	constexpr int kFramesPerPacket = 128;

}