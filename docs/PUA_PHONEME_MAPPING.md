# PUA Phoneme Mapping for Japanese TTS

## Overview

This document explains the Private Use Area (PUA) phoneme mapping system used in Piper for Japanese text-to-speech synthesis. The system ensures consistency between Python training code and C++ inference by mapping multi-character phonemes to single Unicode characters.

## Background

Japanese phonemes in OpenJTalk often consist of multiple characters (e.g., "ch", "ts", "ky"). However, Piper's architecture expects each phoneme to be represented by a single Unicode character. To solve this, we use Unicode's Private Use Area (U+E000-U+F8FF) to create single-character representations of multi-character phonemes.

## PUA Mapping Table

The following fixed mappings are used consistently across Python and C++ code:

| Phoneme | PUA Code | Character | Description |
|---------|----------|-----------|-------------|
| a: | U+E000 | | Long vowel 'a' |
| i: | U+E001 | | Long vowel 'i' |
| u: | U+E002 | | Long vowel 'u' |
| e: | U+E003 | | Long vowel 'e' |
| o: | U+E004 | | Long vowel 'o' |
| cl | U+E005 | | Geminate consonant (っ) |
| ky | U+E006 | | Palatalized 'k' (きゃ) |
| kw | U+E007 | | Labialized 'k' (くゎ) |
| gy | U+E008 | | Palatalized 'g' (ぎゃ) |
| gw | U+E009 | | Labialized 'g' (ぐゎ) |
| ty | U+E00A | | Palatalized 't' (ちゃ) |
| dy | U+E00B | | Palatalized 'd' (ぢゃ) |
| py | U+E00C | | Palatalized 'p' (ぴゃ) |
| by | U+E00D | | Palatalized 'b' (びゃ) |
| ch | U+E00E | | Affricate (ち) |
| ts | U+E00F | | Affricate (つ) |
| sh | U+E010 | | Fricative (し) |
| zy | U+E011 | | Voiced fricative (じ) |
| hy | U+E012 | | Palatalized 'h' (ひゃ) |
| ny | U+E013 | | Palatalized 'n' (にゃ) |
| my | U+E014 | | Palatalized 'm' (みゃ) |
| ry | U+E015 | | Palatalized 'r' (りゃ) |

## Implementation

### Python Side (Training)

The mapping is implemented in `src/python/piper_train/phonemize/token_mapper.py`:

```python
from piper_train.phonemize.token_mapper import TOKEN2CHAR, CHAR2TOKEN

# Convert multi-character phoneme to PUA character
pua_char = TOKEN2CHAR["ch"]  # Returns ''

# Convert PUA character back to phoneme
phoneme = CHAR2TOKEN['']  # Returns "ch"
```

### C++ Side (Inference)

The mapping is implemented in `src/cpp/openjtalk_phonemize.cpp`:

```cpp
// Multi-character phoneme to PUA mapping
static std::unordered_map<std::string, char32_t> multiCharToPUA = {
    {"ch", 0xE00E},
    {"ts", 0xE00F},
    // ... etc
};

// Function to map phoneme strings
Phoneme mapPhonemeStr(const std::string &phonemeStr);
```

## Updating Existing Models

To update existing model configurations to use PUA mappings, use the provided script:

```bash
# Update a single model
python -m piper_train.update_model_config path/to/model.onnx.json

# Update multiple models
python -m piper_train.update_model_config models/*.onnx.json

# Dry run to see what would change
python -m piper_train.update_model_config --dry-run model.onnx.json

# Update without creating backups
python -m piper_train.update_model_config --no-backup model.onnx.json
```

## Benefits

1. **Consistency**: Ensures Python training and C++ inference use identical phoneme representations
2. **Accuracy**: Preserves phonetic information that would be lost by splitting multi-character phonemes
3. **Compatibility**: Works with existing Piper architecture without structural changes
4. **Performance**: Reduces "Missing phoneme" warnings during synthesis

## Technical Details

### Why PUA?

The Private Use Area is a range of Unicode codepoints (U+E000-U+F8FF) reserved for application-specific use. These characters have no predefined meaning, making them perfect for our custom phoneme mappings.

### Model Configuration

In model JSON files, the `phoneme_id_map` section maps phonemes to numeric IDs:

```json
"phoneme_id_map": {
    "_": [0],
    "a": [7],
    "": [30],  // PUA character for "ch"
    "": [31],  // PUA character for "ts"
    // ... etc
}
```

### Debugging

When debugging, PUA characters may appear as boxes or question marks in text editors. Use the reverse mapping functions to convert them back to readable phonemes:

- Python: `CHAR2TOKEN[pua_char]`
- C++: `phonemeToDisplayString(phoneme)`

## Future Considerations

1. The current implementation uses fixed mappings starting at U+E000
2. Dynamic allocation starts at U+E020 for any additional phonemes
3. The system can be extended to support other languages with multi-character phonemes