#!/usr/bin/env python3
"""Test long vowels and special phonemes"""

import sys
from pathlib import Path

# Add src/python_run to path
sys.path.insert(0, str(Path(__file__).parent / "src" / "python_run"))

from piper import PiperVoice

def test_special_phonemes():
    # Model path
    model_path = "/Users/s19447/Desktop/tmp/css10_ja_epoch1999.onnx"
    config_path = "/Users/s19447/Desktop/tmp/css10_ja_epoch1999.onnx.json"
    
    # Test texts with special phonemes
    test_texts = [
        "学校",  # gakkou - should have long 'o:'
        "ちょっと",  # chotto - should have 'ch' and 'tt' (cl)
        "東京",  # toukyou - should have 'ky' and long vowels
        "りょうり",  # ryouri - should have 'ry'
        "しゃしん",  # shashin - should have 'sh'
    ]
    
    print("=== Testing special phonemes ===\n")
    
    # Load Python voice
    voice = PiperVoice.load(model_path, config_path=config_path)
    
    for text in test_texts:
        phonemes_list = voice.phonemize(text)
        phonemes = phonemes_list[0] if phonemes_list else []
        
        # Show both regular and with Unicode codepoints for PUA characters
        print(f"Text: {text}")
        print(f"Phonemes: {' '.join(phonemes)}")
        
        # Show PUA characters as hex
        pua_chars = []
        for p in phonemes:
            if len(p) == 1 and ord(p) >= 0xE000 and ord(p) <= 0xF8FF:
                pua_chars.append(f"{p}(U+{ord(p):04X})")
        
        if pua_chars:
            print(f"PUA chars: {' '.join(pua_chars)}")
        
        print()

if __name__ == "__main__":
    test_special_phonemes()