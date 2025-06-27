"""
Unified phonemization tests - combines all phonemization testing
"""

import pytest

# Try to import implementation, skip if not available
pytest.importorskip("piper_train.phonemize")

from piper_train.phonemize.token_mapper import TOKEN2CHAR, CHAR2TOKEN, map_sequence

# Japanese imports are optional
try:
    import pyopenjtalk
    from piper_train.phonemize.japanese import phonemize_japanese

    HAS_JAPANESE = True
except ImportError:
    HAS_JAPANESE = False


class TestPhonemization:
    """All phonemization tests in one place"""

    @pytest.mark.unit
    def test_token_mapper_pua_mappings(self):
        """Test PUA character mappings are correct"""
        # Critical mappings for Japanese
        assert TOKEN2CHAR["ch"] == "\ue00e"
        assert TOKEN2CHAR["ts"] == "\ue00f"
        assert TOKEN2CHAR["ky"] == "\ue006"

        # Verify bidirectional mapping
        for token, char in TOKEN2CHAR.items():
            assert CHAR2TOKEN[char] == token

    @pytest.mark.unit
    def test_map_sequence(self):
        """Test phoneme sequence mapping"""
        input_seq = ["k", "o", "n", "n", "i", "ch", "i", "w", "a"]
        mapped = map_sequence(input_seq)

        # "ch" should be mapped to PUA
        assert mapped[5] == "\ue00e"
        # Others should remain unchanged
        assert mapped[0] == "k"
        assert mapped[6] == "i"

    @pytest.mark.unit
    @pytest.mark.japanese
    @pytest.mark.requires_openjtalk
    def test_japanese_basic(self):
        """Test basic Japanese phonemization"""
        if not HAS_JAPANESE:
            pytest.skip("Japanese phonemizer not available")

        # Test hiragana
        phonemes = phonemize_japanese("あ")
        assert "^" in phonemes  # Start marker
        assert "a" in phonemes  # Phoneme
        assert "$" in phonemes  # End marker

        # Test with multi-char phonemes
        phonemes = phonemize_japanese("ちゃ")
        assert len(phonemes) > 2

    @pytest.mark.unit
    def test_empty_input(self):
        """Test empty input handling"""
        # Empty list
        result = map_sequence([])
        assert result == []
