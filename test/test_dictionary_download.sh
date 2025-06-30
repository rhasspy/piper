#!/bin/bash
# Test script for OpenJTalk dictionary auto-download functionality

set -e

echo "=== OpenJTalk Dictionary Auto-Download Test ==="
echo

# Set up paths
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
export ESPEAK_DATA_PATH="$PROJECT_ROOT/build/ei/share/espeak-ng-data"
export DYLD_LIBRARY_PATH="$PROJECT_ROOT/build:$PROJECT_ROOT/build/pi/lib:$DYLD_LIBRARY_PATH"
export LD_LIBRARY_PATH="$PROJECT_ROOT/build:$PROJECT_ROOT/build/pi/lib:$LD_LIBRARY_PATH"

# Change to build directory for proper path resolution
cd "$PROJECT_ROOT/build"
PIPER_BIN="./piper"

# Clean up any existing test environment
TEST_DIR="/tmp/piper_dict_test_$$"
mkdir -p "$TEST_DIR"

echo "Test directory: $TEST_DIR"
echo

# Test 1: Auto-download disabled
echo "Test 1: Auto-download disabled (should fail)"
export PIPER_AUTO_DOWNLOAD_DICT=0
export OPENJTALK_DICTIONARY_DIR="$TEST_DIR/nonexistent"
if echo "テスト" | "$PIPER_BIN" --model "$PROJECT_ROOT/test/models/ja_JP-test-medium.onnx" --output_file test1.wav 2>&1 | grep -q "Please download the dictionary manually"; then
    echo "✓ Test 1 passed: Correctly failed when auto-download is disabled"
else
    echo "✗ Test 1 failed: Should have failed with manual download message"
fi
echo

# Test 2: Auto-download enabled (default)
echo "Test 2: Auto-download enabled (should download)"
unset PIPER_AUTO_DOWNLOAD_DICT
export HOME="$TEST_DIR"
unset OPENJTALK_DICTIONARY_DIR

echo "Running piper with auto-download..."
if echo "こんにちは" | "$PIPER_BIN" --model "$PROJECT_ROOT/test/models/ja_JP-test-medium.onnx" --output_file test2.wav; then
    echo "✓ Test 2 passed: Auto-download succeeded"
    
    # Check if dictionary was downloaded
    if [ -d "$TEST_DIR/.piper/dictionaries/openjtalk/open_jtalk_dic_utf_8-1.11" ]; then
        echo "✓ Dictionary downloaded to correct location"
    else
        echo "✗ Dictionary not found at expected location"
    fi
else
    echo "✗ Test 2 failed: Auto-download failed"
fi
echo

# Test 3: Use existing dictionary
echo "Test 3: Use existing dictionary (should not re-download)"
if echo "ありがとう" | "$PIPER_BIN" --model "$PROJECT_ROOT/test/models/ja_JP-test-medium.onnx" --output_file test3.wav 2>&1 | grep -q "Downloading"; then
    echo "✗ Test 3 failed: Should not re-download existing dictionary"
else
    echo "✓ Test 3 passed: Used existing dictionary"
fi
echo

# Test 4: Offline mode
echo "Test 4: Offline mode (should use cached dictionary)"
export PIPER_OFFLINE_MODE=1
if echo "さようなら" | "$PIPER_BIN" --model "$PROJECT_ROOT/test/models/ja_JP-test-medium.onnx" --output_file test4.wav; then
    echo "✓ Test 4 passed: Offline mode works with cached dictionary"
else
    echo "✗ Test 4 failed: Offline mode should work with cached dictionary"
fi
echo

# Clean up
cd /
rm -rf "$TEST_DIR"

echo "=== Test completed ==="