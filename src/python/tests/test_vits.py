"""
Essential VITS utility tests
Only test critical functionality that affects inference
"""
import pytest
import torch
from piper_train.vits import commons


class TestVITSUtils:
    """Test essential VITS utilities"""
    
    @pytest.mark.unit
    @pytest.mark.training
    def test_sequence_mask(self):
        """Test sequence mask generation for variable length inputs"""
        lengths = torch.tensor([10, 8, 12])
        max_len = 15
        
        mask = commons.sequence_mask(lengths, max_len)
        
        assert mask.shape == (3, max_len)
        # First sequence: True for first 10, False for rest
        assert torch.all(mask[0, :10])
        assert not torch.any(mask[0, 10:])
    
    @pytest.mark.unit
    @pytest.mark.training
    def test_get_padding(self):
        """Test padding calculation for convolutions"""
        # Standard cases
        assert commons.get_padding(kernel_size=1, dilation=1) == 0
        assert commons.get_padding(kernel_size=3, dilation=1) == 1
        assert commons.get_padding(kernel_size=5, dilation=1) == 2
        
        # With dilation
        assert commons.get_padding(kernel_size=3, dilation=2) == 2
    
    @pytest.mark.unit
    @pytest.mark.training
    def test_intersperse(self):
        """Test intersperse for adding blanks between phonemes"""
        lst = [1, 2, 3]
        result = commons.intersperse(lst, item=0)
        
        assert result == [0, 1, 0, 2, 0, 3, 0]  # Actual implementation adds item at start and end too
        assert commons.intersperse([], item=0) == []
        assert commons.intersperse([1], item=0) == [0, 1, 0]