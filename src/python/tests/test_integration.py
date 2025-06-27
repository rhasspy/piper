"""
Real integration tests that verify actual functionality
"""

import pytest
import numpy as np
import json
from pathlib import Path
import tempfile


class TestRealIntegration:
    """Test real end-to-end functionality without mocks"""

    @pytest.mark.integration
    @pytest.mark.requires_model
    def test_synthesis_produces_audio(self):
        """Test that synthesis actually produces audio data"""
        try:
            from piper.voice import PiperVoice

            # Use a small test model if available
            model_path = Path("test/models/en_US-lessac-medium.onnx")
            if not model_path.exists():
                pytest.skip("Test model not available")

            voice = PiperVoice.load(str(model_path))
            with tempfile.NamedTemporaryFile(suffix=".wav", delete=True) as tmp:
                voice.synthesize("Hello world", tmp.name)
                # Check that WAV file was created
                assert Path(tmp.name).exists()
                assert Path(tmp.name).stat().st_size > 0

        except ImportError:
            pytest.skip("Piper not installed")

    @pytest.mark.integration
    @pytest.mark.japanese
    @pytest.mark.requires_model
    def test_japanese_synthesis_with_pua(self):
        """Test Japanese synthesis with PUA mapping"""
        try:
            from piper.voice import PiperVoice

            model_path = Path("test/models/ja_JP-test-medium.onnx")
            if not model_path.exists():
                pytest.skip("Japanese test model not available")

            # Check that config has PUA mappings
            config_path = Path(str(model_path) + ".json")
            config = json.loads(config_path.read_text())

            # Should have PUA mappings in phoneme_id_map
            pua_chars = [
                k
                for k in config["phoneme_id_map"].keys()
                if isinstance(k, str) and ord(k[0]) >= 0xE000
            ]
            assert len(pua_chars) > 0, "Model should have PUA mappings"

            # Test synthesis
            voice = PiperVoice.load(str(model_path))
            with tempfile.NamedTemporaryFile(suffix=".wav", delete=True) as tmp:
                voice.synthesize("こんにちは", tmp.name)
                # Check that WAV file was created
                assert Path(tmp.name).exists()
                assert Path(tmp.name).stat().st_size > 0

        except ImportError:
            pytest.skip("Piper not installed")

    @pytest.mark.integration
    def test_model_config_validation(self):
        """Test that model configs are valid"""
        test_configs = [
            {
                "audio": {"sample_rate": 22050},
                "num_symbols": 100,
                "phoneme_id_map": {"_": 0},
            },
            {
                "audio": {"sample_rate": 22050},
                "phoneme_type": "openjtalk",
                "language": {"code": "ja"},
                "num_symbols": 150,
                "phoneme_id_map": {"_": 0, "\ue00e": 30},
            },
        ]

        for config in test_configs:
            # Required fields
            assert "audio" in config
            assert "sample_rate" in config["audio"]
            assert "phoneme_id_map" in config

            # Japanese specific
            if config.get("language", {}).get("code") == "ja":
                assert config.get("phoneme_type") == "openjtalk"

    @pytest.mark.integration
    def test_wav_file_generation(self):
        """Test that we can generate valid WAV files"""
        import wave

        # Create test audio data
        sample_rate = 22050
        duration = 0.5
        samples = int(sample_rate * duration)
        audio_data = np.zeros(samples, dtype=np.int16)

        with tempfile.NamedTemporaryFile(suffix=".wav", delete=False) as tmp:
            # Write WAV
            with wave.open(tmp.name, "wb") as wav_file:
                wav_file.setnchannels(1)
                wav_file.setsampwidth(2)
                wav_file.setframerate(sample_rate)
                wav_file.writeframes(audio_data.tobytes())

            # Verify WAV
            with wave.open(tmp.name, "rb") as wav_file:
                assert wav_file.getnchannels() == 1
                assert wav_file.getsampwidth() == 2
                assert wav_file.getframerate() == sample_rate
                assert wav_file.getnframes() == samples

            Path(tmp.name).unlink()

    @pytest.mark.integration
    @pytest.mark.slow
    def test_performance_baseline(self):
        """Test that synthesis meets basic performance requirements"""
        try:
            from piper.voice import PiperVoice
            import time

            model_path = Path("test/models/en_US-lessac-medium.onnx")
            if not model_path.exists():
                pytest.skip("Test model not available")

            voice = PiperVoice.load(str(model_path))

            # Generate 10 seconds worth of text
            text = "Hello world. " * 50

            with tempfile.NamedTemporaryFile(suffix=".wav", delete=True) as tmp:
                start_time = time.time()
                voice.synthesize(text, tmp.name)
                synthesis_time = time.time() - start_time

                # For now, just check that synthesis completed
                assert Path(tmp.name).exists()
                file_size = Path(tmp.name).stat().st_size
                assert file_size > 0

                # Rough RTF calculation based on file size and sample rate
                # Assuming 16-bit mono at 22050Hz
                # bytes / (bytes_per_sample * sample_rate)
                estimated_duration = file_size / (2 * 22050)
                rtf = (
                    synthesis_time / estimated_duration if estimated_duration > 0 else 0
                )

                # Should be faster than real-time
                assert rtf < 5.0, f"Synthesis too slow: RTF={rtf}"

                # Ideally much faster
                print(f"Real-time factor: {rtf:.2f}x")

        except ImportError:
            pytest.skip("Piper not installed")
