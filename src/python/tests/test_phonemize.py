"""
Unified phonemization tests - combines all phonemization testing
"""
import pytest

# Try to import implementation, skip if not available
pytest.importorskip("piper_train.phonemize")

from piper_train.phonemize import phonemize_text
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
    @pytest.mark.parametrize("lang,text", [
        ("en-us", "Hello"),
        ("de-de", "Hallo"),
        ("fr-fr", "Bonjour"),
        ("ja-jp", "こんにちは"),
    ])
    def test_multiple_languages(self, lang, text):
        """Test phonemization for different languages"""
        try:
            phonemes = phonemize_text(text, lang)
            assert isinstance(phonemes, list)
            assert len(phonemes) > 0
        except ValueError as e:
            if "No phonemizer" in str(e):
                pytest.skip(f"Phonemizer for {lang} not available")
            else:
                raise
    
    @pytest.mark.unit
    def test_edge_cases(self):
        """Test edge cases"""
        # Empty string
        result = phonemize_text("", "en-us")
        assert result == [] or result == ["^", "$"]
        
        # Whitespace only  
        result = phonemize_text("   ", "en-us")
        assert len(result) <= 3  # Minimal output
        
        # Invalid language
        with pytest.raises(ValueError, match="No phonemizer"):
            phonemize_text("test", "invalid-lang")