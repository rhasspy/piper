#!/usr/bin/env python3
import csv
import json
import sys
import unicodedata
from collections import Counter

from .phonemize import DEFAULT_PHONEME_ID_MAP


def main() -> None:
    missing_phonemes: Counter[str] = Counter()

    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue

        utt = json.loads(line)
        for phoneme in utt["phonemes"]:
            if phoneme not in DEFAULT_PHONEME_ID_MAP:
                missing_phonemes[phoneme] += 1

    if missing_phonemes:
        print("Missing", len(missing_phonemes), "phoneme(s)", file=sys.stderr)
        writer = csv.writer(sys.stdout)
        for phoneme, count in missing_phonemes.most_common():
            hex_phoneme = hex(ord(phoneme))
            writer.writerow(
                (
                    phoneme,
                    unicodedata.category(phoneme),
                    unicodedata.name(phoneme),
                    f"\\u{hex_phoneme}",
                    count,
                )
            )


# -----------------------------------------------------------------------------

if __name__ == "__main__":
    main()
