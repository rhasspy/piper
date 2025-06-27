import json
import logging
import wave
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Dict, Iterable, List, Optional, Tuple, Union

import numpy as np
import onnxruntime

# Try to import piper_phonemize, but make it optional
try:
    from piper_phonemize import phonemize_codepoints, phonemize_espeak, tashkeel_run

    HAS_PIPER_PHONEMIZE = True
except ImportError:
    HAS_PIPER_PHONEMIZE = False

    # Provide fallback implementations
    def phonemize_codepoints(text, lang=None):
        # Simple fallback: return text as list of characters
        return list(text)

    def phonemize_espeak(text, voice=None):
        # Simple fallback: return text as list of characters
        return list(text)

    def tashkeel_run(text):
        # Simple fallback: return original text
        return text


# Try to import pyopenjtalk, but make it optional
try:
    import pyopenjtalk

    HAS_PYOPENJTALK = True
except ImportError:
    HAS_PYOPENJTALK = False

from .config import PhonemeType, PiperConfig
from .const import BOS, EOS, PAD
from .util import audio_float_to_int16

_LOGGER = logging.getLogger(__name__)

# Multi-character phoneme to PUA character mapping for Japanese
# This must match the C++ side and Python training side
MULTI_CHAR_TO_PUA = {
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


@dataclass
class PiperVoice:
    session: onnxruntime.InferenceSession
    config: PiperConfig

    @staticmethod
    def load(
        model_path: Union[str, Path],
        config_path: Optional[Union[str, Path]] = None,
        use_cuda: bool = False,
    ) -> "PiperVoice":
        """Load an ONNX model and config."""
        if config_path is None:
            config_path = f"{model_path}.json"

        with open(config_path, "r", encoding="utf-8") as config_file:
            config_dict = json.load(config_file)

        providers: List[Union[str, Tuple[str, Dict[str, Any]]]]
        if use_cuda:
            providers = [
                (
                    "CUDAExecutionProvider",
                    {"cudnn_conv_algo_search": "HEURISTIC"},
                )
            ]
        else:
            providers = ["CPUExecutionProvider"]

        return PiperVoice(
            config=PiperConfig.from_dict(config_dict),
            session=onnxruntime.InferenceSession(
                str(model_path),
                sess_options=onnxruntime.SessionOptions(),
                providers=providers,
            ),
        )

    def phonemize(self, text: str) -> List[List[str]]:
        """Text to phonemes grouped by sentence."""
        if self.config.phoneme_type == PhonemeType.ESPEAK:
            if self.config.espeak_voice == "ar":
                # Arabic diacritization
                # https://github.com/mush42/libtashkeel/
                text = tashkeel_run(text)

            return phonemize_espeak(text, self.config.espeak_voice)

        if self.config.phoneme_type == PhonemeType.TEXT:
            return phonemize_codepoints(text)

        if self.config.phoneme_type == PhonemeType.OPENJTALK:
            # Piper の学習時と同じアルゴリズム（accent/prosody 付き）で音素化
            try:
                # `piper_train` がインストールされていれば専用実装を利用
                from piper_train.phonemize.japanese import phonemize_japanese  # type: ignore

                tokens = phonemize_japanese(text)
                return [tokens]
            except Exception:  # pragma: no cover – フォールバック
                # 学習環境に piper_train が無い場合の簡易フォールバック
                phonemes = pyopenjtalk.g2p(text, kana=False).split()

                converted = []
                # Add BOS marker
                converted.append("^")

                for ph in phonemes:
                    if ph == "pau":
                        converted.append("_")
                        continue

                    if ph == "sil":
                        # Skip sil in the middle, it will be added as EOS
                        continue

                    # Devoiced vowels come back as upper-case (A,I,U,E,O)
                    # But NOT 'N' which is a special phoneme
                    if ph in {"A", "I", "U", "E", "O"}:
                        ph = ph.lower()

                    # Check if this is a multi-character phoneme that needs PUA mapping
                    if ph in MULTI_CHAR_TO_PUA:
                        converted.append(MULTI_CHAR_TO_PUA[ph])
                    else:
                        converted.append(ph)

                # Add EOS marker
                converted.append("$")

                # Log readable phonemes if debug logging is enabled
                if _LOGGER.isEnabledFor(logging.DEBUG):
                    readable_phonemes = []
                    for ph in converted:
                        if len(ph) == 1 and ord(ph) >= 0xE000 and ord(ph) <= 0xF8FF:
                            # Find the original multi-char phoneme
                            for orig, pua in MULTI_CHAR_TO_PUA.items():
                                if pua == ph:
                                    readable_phonemes.append(orig)
                                    break
                            else:
                                readable_phonemes.append(ph)
                        else:
                            readable_phonemes.append(ph)
                    _LOGGER.debug(
                        "Phonemized '%s' to: %s", text, " ".join(readable_phonemes)
                    )

                return [converted]

        raise ValueError(f"Unexpected phoneme type: {self.config.phoneme_type}")

    def phonemes_to_ids(self, phonemes: List[str]) -> List[int]:
        """Phonemes to ids."""
        id_map = self.config.phoneme_id_map
        ids: List[int] = list(id_map[BOS])

        for phoneme in phonemes:
            if phoneme not in id_map:
                _LOGGER.warning("Missing phoneme from id map: %s", phoneme)
                continue

            ids.extend(id_map[phoneme])

            # 学習データが PAD("_") を各音素ごとに含んでいるのは eSpeak 方式のみ。
            # openjtalk で学習したモデルでは PAD は明示的に含まれていないので追加しない。
            if self.config.phoneme_type != PhonemeType.OPENJTALK:
                ids.extend(id_map[PAD])

        ids.extend(id_map[EOS])

        return ids

    def synthesize(
        self,
        text: str,
        wav_file: wave.Wave_write,
        speaker_id: Optional[int] = None,
        length_scale: Optional[float] = None,
        noise_scale: Optional[float] = None,
        noise_w: Optional[float] = None,
        sentence_silence: float = 0.0,
    ):
        """Synthesize WAV audio from text."""
        wav_file.setframerate(self.config.sample_rate)
        wav_file.setsampwidth(2)  # 16-bit
        wav_file.setnchannels(1)  # mono

        for audio_bytes in self.synthesize_stream_raw(
            text,
            speaker_id=speaker_id,
            length_scale=length_scale,
            noise_scale=noise_scale,
            noise_w=noise_w,
            sentence_silence=sentence_silence,
        ):
            wav_file.writeframes(audio_bytes)

    def synthesize_stream_raw(
        self,
        text: str,
        speaker_id: Optional[int] = None,
        length_scale: Optional[float] = None,
        noise_scale: Optional[float] = None,
        noise_w: Optional[float] = None,
        sentence_silence: float = 0.0,
    ) -> Iterable[bytes]:
        """Synthesize raw audio per sentence from text."""
        sentence_phonemes = self.phonemize(text)

        # 16-bit mono
        num_silence_samples = int(sentence_silence * self.config.sample_rate)
        silence_bytes = bytes(num_silence_samples * 2)

        for phonemes in sentence_phonemes:
            phoneme_ids = self.phonemes_to_ids(phonemes)
            yield self.synthesize_ids_to_raw(
                phoneme_ids,
                speaker_id=speaker_id,
                length_scale=length_scale,
                noise_scale=noise_scale,
                noise_w=noise_w,
            ) + silence_bytes

    def synthesize_ids_to_raw(
        self,
        phoneme_ids: List[int],
        speaker_id: Optional[int] = None,
        length_scale: Optional[float] = None,
        noise_scale: Optional[float] = None,
        noise_w: Optional[float] = None,
    ) -> bytes:
        """Synthesize raw audio from phoneme ids."""
        if length_scale is None:
            length_scale = self.config.length_scale

        if noise_scale is None:
            noise_scale = self.config.noise_scale

        if noise_w is None:
            noise_w = self.config.noise_w

        phoneme_ids_array = np.expand_dims(np.array(phoneme_ids, dtype=np.int64), 0)
        phoneme_ids_lengths = np.array([phoneme_ids_array.shape[1]], dtype=np.int64)
        scales = np.array(
            [noise_scale, length_scale, noise_w],
            dtype=np.float32,
        )

        args = {
            "input": phoneme_ids_array,
            "input_lengths": phoneme_ids_lengths,
            "scales": scales,
        }

        if self.config.num_speakers <= 1:
            speaker_id = None

        if (self.config.num_speakers > 1) and (speaker_id is None):
            # Default speaker
            speaker_id = 0

        if speaker_id is not None:
            sid = np.array([speaker_id], dtype=np.int64)
            args["sid"] = sid

        # Synthesize through Onnx
        audio = self.session.run(
            None,
            args,
        )[
            0
        ].squeeze((0, 1))
        audio = audio_float_to_int16(audio.squeeze())
        return audio.tobytes()
