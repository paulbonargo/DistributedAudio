"""
Synthetic packet generator for testing receiver.py without a DAW

Sends a 440 Hz sine wave in DistributedAudio v1 packetsat roughly 
real-time pacing. --drop N skips every Nth packet to simulate packet loss.
"""

import argparse
import math
from re import A
import socket
import struct
import time

HEADER = struct.Struct('<IHHIIQ')
SIGNATURE = 0x44495354 # "DIST" in ASCII
VERSION = 1
FRAMES_PER_PACKET = 128

def main():
    parser = argparse.ArgumentParser(description='Synthetic Distributed Audio UDP Sender')
    parser.add_argument('--host', 
                        default='127.0.0.1', 
                        help='UDP host to send to')

    parser.add_argument('--port', 
                        type=int, 
                        default=9000, 
                        help='UDP port to send to')

    parser.add_argument('--seconds', 
                        type=int, 
                        default=2.0, 
                        help='Duration to send for')

    parser.add_argument('--rate', 
                        type=int, 
                        default=48000, 
                        help='Sample rate to send at')  

    parser.add_argument('--channels', 
                        type=int, 
                        default=2, 
                        help='Number of channels to send')

    parser.add_argument('--drop', 
                        type=int, 
                        default=0, 
                        help='Skip every Nth packet to simulate loss (0 = None)')
    
    args = parser.parse_args()

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    n_packets = int(args.seconds * args.rate / FRAMES_PER_PACKET)
    packet_period = FRAMES_PER_PACKET / args.rate
    sent = skipped = 0

    for seq in range(n_packets):
        frame0 = seq * FRAMES_PER_PACKET
        samples = []
        for i in range(FRAMES_PER_PACKET):
            sample = 0.5 * math.sin(2.0 * math.pi * 440 * (frame0 + i) / args.rate) # 440 Hz sine wave
            samples.extend([sample] * args.channels) # interleaved 

        packet = HEADER.pack(SIGNATURE, VERSION, args.channels, args.rate, FRAMES_PER_PACKET, seq) 
        packet += struct.pack(f"<{len(samples)}f", *samples)

        if args.drop > 0 and seq % args.drop == args.drop - 1:
            skipped += 1 # seq number is consumed but not sent
        
        else:
            sock.sendto(packet, (args.host, args.port))
            sent += 1
        
        time.sleep(packet_period)

    print(f"Sent {sent} packets, skipped {skipped} packets")

if __name__ == "__main__":
    main()
