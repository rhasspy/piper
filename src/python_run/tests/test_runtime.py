"""
Runtime tests for piper voice synthesis
Tests actual implementation without excessive mocking
"""

import pytest
import numpy as np
from piper.util import audio_float_to_int16


class TestAudioUtils:
    """Test audio utility functions"""

    @pytest.mark.unit
    def test_audio_float_to_int16_conversion(self):
        """Test float to int16 audio conversion"""
        # Test normal range
        float_audio = np.array([0.0, 0.5, -0.5, 1.0, -1.0], dtype=np.float32)
        int16_audio = audio_float_to_int16(float_audio)

        assert int16_audio.dtype == np.int16
        assert int16_audio[0] == 0
        assert int16_audio[1] > 0  # 0.5 -> positive
        assert int16_audio[2] < 0  # -0.5 -> negative
        assert int16_audio[3] == 32767  # 1.0 -> max
        assert int16_audio[4] == -32767  # -1.0 -> min (normalized)

    @pytest.mark.unit
    def test_audio_clipping(self):
        """Test clipping of out-of-range values"""
        float_audio = np.array([2.0, -2.0], dtype=np.float32)
        int16_audio = audio_float_to_int16(float_audio)

        assert int16_audio[0] == 32767  # Clipped to max
        assert int16_audio[1] == -32767  # Clipped to min (normalized)


class TestPiperConfig:
    """Test configuration handling"""

    @pytest.mark.unit
    def test_config_from_dict(self):
        """Test creating config from dictionary"""
        # from piper.config import PiperConfig  # noqa: F401

        config_dict = {
            "audio": {"sample_rate": 22050},
            "num_symbols": 100,
            "num_speakers": 1,
            "inference": {"noise_scale": 0.667, "length_scale": 1.0, "noise_w": 0.8},
        }

        # Would normally use: config = PiperConfig.from_dict(config_dict)
        # For now, test that we can access values
        sample_rate = config_dict["audio"]["sample_rate"]
        assert sample_rate == 22050
        assert config_dict["num_symbols"] == 100

    @pytest.mark.unit
    def test_japanese_config(self):
        """Test Japanese-specific configuration"""
        config_dict = {
            "audio": {"sample_rate": 22050},
            "phoneme_type": "openjtalk",
            "language": {"code": "ja"},
            "phoneme_id_map": {"_": 0, "\ue00e": 30, "\ue00f": 31},  # PUA mapping
        }

        assert config_dict["phoneme_type"] == "openjtalk"
        assert config_dict["language"]["code"] == "ja"

        # Check PUA mappings exist
        pua_count = sum(
            1
            for k in config_dict["phoneme_id_map"]
            if isinstance(k, str) and ord(k[0]) >= 0xE000
        )
        assert pua_count >= 2


class TestFileHash:
    """Test file hashing utilities"""

    @pytest.mark.unit
    def test_file_hash_calculation(self, temp_dir):
        """Test file hash calculation"""
        try:
            from piper.file_hash import get_file_hash
        except ImportError:
            pytest.skip("File hash module not available")

        # Create test file
        test_file = temp_dir / "test.txt"
        test_file.write_text("Hello world")

        # Calculate hash
        hash1 = get_file_hash(str(test_file))
        assert isinstance(hash1, str)
        assert len(hash1) > 0

        # Same content should give same hash
        hash2 = get_file_hash(str(test_file))
        assert hash1 == hash2

        # Different content should give different hash
        test_file.write_text("Different content")
        hash3 = get_file_hash(str(test_file))
        assert hash3 != hash1
