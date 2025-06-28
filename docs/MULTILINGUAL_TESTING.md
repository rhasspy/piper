# Multilingual TTS Testing Guide

This document describes how to test Piper's multilingual text-to-speech capabilities.

## Overview

Piper supports TTS for over 40 languages. To ensure quality across all supported languages, we have implemented comprehensive testing infrastructure that:

- Tests multiple languages on all supported platforms (Linux, Windows, macOS)
- Downloads voice models from HuggingFace during testing
- Validates output quality and performance
- Supports different test modes (basic, comprehensive, performance)

## Supported Languages

The following languages are tested in our CI/CD pipeline:

| Language | Code | Model | Quality |
|----------|------|-------|---------|
| English (US) | en_US | lessac | medium |
| English (UK) | en_GB | alan | medium |
| German | de_DE | thorsten | medium |
| French | fr_FR | upmc | medium |
| Spanish | es_ES | mls_9972 | low |
| Italian | it_IT | riccardo | x_low |
| Portuguese (Brazil) | pt_BR | faber | medium |
| Russian | ru_RU | denis | medium |
| Chinese (Simplified) | zh_CN | huayan | medium |
| Dutch | nl_NL | nathalie | x_low |
| Polish | pl_PL | gosia | medium |
| Swedish | sv_SE | nst | medium |

Additional languages are available and can be tested using the local test script.

## Running Tests Locally

### Prerequisites

1. Download or build Piper binary for your platform
2. Python 3.7+ installed
3. Internet connection for downloading voice models

### Basic Testing

Run the test script to test all default languages:

```bash
python test_multilingual_tts.py --piper ./piper/bin/piper
```

### Test Specific Languages

```bash
python test_multilingual_tts.py --piper ./piper/bin/piper --languages en_US de_DE fr_FR
```

### Performance Testing

Run comprehensive performance tests:

```bash
python test_multilingual_tts.py --piper ./piper/bin/piper --test-type performance
```

### Test Output

The script generates:
- Individual WAV files for each language tested
- Performance metrics (characters per second)
- Detailed error reports for failed tests
- Summary report of all test results

## CI/CD Integration

### GitHub Actions Workflow

The multilingual tests run automatically on:
- Pull requests to master or dev branches
- Pushes to dev, feat/*, and fix/* branches
- Manual workflow dispatch

### Running CI/CD Tests Manually

1. Go to Actions tab in GitHub
2. Select "Test Multilingual TTS" workflow
3. Click "Run workflow"
4. Optionally specify:
   - Languages to test (comma-separated)
   - Test type (basic, comprehensive, performance)

### Test Matrix

Tests run on the following matrix:
- **Operating Systems**: Ubuntu (latest), Windows (latest), macOS (latest)
- **Languages**: 12 major languages by default
- **Architectures**: x64, ARM64 (where applicable)

Note: Chinese (zh_CN) tests are skipped on Windows due to phonemizer limitations.

## Adding New Languages

To add a new language to the test suite:

1. Find the model on [HuggingFace](https://huggingface.co/rhasspy/piper-voices)
2. Add language configuration to `test_multilingual_tts.py`:

```python
"lang_CODE": {
    "model": "lang_CODE-speaker-quality",
    "test_text": "Test text in the target language",
    "speaker": "speaker_name"
}
```

3. Add the language to the CI/CD workflow matrix in `.github/workflows/test-multilingual-tts.yml`

## Troubleshooting

### Common Issues

1. **Model download fails**
   - Check internet connection
   - Verify model exists on HuggingFace
   - Check URL structure matches the repository layout

2. **TTS fails for specific language**
   - Ensure proper phonemizer support
   - Check if language requires special dependencies
   - Verify model compatibility with Piper version

3. **Performance issues**
   - Some languages are computationally more intensive
   - Lower quality models (x_low, low) are faster
   - Consider using GPU acceleration if available

### Platform-Specific Notes

- **Linux**: Requires LD_LIBRARY_PATH to be set for shared libraries
- **macOS**: Requires DYLD_LIBRARY_PATH for dynamic libraries
- **Windows**: Some languages (e.g., Chinese, Arabic) may have limited support

## Model Quality Levels

Models are available in different quality levels:

- **x_low**: Fastest, lowest quality (good for testing)
- **low**: Fast, acceptable quality
- **medium**: Balanced speed and quality (recommended)
- **high**: Best quality, slower processing

## Performance Benchmarks

Typical performance on modern hardware:

| Language | Quality | Chars/Second |
|----------|---------|--------------|
| English | medium | 500-800 |
| German | medium | 400-700 |
| Chinese | medium | 200-400 |
| Arabic | medium | 300-500 |

Performance varies based on:
- CPU/GPU capabilities
- Model complexity
- Text content (numbers, special characters affect speed)

## Contributing

When contributing multilingual support:

1. Test your changes locally with multiple languages
2. Ensure CI/CD tests pass for all platforms
3. Document any language-specific requirements
4. Add appropriate test cases for edge cases

## Related Documentation

- [Main README](../README.md)
- [Japanese Usage Guide](../JAPANESE_USAGE.md)
- [Voice Models](../VOICES.md)
- [Training Guide](../TRAINING.md)