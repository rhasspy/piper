#!/usr/bin/env python3
import argparse
import csv
import dataclasses
import itertools
import json
import logging
import os
from dataclasses import dataclass
from multiprocessing import JoinableQueue, Process, Queue
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
    parser.add_argument(
        "--dataset-format", choices=("ljspeech", "mycroft"), required=True
    )
    parser.add_argument("--cache-dir", help="Directory to cache processed audio files")
    parser.add_argument("--max-workers", type=int)
    parser.add_argument(
        "--debug", action="store_true", help="Print DEBUG messages to the console"
    )
    args = parser.parse_args()

    level = logging.DEBUG if args.debug else logging.INFO
    logging.basicConfig(level=level)
    logging.getLogger().setLevel(level)

    # Prevent log spam
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

    if args.dataset_format == "mycroft":
        make_dataset = mycroft_dataset
    else:
        make_dataset = ljspeech_dataset

    # Count speakers
    _LOGGER.debug("Counting number of speakers in the dataset")
    speakers: Set[str] = set()
    num_utterances = 0
    for utt in make_dataset(args.input_dir):
        speakers.add(utt.speaker or "")
        num_utterances += 1

    assert num_utterances > 0, "No utterances found"

    is_multispeaker = len(speakers) > 1
    speaker_ids: Dict[str, int] = {}

    if is_multispeaker:
        _LOGGER.info("%s speakers detected", len(speakers))

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

    if (args.max_workers is None) or (args.max_workers < 1):
        args.max_workers = os.cpu_count()

    batch_size = int(num_utterances / (args.max_workers * 2))
    queue_in = JoinableQueue()
    queue_out = Queue()

    # Start workers
    processes = [
        Process(target=process_batch, args=(args, queue_in, queue_out))
        for _ in range(args.max_workers)
    ]
    for proc in processes:
        proc.start()

    _LOGGER.info(
        "Processing %s utterance(s) with %s worker(s)", num_utterances, args.max_workers
    )
    with open(args.output_dir / "dataset.jsonl", "w", encoding="utf-8") as dataset_file:
        for utt_batch in batched(make_dataset(args.input_dir), batch_size):
            queue_in.put(utt_batch)

        _LOGGER.debug("Waiting for jobs to finish")
        for _ in range(num_utterances):
            utt = queue_out.get()
            if utt is not None:
                # JSONL
                json.dump(
                    dataclasses.asdict(utt),
                    dataset_file,
                    ensure_ascii=False,
                    cls=PathEncoder,
                )
                print("", file=dataset_file)

    # Signal workers to stop
    for proc in processes:
        queue_in.put(None)

    # Wait for workers to stop
    for proc in processes:
        proc.join(timeout=1)


# -----------------------------------------------------------------------------


def process_batch(args: argparse.Namespace, queue_in: JoinableQueue, queue_out: Queue):
    try:
        silence_detector = make_silence_detector()
        phonemizer = Phonemizer(default_voice=args.language)

        while True:
            utt_batch = queue_in.get()
            if utt_batch is None:
                break

            for utt in utt_batch:
                try:
                    _LOGGER.debug(utt)
                    utt.phonemes = phonemize(utt.text, phonemizer)
                    utt.phoneme_ids = phonemes_to_ids(utt.phonemes)
                    utt.audio_norm_path, utt.audio_spec_path = cache_norm_audio(
                        utt.audio_path,
                        args.cache_dir,
                        silence_detector,
                        args.sample_rate,
                    )
                    queue_out.put(utt)
                except Exception:
                    _LOGGER.exception("Failed to process utterance: %s", utt)
                    queue_out.put(None)

            queue_in.task_done()
    except Exception:
        _LOGGER.exception("process_batch")


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


def ljspeech_dataset(dataset_dir: Path) -> Iterable[Utterance]:
    # filename|speaker|text
    # speaker is optional
    metadata_path = dataset_dir / "metadata.csv"
    assert metadata_path.exists(), f"Missing {metadata_path}"

    wav_dir = dataset_dir / "wav"
    if not wav_dir.is_dir():
        wav_dir = dataset_dir / "wavs"

    with open(metadata_path, "r", encoding="utf-8") as csv_file:
        reader = csv.reader(csv_file, delimiter="|")
        for row in reader:
            assert len(row) >= 2, "Not enough colums"

            speaker: Optional[str] = None
            if len(row) == 2:
                filename, text = row[0], row[-1]
            else:
                filename, speaker, text = row[0], row[1], row[-1]

            # Try file name relative to metadata
            wav_path = metadata_path.parent / filename

            if not wav_path.exists():
                # Try with .wav
                wav_path = metadata_path.parent / f"{filename}.wav"

            if not wav_path.exists():
                # Try wav/ or wavs/
                wav_path = wav_dir / filename

            if not wav_path.exists():
                # Try with .wav
                wav_path = wav_dir / f"{filename}.wav"

            if not wav_path.exists():
                _LOGGER.warning("Missing %s", filename)
                continue

            yield Utterance(text=text, audio_path=wav_path, speaker=speaker)


def mycroft_dataset(dataset_dir: Path) -> Iterable[Utterance]:
    for info_path in dataset_dir.glob("*.info"):
        wav_path = info_path.with_suffix(".wav")
        if wav_path.exists():
            text = info_path.read_text(encoding="utf-8").strip()
            yield Utterance(text=text, audio_path=wav_path)


# -----------------------------------------------------------------------------


def batched(iterable, n):
    "Batch data into lists of length n. The last batch may be shorter."
    # batched('ABCDEFG', 3) --> ABC DEF G
    if n < 1:
        raise ValueError("n must be at least one")
    it = iter(iterable)
    batch = list(itertools.islice(it, n))
    while batch:
        yield batch
        batch = list(itertools.islice(it, n))


# -----------------------------------------------------------------------------

if __name__ == "__main__":
    main()
