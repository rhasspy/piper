#!/usr/bin/env python3
"""Test Japanese TTS with Python API"""

import sys
import wave
from pathlib import Path

# Add src/python_run to path
sys.path.insert(0, str(Path(__file__).parent / "src" / "python_run"))

from piper import PiperVoice

def test_japanese_tts():
    # Model path
    model_path = "/Users/s19447/Desktop/tmp/css10_ja_epoch1999.onnx"
    config_path = "/Users/s19447/Desktop/tmp/css10_ja_epoch1999.onnx.json"
    
    # Output path
    output_path = "/Users/s19447/Desktop/tmp/hello_python.wav"
    
    # Load voice
    print("Loading model...")
    voice = PiperVoice.load(model_path, config_path=config_path)
    
    # Test text
    text = "こんにちは、世界"
    print(f"Synthesizing: {text}")
    
    # Synthesize
    with wave.open(output_path, "wb") as wav_file:
        wav_file.setnchannels(1)  # mono
        wav_file.setsampwidth(2)  # 16-bit
        wav_file.setframerate(voice.config.sample_rate)
        
        # Generate audio
        audio_bytes = bytes()
        for audio in voice.synthesize_stream_raw(text):
            audio_bytes += audio
        
        wav_file.writeframes(audio_bytes)
    
    print(f"Audio saved to: {output_path}")
    
    # Also test phoneme extraction
    print("\nPhoneme extraction:")
    phonemes_list = voice.phonemize(text)
    for phonemes in phonemes_list:
        print(f"  Phonemes: {phonemes}")

if __name__ == "__main__":
    test_japanese_tts()