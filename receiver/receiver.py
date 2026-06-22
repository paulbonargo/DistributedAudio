"""
Distributed Audio UDP Receiver

Listens for packets from the AudioSenderPlugin, validates them against the
milestone 3 (DistributedAudio v2) format and writes the received 
float32 PCM -> 16-bit WAV on Ctrl+C (or after --idle-timeout seconds of 
silence once audio has been received).

Milestone 1 packet format (header):
    uint32_t signature      0x44495354  "DIST" in ASCII
    uint32_t version        1

    uint16_t numChannels

    uint32_t sampleRate
    uint32_t numSamples     samples aka frames in the packet

    uint64_t sequenceNumber
    uint64_t startSample

	uint8_t flags           bit 0 is kFlagProcessed

	uint8_t reserved[3];
"""

import argparse
import socket
import struct
import array
import sys
import wave

HEADER = struct.Struct('<IHHIIQB3s') # Q startSample + B flags + 3s reserved = 36 bytes
SIGNATURE = 0x44495354 # "DIST" in ASCII
VERSION = 1

def float32_to_int16(payload: bytes) -> bytes:

    samples = array.array('f')
    samples.frombytes(payload)

    if sys.byteorder == "big": # wire format is little-endian, so swap if host is big-endian
        samples.byteswap()

    # converts to ints in the range [-32767, 32767] with clipping
    ints = array.array('h', (int(max(-1.0, min(1.0, sample)) * 32767) for sample in samples))
    
    if sys.byteorder == "big": # WAV is little-endian
        ints.byteswap()

    return ints.tobytes()


def main():

    parser = argparse.ArgumentParser(description='Distributed Audio UDP Receiver')

    parser.add_argument('--port', 
                        type=int, 
                        default=9000, 
                        help='UDP port to listen on')

    parser.add_argument('--output', 
                        default="capture.wav", 
                        help='Output WAV file name')

    parser.add_argument('--idle-timeout', 
                        type=float, 
                        default=0.0, 
                        help='Seconds of silence after receiving audio to wait before writing output and exiting (0 = run until CTRL+C)')

    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(('', args.port))

    sock.settimeout(0.5) # for idle timeout checking

    print(f"Listening for audio packets on UDP port {args.port}... CTRL+C to stop and write WAV file")

    chunks = []
    channels = sample_rate = None

    packets = bad = lost = late = 0

    expected_seq = 0

    idle = 0.0

    try:
        while True:
            try:
                data, addr = sock.recvfrom(65536) # max UDP packet size

            except socket.timeout:
                idle += 0.5
                if idle >= args.idle_timeout and packets > 0 and idle >= args.idle_timeout:
                    print(f"Idle timeout of {args.idle_timeout} seconds reached after receiving audio, stopping capture")
                    break
                continue
            idle = 0.0

            if len(data) < HEADER.size:
                bad += 1
                continue

            sig, ver, n_ch, rate, n_samples, seq, start_sample, flags, _reserved = HEADER.unpack_from(data)
            payload = data[HEADER.size:]

            if sig != SIGNATURE or ver != VERSION:
                bad += 1
                continue

            if len(payload) != n_ch * n_samples * 4:
                bad += 1
                continue

            if channels is None:
                channels = n_ch
                sample_rate = rate
                print(f"stream from {addr[0]}: {n_ch} ch @ {rate} Hz, {n_samples} frames/packet")

            elif (n_ch, rate) != (channels, sample_rate):
                bad += 1 # mid-stream format change is not permitted
                continue

            if expected_seq is not None:

                if seq > expected_seq:
                    lost += seq - expected_seq # gap in packets
                        
                elif seq < expected_seq:
                    late += 1 # out-of-order packet 

            expected_seq = max(seq + 1, expected_seq or 0)

            chunks.append(payload)
            packets += 1

    except KeyboardInterrupt:
        print("Capture stopped by user")
        pass

    print(f"\nReceived packets: {packets} - lost(sequence gaps): {lost}, \t out-of-order: {late}, \t malformed: {bad}")
    
    if packets == 0:
        print("No packets received, exiting without writing WAV file")
        return

    total = packets + lost
    print(f"Loss rate: {lost / total * 100:.3f}%")

    pcm = float32_to_int16(b''.join(chunks))
    with wave.open(args.output, "wb") as wav:
        wav.setnchannels(channels)
        wav.setsampwidth(2)
        wav.setframerate(sample_rate)
        wav.writeframes(pcm)

        frames = len(pcm) // (channels * 2)

        print(f"WAV file '{args.output}' wrote: {frames} frames ({frames / sample_rate:.2f}s @ {sample_rate} Hz, {channels} channels)")


if __name__ == "__main__":
    main()