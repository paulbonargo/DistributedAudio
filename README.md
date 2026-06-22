Author: Paul Bonargo Jr.

Current Status: Plugin built successfully and passed audio through in a DAW (Ableton 12 Standard Edition)
				Packetized audio to send over UDP
				Implemented python script for receiver to fully validate first milestone
				Confirmed standalone plugin sends out audio data packets to a receiver on same machine
				Confirmed plugin in a DAW sends out audio data packets to a receiver on network (laptop -> studio PC)

Up next: Milestone 2 work : jitter evaluation - needed for LAN? peer-to-peer?
							remote device sending - receiver component in cpp 
							remote device plugin spinup and audio data processing
							parameter adjustment from host -> remote node
							dynamic/configurable remote host
							setup for redundancy
							TCP audio file processing side-job - processed audio caching -> back to host
							option/switch in plugin on host side for live playback audio file rendering / preprocessed caching
							