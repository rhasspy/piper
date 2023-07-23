import ctypes
import io
import json
import logging
import wave
from dataclasses import dataclass
from pathlib import Path
from typing import List, Mapping, Optional, Sequence, Union

import numpy as np
import onnxruntime

_LOGGER = logging.getLogger(__name__)

_BOS = "^"
_EOS = "$"
_PAD = "_"

EE_OK = 0
AUDIO_OUTPUT_SYNCHRONOUS = 0x02
espeakPHONEMES_IPA = 0x02
espeakCHARS_AUTO = 0

CLAUSE_INTONATION_FULL_STOP = 0x00000000
CLAUSE_INTONATION_COMMA = 0x00001000
CLAUSE_INTONATION_QUESTION = 0x00002000
CLAUSE_INTONATION_EXCLAMATION = 0x00003000

CLAUSE_TYPE_SENTENCE = 0x00080000

@dataclass
class PiperConfig:
    num_symbols: int
    num_speakers: int
    sample_rate: int
    espeak_voice: str
    length_scale: float
    noise_scale: float
    noise_w: float
    phoneme_id_map: Mapping[str, Sequence[int]]


class CustomPhonemizer(object):
    """ A modified Phonemizer that keeps the punctuation.
        Needs a patched libespeak-ng.so from https://github.com/rhasspy/espeak-ng """
    def __init__(self, voice):
        # Set voice
        ret = forked_lib.espeak_SetVoiceByName(voice.encode("utf-8"))
        assert ret == EE_OK, ret

    def phonemize(self, text):
        text_pointer = ctypes.c_char_p(text.encode("utf-8"))

        phoneme_flags = espeakPHONEMES_IPA
        text_flags = espeakCHARS_AUTO

        phonemes = ""
        while text_pointer:
            terminator = ctypes.c_int(0)
            clause_phonemes = forked_lib.espeak_TextToPhonemesWithTerminator(
                ctypes.pointer(text_pointer),
                text_flags,
                phoneme_flags,
                ctypes.pointer(terminator),
            )
            if isinstance(clause_phonemes, bytes):
                phonemes += clause_phonemes.decode()

            # Check for punctuation.
            # The testing order here is critical.
            if (terminator.value & CLAUSE_INTONATION_EXCLAMATION) == CLAUSE_INTONATION_EXCLAMATION:
                phonemes += "!"
            elif (terminator.value & CLAUSE_INTONATION_QUESTION) == CLAUSE_INTONATION_QUESTION:
                phonemes += "?"
            elif (terminator.value & CLAUSE_INTONATION_COMMA) == CLAUSE_INTONATION_COMMA:
                phonemes += ","
            elif (terminator.value & CLAUSE_INTONATION_FULL_STOP) == CLAUSE_INTONATION_FULL_STOP:
                phonemes += "."

            # Check for end of sentence
            if (terminator.value & CLAUSE_TYPE_SENTENCE) == CLAUSE_TYPE_SENTENCE:
                phonemes += "\n"
            else:
                phonemes += " "
        return phonemes


# Check if we have the patched lib needed to use the CustomPhonemizer
forked_lib_available = False
try:
    forked_lib = ctypes.cdll.LoadLibrary("libespeak-ng.so")
    # Will fail if custom function is missing
    forked_lib.espeak_TextToPhonemesWithTerminator.restype = ctypes.c_char_p
    # Initialize
    forked_lib_available = forked_lib.espeak_Initialize(AUDIO_OUTPUT_SYNCHRONOUS, 0, None, 0) > 0
    Phonemizer = CustomPhonemizer
except ValueError:
    from espeak_phonemizer import Phonemizer


class Piper:
    def __init__(
        self,
        model_path: Union[str, Path],
        config_path: Optional[Union[str, Path]] = None,
        use_cuda: bool = False,
    ):
        if config_path is None:
            config_path = f"{model_path}.json"

        self.config = load_config(config_path)
        self.phonemizer = Phonemizer(self.config.espeak_voice)
        self.model = onnxruntime.InferenceSession(
            str(model_path),
            sess_options=onnxruntime.SessionOptions(),
            providers=["CPUExecutionProvider"]
            if not use_cuda
            else ["CUDAExecutionProvider"],
        )

    def synthesize_partial(
        self,
        text: str,
        speaker_id: Optional[int] = None,
        length_scale: Optional[float] = None,
        noise_scale: Optional[float] = None,
        noise_w: Optional[float] = None,
    ) -> List[np.ndarray]:
        """Synthesize WAV audio from text."""
        if length_scale is None:
            length_scale = self.config.length_scale

        if noise_scale is None:
            noise_scale = self.config.noise_scale

        if noise_w is None:
            noise_w = self.config.noise_w

        phonemes_str = self.phonemizer.phonemize(text)

        total_audio = []
        for s in phonemes_str.splitlines():
            # print(s)
            phonemes = [_BOS] + list(s)
            phoneme_ids: List[int] = []

            for phoneme in phonemes:
                if phoneme in self.config.phoneme_id_map:
                    phoneme_ids.extend(self.config.phoneme_id_map[phoneme])
                    phoneme_ids.extend(self.config.phoneme_id_map[_PAD])
                else:
                    _LOGGER.warning("No id for phoneme: `%s`", phoneme)

            phoneme_ids.extend(self.config.phoneme_id_map[_EOS])

            phoneme_ids_array = np.expand_dims(np.array(phoneme_ids, dtype=np.int64), 0)
            phoneme_ids_lengths = np.array([phoneme_ids_array.shape[1]], dtype=np.int64)
            scales = np.array(
                [noise_scale, length_scale, noise_w],
                dtype=np.float32,
            )

            if (self.config.num_speakers > 1) and (speaker_id is None):
                # Default speaker
                speaker_id = 0

            sid = None

            if speaker_id is not None:
                sid = np.array([speaker_id], dtype=np.int64)

            # Synthesize through Onnx
            audio = self.model.run(
                None,
                {
                    "input": phoneme_ids_array,
                    "input_lengths": phoneme_ids_lengths,
                    "scales": scales,
                    "sid": sid,
                },
            )[0].squeeze((0, 1))
            total_audio.append(audio.squeeze())

        return total_audio

    def synthesize(
        self,
        text: str,
        speaker_id: Optional[int] = None,
        length_scale: Optional[float] = None,
        noise_scale: Optional[float] = None,
        noise_w: Optional[float] = None,
    ) -> bytes:
        audios = self.synthesize_partial(
            text,
            speaker_id,
            length_scale,
            noise_scale,
            noise_w,
        )
        return self.audios_to_wav(audios)

    def audios_to_wav(
        self, audios: List[np.ndarray]
    ) -> bytes:
        # Convert to WAV
        with io.BytesIO() as wav_io:
            wav_file: wave.Wave_write = wave.open(wav_io, "wb")
            with wav_file:
                wav_file.setframerate(self.config.sample_rate)
                wav_file.setsampwidth(2)
                wav_file.setnchannels(1)
                wav_file.writeframes(audio_float_to_int16(audios))

            return wav_io.getvalue()


def load_config(config_path: Union[str, Path]) -> PiperConfig:
    with open(config_path, "r", encoding="utf-8") as config_file:
        config_dict = json.load(config_file)
        inference = config_dict.get("inference", {})

        return PiperConfig(
            num_symbols=config_dict["num_symbols"],
            num_speakers=config_dict["num_speakers"],
            sample_rate=config_dict["audio"]["sample_rate"],
            espeak_voice=config_dict["espeak"]["voice"],
            noise_scale=inference.get("noise_scale", 0.667),
            length_scale=inference.get("length_scale", 1.0),
            noise_w=inference.get("noise_w", 0.8),
            phoneme_id_map=config_dict["phoneme_id_map"],
        )


def audio_float_to_int16(
    audios: List[np.ndarray], max_wav_value: float = 32767.0
) -> np.ndarray:
    """Normalize audio and convert to int16 range"""
    mx = 0.01
    for audio in audios:
        mx = max(mx, np.max(np.abs(audio)))
    total_audio = b''
    for audio in audios:
        audio_norm = audio * (max_wav_value / mx)
        audio_norm = np.clip(audio_norm, -max_wav_value, max_wav_value)
        audio_norm = audio_norm.astype("int16")
        total_audio += audio_norm.tobytes()
    return total_audio
