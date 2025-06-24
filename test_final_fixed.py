#!/usr/bin/env python3
"""Final test with fixed C++ implementation"""

import subprocess
import sys
import wave
import os
from pathlib import Path
from datetime import datetime

# Add src/python_run to path
sys.path.insert(0, str(Path(__file__).parent / "src" / "python_run"))

from piper import PiperVoice

# Model paths
model_path = "/Users/s19447/Desktop/tmp/css10_ja_epoch1999.onnx"
config_path = "/Users/s19447/Desktop/tmp/css10_ja_epoch1999.onnx.json"

# Test text
text = "日本語の音声合成テストです"
timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")

print(f"Testing: {text}")

# Python generation
python_output = f"/Users/s19447/Desktop/tmp/test_python_{timestamp}.wav"
voice = PiperVoice.load(model_path, config_path=config_path)

with wave.open(python_output, "wb") as wav_file:
    wav_file.setnchannels(1)
    wav_file.setsampwidth(2)
    wav_file.setframerate(voice.config.sample_rate)
    
    audio_bytes = bytes()
    for audio in voice.synthesize_stream_raw(text):
        audio_bytes += audio
    
    wav_file.writeframes(audio_bytes)

print(f"Python: {python_output}")

# C++ generation
cpp_output = f"/Users/s19447/Desktop/tmp/test_cpp_{timestamp}.wav"
env = {"ESPEAK_DATA_PATH": "/Users/s19447/Desktop/tmp/piper/espeak-ng-data"}
subprocess.run(
    ["/Users/s19447/Desktop/tmp/piper/bin/piper", 
     "--model", model_path,
     "--output_file", cpp_output],
    input=text.encode('utf-8'),
    env={**os.environ, **env}
)

print(f"C++:    {cpp_output}")
print("\nPlay commands:")
print(f"afplay {python_output} && sleep 1 && afplay {cpp_output}")