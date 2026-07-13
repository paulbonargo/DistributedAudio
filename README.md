Author: Paul Bonargo Jr.

Current Status: Plugin built successfully and passed audio through in a DAW (Ableton 12 Standard Edition)
				Packetized audio to send over UDP
				Implemented python script for receiver to fully validate first milestone
				Confirmed standalone plugin sends out audio data packets to a receiver on same machine
				Confirmed plugin in a DAW sends out audio data packets to a receiver on network (laptop -> studio PC)
				Cornfirmed remote device round trip 
				remote device plugin spinup and audio data processing
				dynamic/configurable remote node
				parameter adjustment from host -> remote node
				TCP audio file processing side-job - processed audio caching -> back to host

Up next: Milestone 3 work : further jitter & packet loss & latency evaluation
							validation of changes - actual plugin hosting node remote node 
							goal: real selectable VST3 hosting + live parameters
