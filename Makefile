.PHONY: firmware mac test release
firmware:
	pio run -e cardputer_adv

mac:
	swiftc -O mac/VoiceMIDI.swift -o mac/voice-midi

test:
	python3 tools/check.py

release: firmware test
	python3 tools/package.py
