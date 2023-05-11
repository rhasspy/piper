#!/usr/bin/env python3
import argparse
import csv
import json
import sys
import statistics
from pathlib import Path

import librosa


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--dataset_dir", default=Path.cwd())
    parser.add_argument("--csv", action="store_true")
    args = parser.parse_args()

    dataset_dir = Path(args.dataset_dir)
    wav_dir = dataset_dir / "wav"
    if not wav_dir.is_dir():
        wav_dir = dataset_dir / "wavs"

    reader = csv.reader(sys.stdin, delimiter="|")
    writer = csv.writer(sys.stdout, delimiter="|")
    rates = []
    for row in reader:
        filename, text = row[0], row[-1]

        # Try file name relative to metadata
        wav_path = dataset_dir / filename

        if not wav_path.exists():
            # Try with .wav
            wav_path = dataset_dir / f"{filename}.wav"

        if not wav_path.exists():
            # Try wav/ or wavs/
            wav_path = wav_dir / filename

        if not wav_path.exists():
            # Try with .wav
            wav_path = wav_dir / f"{filename}.wav"

        if not wav_path.exists():
            print("Missing", wav_path, file=sys.stderr)
            continue

        if wav_path.stat().st_size == 0:
            print("Empty", wav_path, file=sys.stderr)
            continue

        duration = librosa.get_duration(path=wav_path)
        rate = duration / len(text)

        if args.csv:
            writer.writerow((filename, text, duration, rate))
        else:
            rates.append(rate)

    if not args.csv:
        json.dump(
            {
                "rates": rates,
                "mean": statistics.mean(rates),
                "median": statistics.median(rates),
            },
            sys.stdout,
        )


if __name__ == "__main__":
    main()
