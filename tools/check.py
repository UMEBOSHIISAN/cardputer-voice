"""Offline checks for the synth, MIDI protocol, display model and demo scores."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as directory:
    temp = Path(directory)
    for source in sorted((ROOT / "tests").glob("*.cpp")):
        binary = temp / source.stem
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++17", "-O2", source, "-o", binary], check=True)
        subprocess.run([binary], check=True)
    subprocess.run(["swift", ROOT / "mac/VoiceMIDI.swift", "--test"], check=True)
    for score in sorted((ROOT / "mac").glob("*を歌う.command")):
        subprocess.run([score, "--check"], check=True)
