
import argparse
import socket
import struct
import array
import sys
import math
import sys


HEADER = struct.Struct('<IHHIIQQB3s') # 36 bytes
SIGNATURE = 0x44495354 # "DIST" in ASCII
VERSION = 2
FLAG_PROCESSED = 0x01

NODE_AUDIO_PORT = 9000
HOST_AUDIO_PORT = 9001

def main():
    ap = argparse.ArgumentParser(description="DistributedAudio Python processing node")
    
    ap.add_argument("--gain", 
                    type=float, 
                    default=1.0, 
                    help="linear gain")

    ap.add_argument("--lowpass", 
                    type=float, 
                    default=0.0, 
                    help="one-pole low-pass cutoff Hz (0 = off)")

    ap.add_argument("--in-port", 
                    type=int, 
                    default=NODE_AUDIO_PORT)
    
    ap.add_argument("--out-port", 
                    type=int, 
                    default=HOST_AUDIO_PORT)

    args = ap.parse_args()

    rx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    rx.bind(("", args.in_port))

    tx = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    print(f"processing node: in udp {args.in_port} -> out udp {args.out_port}, gain {args.gain}, lowpass {args.lowpass} Hz")

    lp_state = None
    count = 0

    try:

        while True:

            data, addr = rx.recvfrom(65536)
            if len(data) < HEADER.size:
                continue
            
            sig, ver, ch, rate, nsamp, seq, start, flags, _res = HEADER.unpack_from(data)
            if sig != SIGNATURE or ver != VERSION:
                continue

            payload = data[HEADER.size:]
            samples = array.array("f")

            samples.frombytes(payload)

            if sys.byteorder == "big":
                samples.byteswap()

            if args.gain != 1.0:
                for i in range(len(samples)):
                    samples[i] *= args.gain
                
            if args.lowpass > 0.0 and ch > 0:

                if lp_state is None or len(lp_state) != ch:
                    lp_state = [0.0] * ch

                rc = 1.0 / (2.0 * math.pi * args.lowpass)
                dt = 1.0 / rate
                alpha = dt / (rc + dt)

                for f in range(nsamp):
                    base = f * ch

                    for c in range(ch):
                        y = lp_state[c] + alpha * (samples[base + c] - lp_state[c])
                        lp_state[c] = y
                        samples[base + c] = y

            if sys.byteorder == "big":
                samples.byteswap()

            payload = samples.tobytes()

            out = HEADER.pack(sig, ver, ch, rate, nsamp, seq, start,
                              FLAG_PROCESSED, b"\x00\x00\x00") + payload
            tx.sendto(out, (addr[0], args.out_port))
            count += 1

    except KeyboardInterrupt:

        print(f"\nprocessed {count} packets")


if __name__ == "__main__":
    main()