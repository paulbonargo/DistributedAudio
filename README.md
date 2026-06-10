Author: Paul Bonargo Jr.

Current Status: Plugin builds successfully and passes audio through in a DAW (Ableton 12 Standard Edition)
				Implemented SenderThread - except for run() - current one is for local testing with DBG
				Implemented more of distributed audio packet header and plugin processor logic
				Packetize audio and send over UDP
				Implement rough python script for receiver to fully validate first milestone

Up next: Confirm plugin sends out audio data packets to a receiver on network