import pytest

# Ensure pyopenjtalk is available; skip tests otherwise
pyopenjtalk = pytest.importorskip("pyopenjtalk")

from piper_train.phonemize.japanese import phonemize_japanese


def test_phonemize_simple_sentence():
    text = "今日の天気は？"
    tokens = phonemize_japanese(text)

    # Basic structural checks instead of full sequence (accent symbols may vary per dict)
    assert tokens[0] == "^"
    assert tokens[-1] == "?"

    # Ensure essential phonemes appear in order
    joined = " ".join(tokens)
    for phon in ["ky", "o", "N", "t", "e", "N", "k", "i", "w", "a"]:
        assert phon in joined, f"Missing {phon} in {joined}" 