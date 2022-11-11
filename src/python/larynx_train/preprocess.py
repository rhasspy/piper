#!/usr/bin/env python3
import argparse
import dataclasses
import itertools
import json
import logging
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Set

from espeak_phonemizer import Phonemizer

from .norm_audio import cache_norm_audio, make_silence_detector
from .phonemize import DEFAULT_PHONEME_ID_MAP, phonemes_to_ids, phonemize

_LOGGER = logging.getLogger("preprocess")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--input-dir", required=True, help="Directory with audio dataset"
    )
    parser.add_argument(
        "--output-dir",
        required=True,
        help="Directory to write output files for training",
    )
    parser.add_argument("--language", required=True, help="eSpeak-ng voice")
    parser.add_argument(
        "--sample-rate",
        type=int,
        required=True,
        help="Target sample rate for voice (hertz)",
    )
    parser.add_argument("--cache-dir", help="Directory to cache processed audio files")
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO)
    logging.getLogger("numba").setLevel(logging.WARNING)

    # Convert to paths and create output directories
    args.input_dir = Path(args.input_dir)
    args.output_dir = Path(args.output_dir)
    args.output_dir.mkdir(parents=True, exist_ok=True)

    args.cache_dir = (
        Path(args.cache_dir)
        if args.cache_dir
        else args.output_dir / "cache" / str(args.sample_rate)
    )
    args.cache_dir.mkdir(parents=True, exist_ok=True)

    # Count speakers
    _LOGGER.info("Counting number of speakers in the dataset")
    speakers: Set[str] = set()
    for utt in mycroft_dataset(args.input_dir):
        speakers.add(utt.speaker or "")

    is_multispeaker = len(speakers) > 1
    speaker_ids: Dict[str, int] = {}

    if is_multispeaker:
        _LOGGER.info("%s speaker(s) detected", len(speakers))

        # Assign speaker ids in sorted order
        for speaker_id, speaker in enumerate(sorted(speakers)):
            speaker_ids[speaker] = speaker_id
    else:
        _LOGGER.info("Single speaker dataset")

    # Write config
    with open(args.output_dir / "config.json", "w", encoding="utf-8") as config_file:
        json.dump(
            {
                "audio": {
                    "sample_rate": args.sample_rate,
                },
                "espeak": {
                    "voice": args.language,
                },
                "inference": {"noise_scale": 0.667, "length_scale": 1, "noise_w": 0.8},
                "phoneme_map": {},
                "phoneme_id_map": DEFAULT_PHONEME_ID_MAP,
                "num_symbols": len(
                    set(itertools.chain.from_iterable(DEFAULT_PHONEME_ID_MAP.values()))
                ),
                "num_speakers": len(speakers),
                "speaker_id_map": speaker_ids,
            },
            config_file,
            ensure_ascii=False,
            indent=4,
        )
    _LOGGER.info("Wrote dataset config")

    # Used to trim silence
    silence_detector = make_silence_detector()

    with open(args.output_dir / "dataset.jsonl", "w", encoding="utf-8") as dataset_file:
        phonemizer = Phonemizer(default_voice=args.language)
        for utt in mycroft_dataset(args.input_dir):
            try:
                utt.audio_path = utt.audio_path.absolute()
                _LOGGER.debug(utt)

                utt.phonemes = phonemize(utt.text, phonemizer)
                utt.phoneme_ids = phonemes_to_ids(utt.phonemes)
                utt.audio_norm_path, utt.audio_spec_path = cache_norm_audio(
                    utt.audio_path, args.cache_dir, silence_detector, args.sample_rate
                )

                # JSONL
                json.dump(
                    dataclasses.asdict(utt),
                    dataset_file,
                    ensure_ascii=False,
                    cls=PathEncoder,
                )
                print("", file=dataset_file)
            except Exception:
                _LOGGER.exception("Failed to process utterance: %s", utt)


# -----------------------------------------------------------------------------


@dataclass
class Utterance:
    text: str
    audio_path: Path
    speaker: Optional[str] = None
    phonemes: Optional[List[str]] = None
    phoneme_ids: Optional[List[int]] = None
    audio_norm_path: Optional[Path] = None
    audio_spec_path: Optional[Path] = None


class PathEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, Path):
            return str(o)
        return super().default(o)


def mycroft_dataset(dataset_dir: Path) -> Iterable[Utterance]:
    for info_path in dataset_dir.glob("*.info"):
        wav_path = info_path.with_suffix(".wav")
        if wav_path.exists():
            text = info_path.read_text(encoding="utf-8").strip()
            yield Utterance(text=text, audio_path=wav_path)


# -----------------------------------------------------------------------------

if __name__ == "__main__":
    main()
