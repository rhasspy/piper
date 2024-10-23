import typing
from pathlib import Path

import numpy as np
import onnxruntime


class SileroVoiceActivityDetector:
    """Detects speech/silence using Silero VAD.

    https://github.com/snakers4/silero-vad
    """

    def __init__(self, onnx_path: typing.Union[str, Path]):
        onnx_path = str(onnx_path)

        self.session = onnxruntime.InferenceSession(onnx_path, providers=['CPUExecutionProvider'])
        self.session.intra_op_num_threads = 1
        self.session.inter_op_num_threads = 1

        self._h = np.zeros((2, 1, 64)).astype("float32")
        self._c = np.zeros((2, 1, 64)).astype("float32")

    def __call__(self, audio_array: np.ndarray, sample_rate: int = 16000):
        """Return probability of speech in audio [0-1].

        Audio must be 16Khz 16-bit mono PCM.
        """
        if len(audio_array.shape) == 1:
            # Add batch dimension
            audio_array = np.expand_dims(audio_array, 0)

        if len(audio_array.shape) > 2:
            raise ValueError(
                f"Too many dimensions for input audio chunk {audio_array.shape}"
            )

        ort_inputs = {
            "input": audio_array.astype(np.float32),
            "h": self._h,
            "c": self._c,
            "sr": np.array(sample_rate, dtype=np.int64),
        }
        ort_outs = self.session.run(None, ort_inputs)
        out, self._h, self._c = ort_outs

        return out.squeeze()
