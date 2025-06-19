from typing import Dict, List
from .token_mapper import register

__all__ = ["get_japanese_id_map", "JAPANESE_PHONEMES", "SPECIAL_TOKENS"]

# -----------------------------------------------------------------------------
# Japanese phoneme inventory (Open JTalk style) + prosody/special tokens
# -----------------------------------------------------------------------------
# NOTE: This list purposely errs on the side of *including* more phonemes than
# may actually appear in the corpus so that we avoid "Missing phoneme" warnings
# at学習時.  If some of these tokens never appear they are simply unused.
# -----------------------------------------------------------------------------

# Prosody / sentence boundary tokens inserted by `phonemize_japanese`
SPECIAL_TOKENS: List[str] = [
    "_",  # short pause (pau)
    "^",  # BOS
    "$",  # EOS (declarative)
    "?",  # EOS (interrogative)
    "#",  # accent phrase boundary
    "[",  # rising pitch mark (accent phrase head)
    "]",  # falling pitch mark (accent nucleus)
]

# Core phoneme set – based on Open JTalk definitions and common practice in
# Japanese TTS front-ends (Tacotron, VITS, etc.)
# Long vowels (a:, i:, …) are kept as separate tokens.  Devoiced vowels
# (A, I, …) are converted to lower-case during phonemization, so we only need
# the lowercase versions here.
JAPANESE_PHONEMES: List[str] = [
    # vowels
    "a",
    "i",
    "u",
    "e",
    "o",
    "a:",
    "i:",
    "u:",
    "e:",
    "o:",
    # special consonant-centric phonemes
    "N",   # 撥音 (ん)
    "cl",  # 促音 / 終止閉鎖
    "q",   # 促音 (alternate label)
    # plosives + voiced counterparts
    "k", "ky",
    "kw",  # くゎ系歴史的仮名・方言
    "g", "gy",
    "gw",  # ぐゎ系
    "t", "ty",
    "d", "dy",
    "p", "py",
    "b", "by",
    # affricates, fricatives, etc.
    "ch", "ts",
    "s", "sh",
    "z", "j", "zy",
    "f", "h", "hy",
    "v",  # 外来音ヴ用
    # nasals / approximants
    "n", "ny",
    "m", "my",
    "r", "ry",
    "w", "y",
]


def get_japanese_id_map() -> Dict[str, List[int]]:
    """Return a mapping {symbol: [id]} suitable for Piper config.

    The first token (id=0) is always the pause "_" so that it functions as the
    padding symbol, mirroring the convention used in Piper's English mapping.
    """

    # 各トークンを1文字へ写像
    symbols: List[str] = [register(s) for s in (SPECIAL_TOKENS + JAPANESE_PHONEMES)]
    id_map: Dict[str, List[int]] = {}
    for idx, symbol in enumerate(symbols):
        id_map[symbol] = [idx]
    return id_map 