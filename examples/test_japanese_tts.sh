#!/bin/bash

# Japanese TTS test script for piper
# This script downloads necessary files and tests Japanese TTS

set -e

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo "=== Japanese TTS Test Script ==="

# 1. Download OpenJTalk dictionary if not exists
if [ ! -d "build/naist-jdic" ]; then
    echo "Downloading OpenJTalk dictionary..."
    cd build
    curl -L -o dict.tar.gz "https://sourceforge.net/projects/open-jtalk/files/Dictionary/open_jtalk_dic-1.11/open_jtalk_dic_utf_8-1.11.tar.gz/download"
    tar -xzf dict.tar.gz
    mv open_jtalk_dic_utf_8-1.11 naist-jdic
    rm dict.tar.gz
    cd ..
fi

# 2. Download HTS voice model if not exists
if [ ! -f "build/nitech_jp_atr503_m001.htsvoice" ]; then
    echo "Downloading HTS voice model..."
    cd build
    curl -L -o voice.tar.gz "https://sourceforge.net/projects/open-jtalk/files/HTS%20voice/hts_voice_nitech_jp_atr503_m001-1.05/hts_voice_nitech_jp_atr503_m001-1.05.tar.gz/download"
    tar -xzf voice.tar.gz
    cp hts_voice_nitech_jp_atr503_m001-1.05/*.htsvoice .
    rm -rf voice.tar.gz hts_voice_nitech_jp_atr503_m001-1.05
    cd ..
fi

# 3. Create directory structure for piper
echo "Setting up directory structure..."
cd build
mkdir -p ../share/open_jtalk
cp -r naist-jdic ../share/open_jtalk/dic

# 4. Test open_jtalk directly
echo "Testing OpenJTalk..."
echo "こんにちは" | open_jtalk -x naist-jdic -m nitech_jp_atr503_m001.htsvoice -ot trace.txt -ow /dev/null
if [ -f trace.txt ]; then
    echo "OpenJTalk trace output:"
    head -5 trace.txt
    rm trace.txt
fi

# 5. Run piper with Japanese text
echo "Testing piper with Japanese text..."
export OPENJTALK_DICTIONARY_DIR=$PWD/naist-jdic
export DYLD_LIBRARY_PATH=$PWD/pi/lib:$DYLD_LIBRARY_PATH
export OPENJTALK_VOICE=$PWD/nitech_jp_atr503_m001.htsvoice

echo "こんにちは、音声合成のテストです。" > test_ja.txt

./piper --model ../test/models/ja_JP-test-medium.onnx --output_file test_output_ja.wav < test_ja.txt

if [ -f test_output_ja.wav ]; then
    echo "Success! Generated test_output_ja.wav"
    ls -la test_output_ja.wav
    
    # Play if afplay is available
    if command -v afplay &> /dev/null; then
        echo "Playing audio..."
        afplay test_output_ja.wav
    fi
else
    echo "Error: Failed to generate audio"
fi

cd ..