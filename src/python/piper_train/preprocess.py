#!/usr/bin/env python3
import argparse
import csv
import dataclasses
import itertools
import json
import logging
import os
import sys
import signal

# import unicodedata  # noqa: F401 - May be used for text normalization
from collections import Counter
from dataclasses import dataclass, field
from enum import Enum
from multiprocessing import JoinableQueue, Process, Queue
from pathlib import Path
from typing import Dict, Iterable, List, Optional

# import pyopenjtalk  # noqa: F401 - Used in conditional imports
from tqdm import tqdm

from piper_phonemize import (
    phonemize_espeak,
    phonemize_codepoints,
    phoneme_ids_espeak,
    phoneme_ids_codepoints,
    get_codepoints_map,
    get_espeak_map,
    get_max_phonemes,
    tashkeel_run,
)

from .norm_audio import cache_norm_audio, make_silence_detector

# Custom Japanese phonemizer with accent/prosody marks
try:
    from .phonemize.japanese import phonemize_japanese  # type: ignore
except ImportError:
    # When running as script, relative import may fail; try absolute import fallback
    from piper_train.phonemize.japanese import phonemize_japanese  # type: ignore

# -----------------------------------------------------------------------------
# Japanese phoneme id map support
# -----------------------------------------------------------------------------
try:
    from .phonemize.jp_id_map import get_japanese_id_map  # type: ignore
except ImportError:
    from piper_train.phonemize.jp_id_map import get_japanese_id_map  # type: ignore

_DIR = Path(__file__).parent
_VERSION = (_DIR / "VERSION").read_text(encoding="utf-8").strip()
_LOGGER = logging.getLogger("preprocess")


class PhonemeType(str, Enum):
    ESPEAK = "espeak"
    """Phonemes come from espeak-ng"""

    TEXT = "text"
    """Phonemes come from text itself"""

    OPENJTALK = "openjtalk"
    """Phonemes come from pyopenjtalk for Japanese"""


def main() -> None:
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
        "--single-speaker", action="store_true", help="Force single speaker dataset"
    )
    parser.add_argument(
        "--speaker-id", type=int, help="Add speaker id to single speaker dataset"
    )
    #
    parser.add_argument(
        "--phoneme-type",
        choices=list(PhonemeType),
        default=PhonemeType.ESPEAK,
        help="Type of phonemes to use (default: espeak)",
    )
    parser.add_argument(
        "--text-casing",
        choices=("ignore", "lower", "upper", "casefold"),
        default="ignore",
        help="Casing applied to utterance text",
    )
    #
    parser.add_argument(
        "--dataset-name",
        help="Name of dataset to put in config (default: name of <ouput_dir>/../)",
    )
    parser.add_argument(
        "--audio-quality",
        help="Audio quality to put in config (default: name of <output_dir>)",
    )
    #
    parser.add_argument(
        "--tashkeel",
        action="store_true",
        help="Diacritize Arabic text with libtashkeel",
    )
    #
    parser.add_argument(
        "--skip-audio", action="store_true", help="Don't preprocess audio"
    )
    parser.add_argument(
        "--debug", action="store_true", help="Print DEBUG messages to the console"
    )
    parser.add_argument(
        "--timeout-seconds",
        type=int,
        default=60,
        help="Timeout in seconds for processing utterances",
    )
    args = parser.parse_args()

    if args.single_speaker and (args.speaker_id is not None):
        _LOGGER.fatal("--single-speaker and --speaker-id cannot both be provided")
        return

    level = logging.DEBUG if args.debug else logging.INFO
    logging.basicConfig(level=level)
    logging.getLogger().setLevel(level)

    # Prevent log spam
    logging.getLogger("numba").setLevel(logging.WARNING)

    # pyopenjtalkの警告メッセージを抑制（プログレスバーの表示を妨げないため）
    import warnings

    warnings.filterwarnings("ignore", category=UserWarning)

    # Ensure enum
    args.phoneme_type = PhonemeType(args.phoneme_type)

    # 日本語の場合は自動的に OPENJTALK を使用し、ID マップを設定
    japanese_id_map = None
    if args.language == "ja":
        args.phoneme_type = PhonemeType.OPENJTALK
        japanese_id_map = get_japanese_id_map()
        args.phoneme_id_map = japanese_id_map  # 子プロセスへ渡すため
        _LOGGER.info(
            "Using pyopenjtalk for Japanese phonemization (%s symbols)",
            len(japanese_id_map),
        )

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
    _LOGGER.debug("Counting number of speakers/utterances in the dataset")
    speaker_counts: "Counter[str]" = Counter()
    num_utterances = 0
    for utt in make_dataset(args):
        speaker = utt.speaker or ""
        speaker_counts[speaker] += 1
        num_utterances += 1

    assert num_utterances > 0, "No utterances found"

    is_multispeaker = len(speaker_counts) > 1
    speaker_ids: Dict[str, int] = {}

    if is_multispeaker:
        _LOGGER.info("%s speakers detected", len(speaker_counts))

        # Assign speaker ids by most number of utterances first
        for speaker_id, (speaker, _speaker_count) in enumerate(
            speaker_counts.most_common()
        ):
            speaker_ids[speaker] = speaker_id
    else:
        _LOGGER.info("Single speaker dataset")

    # Write config
    audio_quality = args.audio_quality or args.output_dir.name
    dataset_name = args.dataset_name or args.output_dir.parent.name

    with open(args.output_dir / "config.json", "w", encoding="utf-8") as config_file:
        json.dump(
            {
                "dataset": dataset_name,
                "audio": {
                    "sample_rate": args.sample_rate,
                    "quality": audio_quality,
                },
                "espeak": {
                    "voice": args.language,
                },
                "language": {
                    "code": args.language,
                },
                "inference": {"noise_scale": 0.667, "length_scale": 1, "noise_w": 0.8},
                "phoneme_type": args.phoneme_type.value,
                "phoneme_map": {},
                "phoneme_id_map": (
                    get_codepoints_map()[args.language]
                    if args.phoneme_type == PhonemeType.TEXT
                    else (
                        japanese_id_map
                        if japanese_id_map is not None
                        else get_espeak_map()
                    )
                ),
                "num_symbols": (
                    len(japanese_id_map)
                    if japanese_id_map is not None
                    else get_max_phonemes()
                ),
                "num_speakers": len(speaker_counts),
                "speaker_id_map": speaker_ids,
                "piper_version": _VERSION,
            },
            config_file,
            ensure_ascii=True,
            indent=4,
        )
    _LOGGER.info("Wrote dataset config")

    if (args.max_workers is None) or (args.max_workers < 1):
        args.max_workers = os.cpu_count()

    assert args.max_workers is not None

    batch_size = int(num_utterances / (args.max_workers * 2))
    queue_in: "Queue[Iterable[Utterance]]" = JoinableQueue()
    queue_out: "Queue[Optional[Utterance]]" = Queue()

    # Start workers
    if args.phoneme_type == PhonemeType.TEXT:
        target = phonemize_batch_text
    elif args.phoneme_type == PhonemeType.OPENJTALK:
        target = phonemize_batch_openjtalk
    else:
        target = phonemize_batch_espeak

    processes = [
        Process(target=target, args=(args, queue_in, queue_out))
        for _ in range(args.max_workers)
    ]
    for proc in processes:
        proc.start()

    _LOGGER.info(
        "Processing %s utterance(s) with %s worker(s)", num_utterances, args.max_workers
    )
    # プログレスバーを表示（stdoutに表示し、早めに初期化）
    print(
        f"Starting to process {num_utterances} utterances...",
        file=sys.stdout,
        flush=True,
    )
    pbar = tqdm(
        total=num_utterances,
        desc="Preprocessing",
        unit="utt",
        file=sys.stdout,
        leave=True,
        dynamic_ncols=True,
        ascii=True,
        disable=False,
    )

    output_dataset_path = args.output_dir / "dataset.jsonl"
    # 途中で処理が停止してもこれまでの結果を保持できるよう、追加(append)モードで開く
    with open(output_dataset_path, "a", encoding="utf-8") as dataset_file:
        for utt_batch in batched(
            make_dataset(args),
            batch_size,
        ):
            queue_in.put(utt_batch)

        _LOGGER.debug("Waiting for jobs to finish")
        missing_phonemes: "Counter[str]" = Counter()
        for _ in range(num_utterances):
            utt = queue_out.get()
            if utt is not None:
                if utt.speaker is not None:
                    utt.speaker_id = speaker_ids[utt.speaker]

                utt_dict = dataclasses.asdict(utt)
                utt_dict.pop("missing_phonemes")

                # JSONL
                json.dump(
                    utt_dict,
                    dataset_file,
                    ensure_ascii=True,
                    cls=PathEncoder,
                )
                print("", file=dataset_file)

                missing_phonemes.update(utt.missing_phonemes)

            # プログレスバーを最後に更新（処理済みかどうかに関わらず）
            pbar.update(1)
            if (pbar.n % 500) == 0:
                pbar.refresh()

            # データ損失を防ぐため、定期的にフラッシュする
            if (pbar.n % 100) == 0:
                dataset_file.flush()

        pbar.close()
        if missing_phonemes:
            for phoneme, count in missing_phonemes.most_common():
                _LOGGER.warning("Missing %s (%s)", phoneme, count)

            _LOGGER.warning("Missing %s phoneme(s)", len(missing_phonemes))

    # Signal workers to stop
    for proc in processes:
        queue_in.put(None)

    # Wait for workers to stop
    for proc in processes:
        proc.join(timeout=1)


# -----------------------------------------------------------------------------


def get_text_casing(casing: str):
    if casing == "lower":
        return str.lower

    if casing == "upper":
        return str.upper

    if casing == "casefold":
        return str.casefold

    return lambda s: s


def phonemize_batch_espeak(
    args: argparse.Namespace, queue_in: JoinableQueue, queue_out: Queue
):
    try:
        # Suppress C-level warnings from pyopenjtalk/OpenJTalk to keep output clean
        if not getattr(args, "debug", False):
            devnull_fd = os.open(os.devnull, os.O_RDWR)
            os.dup2(devnull_fd, 2)

        casing = get_text_casing(args.text_casing)
        silence_detector = make_silence_detector()

        # Timeout
        timeout_sec = getattr(args, "timeout_seconds", 0)

        def _timeout_handler(signum, frame):
            raise TimeoutError()

        if timeout_sec > 0:
            signal.signal(signal.SIGALRM, _timeout_handler)

        while True:
            utt_batch = queue_in.get()
            if utt_batch is None:
                break

            for utt in utt_batch:
                try:
                    if args.tashkeel:
                        utt.text = tashkeel_run(utt.text)

                    if timeout_sec > 0:
                        signal.alarm(timeout_sec)
                    _LOGGER.debug(utt)
                    all_phonemes = phonemize_espeak(casing(utt.text), args.language)

                    # Flatten
                    utt.phonemes = [
                        phoneme
                        for sentence_phonemes in all_phonemes
                        for phoneme in sentence_phonemes
                    ]
                    utt.phoneme_ids = phoneme_ids_espeak(
                        utt.phonemes,
                        missing_phonemes=utt.missing_phonemes,
                    )
                    if not args.skip_audio:
                        utt.audio_norm_path, utt.audio_spec_path = cache_norm_audio(
                            utt.audio_path,
                            args.cache_dir,
                            silence_detector,
                            args.sample_rate,
                        )
                    queue_out.put(utt)
                    if timeout_sec > 0:
                        signal.alarm(0)
                except TimeoutError:
                    _LOGGER.error("Skipping utterance due to timeout: %s", utt)
                    queue_out.put(None)
                except Exception:
                    _LOGGER.exception("Failed to process utterance: %s", utt)
                    queue_out.put(None)

            queue_in.task_done()
    except Exception:
        _LOGGER.exception("phonemize_batch_espeak")


def phonemize_batch_text(
    args: argparse.Namespace, queue_in: JoinableQueue, queue_out: Queue
):
    try:
        if not getattr(args, "debug", False):
            devnull_fd = os.open(os.devnull, os.O_RDWR)
            os.dup2(devnull_fd, 2)

        casing = get_text_casing(args.text_casing)
        silence_detector = make_silence_detector()

        timeout_sec = getattr(args, "timeout_seconds", 0)

        def _timeout_handler(signum, frame):
            raise TimeoutError()

        if timeout_sec > 0:
            signal.signal(signal.SIGALRM, _timeout_handler)

        while True:
            utt_batch = queue_in.get()
            if utt_batch is None:
                break

            for utt in utt_batch:
                try:
                    if args.tashkeel:
                        utt.text = tashkeel_run(utt.text)

                    if timeout_sec > 0:
                        signal.alarm(timeout_sec)
                    _LOGGER.debug(utt)
                    all_phonemes = phonemize_codepoints(casing(utt.text))
                    # Flatten
                    utt.phonemes = [
                        phoneme
                        for sentence_phonemes in all_phonemes
                        for phoneme in sentence_phonemes
                    ]
                    utt.phoneme_ids = phoneme_ids_codepoints(
                        args.language,
                        utt.phonemes,
                        missing_phonemes=utt.missing_phonemes,
                    )
                    if not args.skip_audio:
                        utt.audio_norm_path, utt.audio_spec_path = cache_norm_audio(
                            utt.audio_path,
                            args.cache_dir,
                            silence_detector,
                            args.sample_rate,
                        )
                    queue_out.put(utt)
                    if timeout_sec > 0:
                        signal.alarm(0)
                except TimeoutError:
                    _LOGGER.error("Skipping utterance due to timeout: %s", utt)
                    queue_out.put(None)
                except Exception:
                    _LOGGER.exception("Failed to process utterance: %s", utt)
                    queue_out.put(None)

            queue_in.task_done()
    except Exception:
        _LOGGER.exception("phonemize_batch_text")


def phonemize_batch_openjtalk(
    args: argparse.Namespace, queue_in: JoinableQueue, queue_out: Queue
):
    try:
        if not getattr(args, "debug", False):
            devnull_fd = os.open(os.devnull, os.O_RDWR)
            os.dup2(devnull_fd, 2)

        casing = get_text_casing(args.text_casing)
        silence_detector = make_silence_detector()

        timeout_sec = getattr(args, "timeout_seconds", 0)

        def _timeout_handler(signum, frame):
            raise TimeoutError()

        if timeout_sec > 0:
            signal.signal(signal.SIGALRM, _timeout_handler)

        while True:
            utt_batch = queue_in.get()
            if utt_batch is None:
                break

            for utt in utt_batch:
                try:
                    if timeout_sec > 0:
                        signal.alarm(timeout_sec)
                    _LOGGER.debug(utt)
                    # 高低アクセントを含む日本語 phonemizer
                    utt.phonemes = phonemize_japanese(casing(utt.text))
                    # phoneme_ids は phoneme_id_map から取得
                    utt.phoneme_ids = []
                    for phoneme in utt.phonemes:
                        if phoneme in args.phoneme_id_map:
                            utt.phoneme_ids.extend(args.phoneme_id_map[phoneme])
                        else:
                            utt.missing_phonemes[phoneme] += 1
                            _LOGGER.warning(f"Missing phoneme: {phoneme}")

                    if not args.skip_audio:
                        utt.audio_norm_path, utt.audio_spec_path = cache_norm_audio(
                            utt.audio_path,
                            args.cache_dir,
                            silence_detector,
                            args.sample_rate,
                        )
                    queue_out.put(utt)
                    if timeout_sec > 0:
                        signal.alarm(0)
                except TimeoutError:
                    _LOGGER.error("Skipping utterance due to timeout: %s", utt)
                    queue_out.put(None)
                except Exception:
                    _LOGGER.exception("Failed to process utterance: %s", utt)
                    queue_out.put(None)

            queue_in.task_done()
    except Exception:
        _LOGGER.exception("phonemize_batch_openjtalk")


# -----------------------------------------------------------------------------


@dataclass
class Utterance:
    text: str
    audio_path: Path
    speaker: Optional[str] = None
    speaker_id: Optional[int] = None
    phonemes: Optional[List[str]] = None
    phoneme_ids: Optional[List[int]] = None
    audio_norm_path: Optional[Path] = None
    audio_spec_path: Optional[Path] = None
    missing_phonemes: "Counter[str]" = field(default_factory=Counter)


class PathEncoder(json.JSONEncoder):
    def default(self, o):
        if isinstance(o, Path):
            return str(o)
        return super().default(o)


def ljspeech_dataset(args: argparse.Namespace) -> Iterable[Utterance]:
    dataset_dir = args.input_dir
    is_single_speaker = args.single_speaker
    speaker_id = args.speaker_id
    skip_audio = args.skip_audio

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
            assert len(row) >= 2, "Not enough columns"

            speaker: Optional[str] = None
            if is_single_speaker or (len(row) == 2):
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

            if not skip_audio:
                if not wav_path.exists():
                    _LOGGER.warning("Missing %s", filename)
                    continue

                if wav_path.stat().st_size == 0:
                    _LOGGER.warning("Empty file: %s", wav_path)
                    continue

            yield Utterance(
                text=text, audio_path=wav_path, speaker=speaker, speaker_id=speaker_id
            )


def mycroft_dataset(args: argparse.Namespace) -> Iterable[Utterance]:
    dataset_dir = args.input_dir
    is_single_speaker = args.single_speaker
    skip_audio = args.skip_audio

    speaker_id = 0
    for metadata_path in dataset_dir.glob("**/*-metadata.txt"):
        speaker = metadata_path.parent.name if not is_single_speaker else None
        with open(metadata_path, "r", encoding="utf-8") as csv_file:
            # filename|text|length
            reader = csv.reader(csv_file, delimiter="|")
            for row in reader:
                filename, text = row[0], row[1]
                wav_path = metadata_path.parent / filename
                if skip_audio or (wav_path.exists() and (wav_path.stat().st_size > 0)):
                    yield Utterance(
                        text=text,
                        audio_path=wav_path,
                        speaker=speaker,
                        speaker_id=speaker_id if not is_single_speaker else None,
                    )
        speaker_id += 1


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
