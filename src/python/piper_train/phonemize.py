import unicodedata
from collections import Counter
from typing import Dict, Iterable, List, Mapping, Optional

from espeak_phonemizer import Phonemizer

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
