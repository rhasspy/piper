#!/usr/bin/env python3
import argparse
from pathlib import Path

import torch


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--cache-dir",
        required=True,
        help="Path to directory with audio/spectrogram files (*.pt)",
    )
    parser.add_argument(
        "--delete", action="store_true", help="Delete files that fail to load"
    )
    args = parser.parse_args()

    cache_dir = Path(args.cache_dir)
    num_deleted = 0
    for pt_path in cache_dir.glob("*.pt"):
        try:
            torch.load(str(pt_path))
        except Exception:
            print(pt_path)
            if args.delete:
                pt_path.unlink()
                num_deleted += 1

    print("Deleted:", num_deleted, "file(s)")


if __name__ == "__main__":
    main()
