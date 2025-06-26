#!/bin/bash
# Download NAIST Japanese Dictionary for OpenJTalk

set -e

DICT_URL="https://sourceforge.net/projects/open-jtalk/files/Dictionary/open_jtalk_dic-1.11/open_jtalk_dic_utf_8-1.11.tar.gz/download"
DICT_DIR="${1:-./naist-jdic}"

echo "Downloading NAIST Japanese Dictionary..."
mkdir -p "$DICT_DIR"
cd "$DICT_DIR"

# Download dictionary
curl -L -o dict.tar.gz "$DICT_URL"

# Extract
tar -xzf dict.tar.gz --strip-components=1

# Clean up
rm dict.tar.gz

echo "Dictionary downloaded to: $DICT_DIR"