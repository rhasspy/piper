"""
Tests for existing utility function implementations
"""
import pytest
import numpy as np
from piper.util import audio_float_to_int16


class TestUtilImplementation:
    """Test the existing utility implementations"""
    
    @pytest.mark.unit
    def test_audio_float_to_int16_basic(self):
        """Test basic float to int16 conversion"""
        # Test normal range values
        float_audio = np.array([0.0, 0.5, -0.5, 1.0, -1.0], dtype=np.float32)
        int16_audio = audio_float_to_int16(float_audio)
        
        assert int16_audio.dtype == np.int16
        assert len(int16_audio) == len(float_audio)
        
        # Check conversions
        assert int16_audio[0] == 0  # 0.0 -> 0
        assert int16_audio[1] > 0  # 0.5 -> positive
        assert int16_audio[2] < 0  # -0.5 -> negative
        assert int16_audio[3] == 32767  # 1.0 -> max int16
        assert int16_audio[4] == -32768  # -1.0 -> min int16
    
    @pytest.mark.unit
    def test_audio_float_to_int16_clipping(self):
        """Test clipping of out-of-range values"""
        # Test values outside [-1, 1]
        float_audio = np.array([1.5, -1.5, 2.0, -2.0], dtype=np.float32)
        int16_audio = audio_float_to_int16(float_audio)
        
        # Should be clipped to int16 range
        assert np.all(int16_audio <= 32767)
        assert np.all(int16_audio >= -32768)
        
        # Specifically check clipping
        assert int16_audio[0] == 32767  # 1.5 clipped to max
        assert int16_audio[1] == -32768  # -1.5 clipped to min
    
    @pytest.mark.unit
    def test_audio_float_to_int16_empty(self):
        """Test conversion of empty array"""
        float_audio = np.array([], dtype=np.float32)
        int16_audio = audio_float_to_int16(float_audio)
        
        assert int16_audio.dtype == np.int16
        assert len(int16_audio) == 0
    
    @pytest.mark.unit
    def test_audio_float_to_int16_large_array(self):
        """Test conversion of large array"""
        # Generate 1 second of audio at 22050 Hz
        sample_rate = 22050
        duration = 1.0
        t = np.linspace(0, duration, int(sample_rate * duration))
        
        # Generate sine wave
        frequency = 440  # A4
        float_audio = np.sin(2 * np.pi * frequency * t).astype(np.float32)
        
        int16_audio = audio_float_to_int16(float_audio)
        
        assert int16_audio.dtype == np.int16
        assert len(int16_audio) == len(float_audio)
        assert int16_audio.max() <= 32767
        assert int16_audio.min() >= -32768
    
    @pytest.mark.unit
    def test_audio_float_to_int16_preserves_silence(self):
        """Test that silence remains silence"""
        # Array of zeros
        float_audio = np.zeros(1000, dtype=np.float32)
        int16_audio = audio_float_to_int16(float_audio)
        
        assert np.all(int16_audio == 0)
    
    @pytest.mark.unit
    @pytest.mark.parametrize("input_value,expected", [
        (0.0, 0),
        (0.25, 8191),  # Approximately 32767 * 0.25
        (-0.25, -8192),  # Approximately -32768 * 0.25
        (0.99999, 32767),  # Should round to max
        (-0.99999, -32768),  # Should round to min
    ])
    def test_audio_float_to_int16_specific_values(self, input_value, expected):
        """Test specific value conversions"""
        float_audio = np.array([input_value], dtype=np.float32)
        int16_audio = audio_float_to_int16(float_audio)
        
        # Allow for small rounding differences
        assert abs(int16_audio[0] - expected) <= 1
    
    @pytest.mark.unit
    def test_audio_float_to_int16_maintains_shape(self):
        """Test that array shape is preserved"""
        # Test 1D array
        float_audio_1d = np.random.randn(100).astype(np.float32)
        int16_audio_1d = audio_float_to_int16(float_audio_1d)
        assert int16_audio_1d.shape == float_audio_1d.shape
        
        # Test that it handles only 1D arrays (most implementations expect 1D)
        # Multi-dimensional should either work or raise appropriate error