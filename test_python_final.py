#!/usr/bin/env python3
"""Test Japanese TTS with Python API - Final comparison"""

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
    output_path = "/Users/s19447/Desktop/tmp/hello_python_final.wav"
    
    # Load voice
    print("Loading model...")
    voice = PiperVoice.load(model_path, config_path=config_path)
    
    # Test text
    text = "こんにちは、世界"
    print(f"Synthesizing: {text}")
    
    # Check phonemes
    phonemes_list = voice.phonemize(text)
    for phonemes in phonemes_list:
        print(f"Phonemes: {' '.join(phonemes)}")
    
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
    
    # Compare with C++ version
    print("\nComparison:")
    print("Python phonemes: k o N n i ch i w a _ s e k a i")
    print("C++ phonemes:    ^ k o N n i ch i w a _ s e k a i $")
    print("Note: Python version is missing sentence markers (^ and $)")

if __name__ == "__main__":
    test_japanese_tts()