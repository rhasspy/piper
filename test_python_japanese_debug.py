#!/usr/bin/env python3
"""Test Japanese TTS with Python API - Debug version"""

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
    output_path = "/Users/s19447/Desktop/tmp/hello_python_debug.wav"
    
    # Test imports
    print("Testing imports...")
    try:
        from piper_train.phonemize.japanese import phonemize_japanese
        print("✓ piper_train.phonemize.japanese imported successfully")
    except Exception as e:
        print(f"✗ Failed to import piper_train.phonemize.japanese: {e}")
    
    try:
        import pyopenjtalk_plus
        print("✓ pyopenjtalk_plus imported successfully")
    except Exception as e:
        print(f"✗ Failed to import pyopenjtalk_plus: {e}")
    
    # Check phoneme type
    print("\nChecking config...")
    voice = PiperVoice.load(model_path, config_path=config_path)
    print(f"Phoneme type: {voice.config.phoneme_type}")
    print(f"Phoneme map keys (first 10): {list(voice.config.phoneme_id_map.keys())[:10]}")
    
    # Test text
    text = "こんにちは、世界"
    print(f"\nSynthesizing: {text}")
    
    # Test phonemization directly
    print("\nDirect phonemization test:")
    try:
        import pyopenjtalk_plus as pyopenjtalk
        result = pyopenjtalk.g2p(text, kana=False)
        print(f"pyopenjtalk.g2p result: {result}")
    except Exception as e:
        print(f"pyopenjtalk.g2p failed: {e}")
    
    # Test voice phonemization
    phonemes_list = voice.phonemize(text)
    for phonemes in phonemes_list:
        print(f"Voice phonemes: {phonemes}")

if __name__ == "__main__":
    test_japanese_tts()