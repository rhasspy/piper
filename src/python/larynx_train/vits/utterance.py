import json
from dataclasses import dataclass
from pathlib import Path
from subprocess import Popen
from typing import List, Optional

import librosa
import torch
from dataclasses_json import DataClassJsonMixin
from torch import Tensor


@dataclass
class DatasetUtterance(DataClassJsonMixin):
    id: str
    text: Optional[str] = None
    phonemes: Optional[List[str]] = None
    phoneme_ids: Optional[List[int]] = None
    audio_path: Optional[Path] = None
    audio_norm_path: Optional[Path] = None
    mel_spec_path: Optional[Path] = None
    speaker: Optional[str] = None
    speaker_id: Optional[int] = None

    def __post_init__(self):
        self._original_json: Optional[str] = None

    @property
    def original_json(self) -> str:
        if self._original_json is None:
            self._original_json = self.to_json(ensure_ascii=False)

        return self._original_json


@dataclass
class TrainingUtterance:
    id: str
    phoneme_ids: Tensor
    audio_norm: Tensor
    mel_spec: Tensor
    speaker_id: Optional[Tensor] = None


# -----------------------------------------------------------------------------


@dataclass
class UtteranceLoadingContext:
    cache_dir: Path
    is_multispeaker: bool
    phonemize: Optional[Popen] = None
    phonemes2ids: Optional[Popen] = None
    speaker2id: Optional[Popen] = None
    audio2norm: Optional[Popen] = None
    audio2spec: Optional[Popen] = None


def load_utterance(
    utterance_json: str, context: UtteranceLoadingContext
) -> TrainingUtterance:
    data_utterance = DatasetUtterance.from_json(utterance_json)

    # pylint: disable=protected-access
    data_utterance._original_json = utterance_json

    # Requirements:
    # 1. phoneme ids
    # 2. audio norm
    # 3. mel spec
    # 4. speaker id (if multispeaker)

    # 1. phoneme ids
    if data_utterance.phoneme_ids is None:
        _load_phoneme_ids(data_utterance, context)

    # 2. audio norm
    if (data_utterance.audio_norm_path is None) or (
        not data_utterance.audio_norm_path.exists()
    ):
        _load_audio_norm(data_utterance, context)

    # 3. mel spec
    if (data_utterance.mel_spec_path is None) or (
        not data_utterance.mel_spec_path.exists()
    ):
        _load_mel_spec(data_utterance, context)

    # 4. speaker id
    if context.is_multispeaker:
        if data_utterance.speaker_id is None:
            _load_speaker_id(data_utterance, context)

    # Convert to training utterance
    assert data_utterance.phoneme_ids is not None
    assert data_utterance.audio_norm_path is not None
    assert data_utterance.mel_spec_path is not None

    if context.is_multispeaker:
        assert data_utterance.speaker_id is not None

    train_utterance = TrainingUtterance(
        id=data_utterance.id,
        phoneme_ids=torch.LongTensor(data_utterance.phoneme_ids),
        audio_norm=torch.load(data_utterance.audio_norm_path),
        mel_spec=torch.load(data_utterance.mel_spec_path),
        speaker_id=None
        if data_utterance.speaker_id is None
        else torch.LongTensor(data_utterance.speaker_id),
    )

    return train_utterance


def _load_phoneme_ids(
    data_utterance: DatasetUtterance, context: UtteranceLoadingContext
):
    if data_utterance.phonemes is None:
        # Need phonemes first
        _load_phonemes(data_utterance, context)

    assert (
        data_utterance.phonemes is not None
    ), f"phonemes is required for phoneme ids: {data_utterance}"

    assert (
        context.phonemes2ids is not None
    ), f"phonemes2ids program is required for phoneme ids: {data_utterance}"

    assert context.phonemes2ids.stdin is not None
    assert context.phonemes2ids.stdout is not None

    # JSON in, JSON out
    print(data_utterance.original_json, file=context.phonemes2ids.stdin, flush=True)
    result_json = context.phonemes2ids.stdout.readline()
    result_dict = json.loads(result_json)

    # Update utterance
    data_utterance.phoneme_ids = result_dict["phoneme_ids"]
    data_utterance._original_json = result_json


def _load_phonemes(data_utterance: DatasetUtterance, context: UtteranceLoadingContext):
    assert (
        data_utterance.text is not None
    ), f"text is required for phonemes: {data_utterance}"

    assert (
        context.phonemize is not None
    ), f"phonemize program is required for phonemes: {data_utterance}"

    assert context.phonemize.stdin is not None
    assert context.phonemize.stdout is not None

    # JSON in, JSON out
    print(data_utterance.original_json, file=context.phonemize.stdin, flush=True)
    result_json = context.phonemize.stdout.readline()
    result_dict = json.loads(result_json)

    # Update utterance
    data_utterance.phonemes = result_dict["phoneme"]
    data_utterance._original_json = result_json


def _load_audio_norm(
    data_utterance: DatasetUtterance, context: UtteranceLoadingContext
):
    pass


def _load_mel_spec(data_utterance: DatasetUtterance, context: UtteranceLoadingContext):
    pass
