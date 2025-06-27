"""
Pytest configuration for piper runtime tests
"""

import pytest
import tempfile
import shutil
from pathlib import Path


@pytest.fixture
def temp_dir():
    """Create a temporary directory for test files"""
    temp_path = tempfile.mkdtemp()
    yield Path(temp_path)
    shutil.rmtree(temp_path)


@pytest.fixture
def mock_model_path(temp_dir):
    """Create a mock model file for testing"""
    model_path = temp_dir / "test_model.onnx"
    model_path.touch()

    # Create corresponding JSON config
    config_path = temp_dir / "test_model.onnx.json"
    config_content = {
        "audio": {"sample_rate": 22050},
        "espeak": {"voice": "en-us"},
        "inference": {"noise_scale": 0.667, "length_scale": 1, "noise_w": 0.8},
        "phoneme_id_map": {"_": [0], "a": [1]},
    }

    import json

    config_path.write_text(json.dumps(config_content))

    return model_path


@pytest.fixture
def mock_japanese_model_path(temp_dir):
    """Create a mock Japanese model file for testing"""
    model_path = temp_dir / "ja_JP_test.onnx"
    model_path.touch()

    # Create corresponding JSON config with Japanese settings
    config_path = temp_dir / "ja_JP_test.onnx.json"
    config_content = {
        "audio": {"sample_rate": 22050},
        "inference": {"noise_scale": 0.667, "length_scale": 1, "noise_w": 0.8},
        "phoneme_type": "openjtalk",
        "language": {"code": "ja"},
        "phoneme_id_map": {
            "_": [0],
            "a": [1],
            "i": [2],
            "u": [3],
            "e": [4],
            "o": [5],
            # PUA mappings
            "\ue00e": [30],  # ch
            "\ue00f": [31],  # ts
        },
    }

    import json

    config_path.write_text(json.dumps(config_content))

    return model_path
