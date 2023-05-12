import argparse
import json
import sys
import unicodedata
from collections import Counter
from enum import Enum
from typing import Dict, Iterable, List, Mapping, Optional

from espeak_phonemizer import Phonemizer


class PhonemeType(str, Enum):
    ESPEAK = "espeak"
    """Phonemes come from espeak-ng"""

    TEXT = "text"
    """Phonemes come from text itself"""


MAX_PHONEMES = 256
DEFAULT_PHONEME_ID_MAP: Dict[str, List[int]] = {
    "_": [0],
    "^": [1],
    "$": [2],
    " ": [3],
    "!": [4],
    "'": [5],
    "(": [6],
    ")": [7],
    ",": [8],
    "-": [9],
    ".": [10],
    ":": [11],
    ";": [12],
    "?": [13],
    "a": [14],
    "b": [15],
    "c": [16],
    "d": [17],
    "e": [18],
    "f": [19],
    "h": [20],
    "i": [21],
    "j": [22],
    "k": [23],
    "l": [24],
    "m": [25],
    "n": [26],
    "o": [27],
    "p": [28],
    "q": [29],
    "r": [30],
    "s": [31],
    "t": [32],
    "u": [33],
    "v": [34],
    "w": [35],
    "x": [36],
    "y": [37],
    "z": [38],
    "æ": [39],
    "ç": [40],
    "ð": [41],
    "ø": [42],
    "ħ": [43],
    "ŋ": [44],
    "œ": [45],
    "ǀ": [46],
    "ǁ": [47],
    "ǂ": [48],
    "ǃ": [49],
    "ɐ": [50],
    "ɑ": [51],
    "ɒ": [52],
    "ɓ": [53],
    "ɔ": [54],
    "ɕ": [55],
    "ɖ": [56],
    "ɗ": [57],
    "ɘ": [58],
    "ə": [59],
    "ɚ": [60],
    "ɛ": [61],
    "ɜ": [62],
    "ɞ": [63],
    "ɟ": [64],
    "ɠ": [65],
    "ɡ": [66],
    "ɢ": [67],
    "ɣ": [68],
    "ɤ": [69],
    "ɥ": [70],
    "ɦ": [71],
    "ɧ": [72],
    "ɨ": [73],
    "ɪ": [74],
    "ɫ": [75],
    "ɬ": [76],
    "ɭ": [77],
    "ɮ": [78],
    "ɯ": [79],
    "ɰ": [80],
    "ɱ": [81],
    "ɲ": [82],
    "ɳ": [83],
    "ɴ": [84],
    "ɵ": [85],
    "ɶ": [86],
    "ɸ": [87],
    "ɹ": [88],
    "ɺ": [89],
    "ɻ": [90],
    "ɽ": [91],
    "ɾ": [92],
    "ʀ": [93],
    "ʁ": [94],
    "ʂ": [95],
    "ʃ": [96],
    "ʄ": [97],
    "ʈ": [98],
    "ʉ": [99],
    "ʊ": [100],
    "ʋ": [101],
    "ʌ": [102],
    "ʍ": [103],
    "ʎ": [104],
    "ʏ": [105],
    "ʐ": [106],
    "ʑ": [107],
    "ʒ": [108],
    "ʔ": [109],
    "ʕ": [110],
    "ʘ": [111],
    "ʙ": [112],
    "ʛ": [113],
    "ʜ": [114],
    "ʝ": [115],
    "ʟ": [116],
    "ʡ": [117],
    "ʢ": [118],
    "ʲ": [119],
    "ˈ": [120],
    "ˌ": [121],
    "ː": [122],
    "ˑ": [123],
    "˞": [124],
    "β": [125],
    "θ": [126],
    "χ": [127],
    "ᵻ": [128],
    "ⱱ": [129],
    "0": [130],  # tones
    "1": [131],
    "2": [132],
    "3": [133],
    "4": [134],
    "5": [135],
    "6": [136],
    "7": [137],
    "8": [138],
    "9": [139],
    "\u0327": [140],  # combining cedilla
    "\u0303": [141],  # combining tilde
    "\u032a": [142],  # combining bridge below
    "\u032f": [143],  # combining inverted breve below
    "\u0329": [144],  # combining vertical line below
    "ʰ": [145],
    "ˤ": [146],
    "ε": [147],
    "↓": [148],
    "#": [149],  # Icelandic
    '"': [150],  # Russian
    "↑": [151],
}

ALPHABETS = {
    # Ukrainian
    "uk": {
        "_": [0],
        "^": [1],
        "$": [2],
        " ": [3],
        "!": [4],
        "'": [5],
        ",": [6],
        "-": [7],
        ".": [8],
        ":": [9],
        ";": [10],
        "?": [11],
        "а": [12],
        "б": [13],
        "в": [14],
        "г": [15],
        "ґ": [16],
        "д": [17],
        "е": [18],
        "є": [19],
        "ж": [20],
        "з": [21],
        "и": [22],
        "і": [23],
        "ї": [24],
        "й": [25],
        "к": [26],
        "л": [27],
        "м": [28],
        "н": [29],
        "о": [30],
        "п": [31],
        "р": [32],
        "с": [33],
        "т": [34],
        "у": [35],
        "ф": [36],
        "х": [37],
        "ц": [38],
        "ч": [39],
        "ш": [40],
        "щ": [41],
        "ь": [42],
        "ю": [43],
        "я": [44],
        "\u0301": [45],  # combining acute accent
        "\u0306": [46],  # combining breve
        "\u0308": [47],  # combining diaeresis
        "—": [48],       # em dash
    }
}


def phonemize(text: str, phonemizer: Phonemizer) -> List[str]:
    phonemes_str = phonemizer.phonemize(text=text, keep_clause_breakers=True)

    # Phonemes are decomposed into unicode codepoints
    return list(unicodedata.normalize("NFD", phonemes_str))


def phonemes_to_ids(
    phonemes: Iterable[str],
    phoneme_id_map: Optional[Mapping[str, Iterable[int]]] = None,
    missing_phonemes: "Optional[Counter[str]]" = None,
    pad: Optional[str] = "_",
    bos: Optional[str] = "^",
    eos: Optional[str] = "$",
) -> List[int]:
    if phoneme_id_map is None:
        phoneme_id_map = DEFAULT_PHONEME_ID_MAP

    phoneme_ids: List[int] = []

    if bos:
        phoneme_ids.extend(phoneme_id_map[bos])

    if pad:
        phoneme_ids.extend(phoneme_id_map[pad])

    for phoneme in phonemes:
        mapped_phoneme_ids = phoneme_id_map.get(phoneme)
        if mapped_phoneme_ids:
            phoneme_ids.extend(mapped_phoneme_ids)

            if pad:
                phoneme_ids.extend(phoneme_id_map[pad])
        elif missing_phonemes is not None:
            # Make note of missing phonemes
            missing_phonemes[phoneme] += 1

    if eos:
        phoneme_ids.extend(phoneme_id_map[eos])

    return phoneme_ids


# -----------------------------------------------------------------------------


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("language")
    args = parser.parse_args()

    phonemizer = Phonemizer(args.language)

    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue

        phonemes = phonemize(line, phonemizer)
        phoneme_ids = phonemes_to_ids(phonemes)
        json.dump(
            {
                "text": line,
                "phonemes": phonemes,
                "phoneme_ids": phoneme_ids,
            },
            sys.stdout,
            ensure_ascii=False,
        )
        print("")


if __name__ == "__main__":
    main()
