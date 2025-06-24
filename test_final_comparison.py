#!/usr/bin/env python3
"""Final comparison test between Python and C++ implementations"""

import subprocess
import sys
import wave
import os
from pathlib import Path
from datetime import datetime

# Add src/python_run to path
sys.path.insert(0, str(Path(__file__).parent / "src" / "python_run"))

from piper import PiperVoice

def test_implementations():
    # Model paths
    model_path = "/Users/s19447/Desktop/tmp/css10_ja_epoch1999.onnx"
    config_path = "/Users/s19447/Desktop/tmp/css10_ja_epoch1999.onnx.json"
    
    # Test texts
    test_texts = [
        ("konnichiwa", "こんにちは、世界"),
        ("arigatou", "ありがとうございます"),
        ("chotto", "ちょっと待ってください"),
        ("nihongo", "日本語の音声合成テストです")
    ]
    
    # Create timestamp for unique filenames
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    
    print("=== Final Comparison Test ===")
    print(f"Timestamp: {timestamp}")
    print(f"Model: {model_path}")
    print()
    
    # Load Python voice
    print("Loading Python voice...")
    voice = PiperVoice.load(model_path, config_path=config_path)
    
    generated_files = []
    
    for test_id, text in test_texts:
        print(f"\nTest: {test_id}")
        print(f"Text: {text}")
        
        # Python generation
        python_output = f"/Users/s19447/Desktop/tmp/{test_id}_python_{timestamp}.wav"
        print(f"  Python output: {python_output}")
        
        with wave.open(python_output, "wb") as wav_file:
            wav_file.setnchannels(1)
            wav_file.setsampwidth(2)
            wav_file.setframerate(voice.config.sample_rate)
            
            audio_bytes = bytes()
            for audio in voice.synthesize_stream_raw(text):
                audio_bytes += audio
            
            wav_file.writeframes(audio_bytes)
        
        # Get Python phonemes
        phonemes = voice.phonemize(text)[0]
        readable_phonemes = []
        for ph in phonemes:
            if len(ph) == 1 and ord(ph) >= 0xE000 and ord(ph) <= 0xF8FF:
                # Find original multi-char phoneme
                from piper.voice import MULTI_CHAR_TO_PUA
                for orig, pua in MULTI_CHAR_TO_PUA.items():
                    if pua == ph:
                        readable_phonemes.append(orig)
                        break
                else:
                    readable_phonemes.append(ph)
            else:
                readable_phonemes.append(ph)
        print(f"  Python phonemes: {' '.join(readable_phonemes)}")
        
        # C++ generation
        cpp_output = f"/Users/s19447/Desktop/tmp/{test_id}_cpp_{timestamp}.wav"
        print(f"  C++ output: {cpp_output}")
        
        env = {"ESPEAK_DATA_PATH": "/Users/s19447/Desktop/tmp/piper/espeak-ng-data"}
        proc = subprocess.run(
            ["/Users/s19447/Desktop/tmp/piper/bin/piper", 
             "--model", model_path,
             "--output_file", cpp_output],
            input=text.encode('utf-8'),
            capture_output=True,
            env={**os.environ, **env}
        )
        
        if proc.returncode != 0:
            print(f"  C++ error: {proc.stderr.decode('utf-8')}")
        else:
            print(f"  C++ generation successful")
        
        # Check file sizes
        if os.path.exists(python_output) and os.path.exists(cpp_output):
            py_size = os.path.getsize(python_output)
            cpp_size = os.path.getsize(cpp_output)
            print(f"  File sizes: Python={py_size:,} bytes, C++={cpp_size:,} bytes")
            
            generated_files.append({
                'test_id': test_id,
                'text': text,
                'python': python_output,
                'cpp': cpp_output
            })
    
    print("\n=== Generated Audio Files ===")
    for file_info in generated_files:
        print(f"\n{file_info['test_id']}: {file_info['text']}")
        print(f"  Python: {file_info['python']}")
        print(f"  C++:    {file_info['cpp']}")
    
    print("\n=== Play Commands ===")
    print("To play and compare the audio files, use these commands:")
    for file_info in generated_files:
        print(f"\n# {file_info['test_id']}: {file_info['text']}")
        print(f"afplay {file_info['python']}  # Python version")
        print(f"afplay {file_info['cpp']}     # C++ version")

if __name__ == "__main__":
    test_implementations()