#!/usr/bin/env python3
"""Test Python logging with readable phonemes"""

import sys
import logging
from pathlib import Path

# Enable debug logging
logging.basicConfig(level=logging.DEBUG, format='%(levelname)s: %(message)s')

sys.path.insert(0, str(Path(__file__).parent / "src" / "python_run"))

from piper import PiperVoice

# Load voice
voice = PiperVoice.load(
    "/Users/s19447/Desktop/tmp/css10_ja_epoch1999.onnx",
    config_path="/Users/s19447/Desktop/tmp/css10_ja_epoch1999.onnx.json"
)

# Test text
text = "ちょっと待ってください"
print(f"Input text: {text}")

# Get phonemes
phonemes = voice.phonemize(text)[0]
print(f"Raw phonemes ({len(phonemes)}): {phonemes}")