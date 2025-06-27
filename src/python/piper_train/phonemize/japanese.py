import re
from typing import List

import pyopenjtalk
from .token_mapper import map_sequence

__all__ = ["phonemize_japanese"]

# Regular expressions reused many times
_RE_PHONEME = re.compile(r"-([^+]+)\+")
_RE_A1 = re.compile(r"/A:([\d-]+)\+")
_RE_A2 = re.compile(r"\+([0-9]+)\+")
_RE_A3 = re.compile(r"\+([0-9]+)/")


def _is_question(text: str) -> bool:
    """Return True if *text* ends with a Japanese/ASCII question mark."""
    return text.strip().endswith("?") or text.strip().endswith("？")


def phonemize_japanese(text: str) -> List[str]:
    """Convert *text* into a list of phoneme/prosody tokens that Piper can ingest.

    The algorithm follows the so-called "Kurihara method" that inserts the
    following extra symbols in the phoneme sequence:

    ^   : beginning of sentence
    $/?: end of sentence (choose ? for interrogative)
    _   : short pause (pau)
    #   : accent phrase boundary
    [   : rising-pitch mark (accent phrase head)
    ]   : falling-pitch mark (accent nucleus)

    Notes
    -----
    1. We rely on *pyopenjtalk.extract_fullcontext* to obtain full-context labels.
    2. For vowels that are labelled as devoiced ("A I U E O"), we convert them
       to their voiced counterparts ("a i u e o") so that they share the same
       embedding.
    3. "sil" at the beginning / end of the utterance is converted into ^ / $ or ?.
    """

    labels = pyopenjtalk.extract_fullcontext(text)
    tokens: List[str] = []

    for idx, label in enumerate(labels):
        m_ph = _RE_PHONEME.search(label)
        if not m_ph:
            # Should never happen – skip just in case
            continue
        phoneme = m_ph.group(1)

        # Beginning / end silence handling
        if phoneme == "sil":
            if idx == 0:
                tokens.append("^")
            elif idx == len(labels) - 1:
                tokens.append("?" if _is_question(text) else "$")
            # Skip adding ordinary phoneme for sil
            continue

        # Short pause
        if phoneme == "pau":
            tokens.append("_")
            continue

        # Devoiced vowels – convert to lower-case counterpart
        if phoneme in {"A", "I", "U", "E", "O"}:
            phoneme = phoneme.lower()

        tokens.append(phoneme)

        # ------------------------------------------------------------------
        # Prosody mark extraction – see Open JTalk label definition
        # ------------------------------------------------------------------
        # A1 : accented? 1 if accented mora else 0
        # A2 : position of current mora in the accent phrase (1-based)
        # A3 : number of mora in the accent phrase
        #
        # A2_next is needed to detect accent nucleus and phrase boundary.
        # ------------------------------------------------------------------
        m_a1 = _RE_A1.search(label)
        m_a2 = _RE_A2.search(label)
        m_a3 = _RE_A3.search(label)
        if not (m_a1 and m_a2 and m_a3):
            # Cannot get accent info – continue
            continue

        a1 = int(m_a1.group(1))
        a2 = int(m_a2.group(1))
        a3 = int(m_a3.group(1))

        # Look-ahead to next label to fetch a2_next
        if idx < len(labels) - 1:
            m_a2_next = _RE_A2.search(labels[idx + 1])
            a2_next = int(m_a2_next.group(1)) if m_a2_next else -1
        else:
            a2_next = -1

        # Insert accent nucleus mark "]" at the descending point.
        # Kurihara rule: a1==0 && a2_next == a2 + 1 (i.e., pitch goes from H to L)
        if (a1 == 0) and (a2_next == a2 + 1):
            tokens.append("]")

        # Insert accent phrase boundary "#" when current mora is last in phrase
        if (a2 == a3) and (a2_next == 1):
            tokens.append("#")

        # Insert rising mark "[" at phrase head (a2==1) when next mora is 2
        if (a2 == 1) and (a2_next == 2):
            tokens.append("[")

    # 多文字トークンを1コードポイントへ変換
    return map_sequence(tokens)
