# 新規追加ファイル: 多文字音素→1文字(コードポイント) 変換を共通提供
TOKEN2CHAR = {}
CHAR2TOKEN = {}

# Private Use Area 開始位置
_PUA_START = 0xE000
_next = _PUA_START

def register(token: str) -> str:
    """Register *token* and return its single-codepoint replacement."""
    global _next
    if token in TOKEN2CHAR:
        return TOKEN2CHAR[token]

    # 既に1コードポイントの場合はそのまま流用
    if len(token) == 1:
        TOKEN2CHAR[token] = token
        CHAR2TOKEN[token] = token
        return token

    # 割り当て
    ch = chr(_next)
    _next += 1
    TOKEN2CHAR[token] = ch
    CHAR2TOKEN[ch] = token
    return ch


def map_sequence(seq):
    """seq は List[str]。各要素を1文字に置換したリストを返す"""
    return [register(t) for t in seq] 