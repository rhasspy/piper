"""
Tests for existing token_mapper implementation
Testing the actual implementation without modifying it
"""

import pytest
from piper_train.phonemize.token_mapper import (
    TOKEN2CHAR,
    CHAR2TOKEN,
    register,
    map_sequence,
)


class TestTokenMapperImplementation:
    """Test the existing token mapper implementation"""

    @pytest.mark.unit
    def test_predefined_mappings_exist(self):
        """Test that predefined PUA mappings exist"""
        # These mappings are defined in the actual implementation
        expected_mappings = {
            "a:": "\ue000",
            "i:": "\ue001",
            "u:": "\ue002",
            "e:": "\ue003",
            "o:": "\ue004",
            "cl": "\ue005",
            "ky": "\ue006",
            "kw": "\ue007",
            "gy": "\ue008",
            "gw": "\ue009",
            "ty": "\ue00a",
            "dy": "\ue00b",
            "py": "\ue00c",
            "by": "\ue00d",
            "ch": "\ue00e",
            "ts": "\ue00f",
            "sh": "\ue010",
            "zy": "\ue011",
            "hy": "\ue012",
            "ny": "\ue013",
            "my": "\ue014",
            "ry": "\ue015",
        }

        for token, expected_char in expected_mappings.items():
            assert TOKEN2CHAR[token] == expected_char
            assert CHAR2TOKEN[expected_char] == token

    @pytest.mark.unit
    def test_register_new_mapping(self):
        """Test registering a new token mapping"""
        # Register a new mapping (using unreserved PUA character)
        new_token = "test_token"
        new_char = register(new_token)

        # Verify it was registered
        assert new_char is not None
        assert TOKEN2CHAR[new_token] == new_char
        assert CHAR2TOKEN[new_char] == new_token

        # Verify it's in PUA range
        code_point = ord(new_char)
        assert 0xE000 <= code_point <= 0xF8FF

    @pytest.mark.unit
    def test_register_existing_token_returns_same_char(self):
        """Test registering an existing token returns the same character"""
        # Try to register an existing token
        existing_token = "ch"
        expected_char = "\ue00e"

        result = register(existing_token)
        assert result == expected_char

    @pytest.mark.unit
    def test_map_sequence_basic(self):
        """Test mapping a sequence of tokens"""
        # Test sequence with both mapped and unmapped tokens
        sequence = ["k", "o", "n", "n", "i", "ch", "i", "w", "a"]

        mapped = map_sequence(sequence)

        # Verify the mapping
        assert mapped[0] == "k"  # unmapped
        assert mapped[1] == "o"  # unmapped
        assert mapped[5] == "\ue00e"  # "ch" -> PUA
        assert mapped[6] == "i"  # unmapped

    @pytest.mark.unit
    def test_map_sequence_with_multiple_mappings(self):
        """Test mapping sequence with multiple multi-char phonemes"""
        sequence = ["ch", "i", "ts", "u", "ky", "o"]

        mapped = map_sequence(sequence)

        assert mapped[0] == "\ue00e"  # ch
        assert mapped[1] == "i"
        assert mapped[2] == "\ue00f"  # ts
        assert mapped[3] == "u"
        assert mapped[4] == "\ue006"  # ky
        assert mapped[5] == "o"

    @pytest.mark.unit
    def test_map_sequence_empty(self):
        """Test mapping an empty sequence"""
        assert map_sequence([]) == []

    @pytest.mark.unit
    def test_map_sequence_no_mappings(self):
        """Test mapping sequence with no multi-char phonemes"""
        sequence = ["a", "i", "u", "e", "o"]
        mapped = map_sequence(sequence)
        assert mapped == sequence  # Should be unchanged

    @pytest.mark.unit
    def test_char_to_token_reverse_mapping(self):
        """Test reverse mapping from PUA char to token"""
        # Test all predefined reverse mappings
        for char, token in CHAR2TOKEN.items():
            assert TOKEN2CHAR[token] == char

    @pytest.mark.unit
    @pytest.mark.parametrize(
        "token,expected_char",
        [
            ("a:", "\ue000"),  # long vowels
            ("i:", "\ue001"),
            ("u:", "\ue002"),
            ("e:", "\ue003"),
            ("o:", "\ue004"),
            ("cl", "\ue005"),  # consonants
            ("ch", "\ue00e"),
            ("ts", "\ue00f"),
            ("sh", "\ue010"),
        ],
    )
    def test_specific_token_mappings(self, token, expected_char):
        """Test specific token to PUA character mappings"""
        assert TOKEN2CHAR[token] == expected_char
        assert CHAR2TOKEN[expected_char] == token
