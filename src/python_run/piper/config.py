"""Piper configuration"""

from dataclasses import dataclass
from enum import Enum
from typing import Any, Dict, Mapping, Sequence


class PhonemeType(str, Enum):
    ESPEAK = "espeak"
    TEXT = "text"
    PHONEMES = "phonemes"


@dataclass
class PiperConfig:
    """Piper configuration"""

    num_symbols: int
    """Number of phonemes"""

    num_speakers: int
    """Number of speakers"""

    sample_rate: int
    """Sample rate of output audio"""

    voice: str
    """Name of voice or alphabet"""

    length_scale: float
    noise_scale: float
    noise_w: float

    phoneme_id_map: Mapping[str, Sequence[int]]
    """Phoneme -> [id,]"""

    phoneme_type: PhonemeType
    """espeak or text"""

    @staticmethod
    def from_dict(config: Dict[str, Any]) -> "PiperConfig":
        inference = config.get("inference", {})

        return PiperConfig(
            num_symbols=config["num_symbols"],
            num_speakers=config["num_speakers"],
            sample_rate=config["audio"]["sample_rate"],
            noise_scale=inference.get("noise_scale", 0.667),
            length_scale=inference.get("length_scale", 1.0),
            noise_w=inference.get("noise_w", 0.8),
            #
            voice=get_voice(config),
            phoneme_id_map=config["phoneme_id_map"],
            phoneme_type=get_phoneme_type(config.get("phoneme_type", "ESPEAK")),
        )


def get_voice(config: Dict[str, Any]) -> str:
    if "voice" in config and isinstance(config["voice"], str):
        return config["voice"]
    for key, section_data in config.items():
        if isinstance(section_data, dict) and "voice" in section_data:
            return section_data["voice"]
    return ""


def get_phoneme_type(config_value: str) -> PhonemeType:
    if config_value.startswith("PhonemeType."):
        config_value = config_value.split(".")[-1]
    try:
        return PhonemeType[config_value.upper()]
    except KeyError:
        return PhonemeType.ESPEAK
