Author: Paul Bonargo Jr.

Current Status: Plugin built successfully and passed audio through in a DAW (Ableton 12 Standard Edition)
				Packetized audio to send over UDP
				Implemented python script for receiver to fully validate first milestone
				Confirmed standalone plugin sends out audio data packets to a receiver on same machine
				Confirmed plugin in a DAW sends out audio data packets to a receiver on network (laptop -> studio PC)
				Confirmed remote device round trip 
				Remote device plugin spinup and audio data processing
				Dynamic/configurable remote node
				Parameter adjustment from host -> remote node
				Validation of changes - actual plugin hosting node remote node
				UI ease of use changes, privacy, and metrics
				Persistence within one session and on session reboot - application settings and parameters
				Plugin registry for selection
				Adjustable latency values within the plugin UI
				Address information on host plugin and node
				Goal completed: real selectable VST3 hosting + live parameter editing

Up next: Milestone 3+ work : Complete packet loss & latency evaluation
                             Parallel network plugin execution - port expansion
							 Automatic plugin scanning in specified or vst3 directory 

Stretch goals: TCP audio file processing side-job - processed audio caching -> back to host
         	   Network discovery for new Nodes
