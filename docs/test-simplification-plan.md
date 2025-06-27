# テスト簡素化計画

## 現状の問題点

1. **過剰なモック**: 実装ではなくモックをテストしている
2. **重複**: 同じ機能を複数ファイルでテスト
3. **実用性の欠如**: 実際の動作を検証していない

## 推奨されるテスト構造

### Python テスト
```
src/python/tests/
├── test_phonemize.py         # 音素変換の統合テスト
├── test_voice.py             # 音声合成の実装テスト
└── test_integration.py       # E2Eテスト

src/python_run/tests/
├── test_runtime.py           # ランタイムの実装テスト
└── test_integration.py       # 推論のE2Eテスト
```

### C++ テスト
```
src/cpp/tests/
├── test_piper.cpp            # コア機能テスト
├── test_phonemize.cpp        # 音素変換テスト
└── CMakeLists.txt           # ビルド設定
```

## 削除推奨ファイル

以下のファイルは重複または過剰なモックのため削除推奨：

1. `test_tdd_*.py` - TDD開発用のスキャフォールディング
2. `test_*_impl.py` のうちモックが多いもの
3. 重複した音素変換テスト

## 必要十分なテストの基準

### 1. 実装をテストする
```python
# ❌ 悪い例：モックをテスト
mock_voice = MagicMock()
mock_voice.synthesize.return_value = "mocked"

# ✅ 良い例：実装をテスト
voice = PiperVoice.load("model.onnx")
audio = voice.synthesize("Hello")
assert len(audio) > 0
```

### 2. 一つのテストは一つのことを検証
```python
# ❌ 悪い例：複数の検証
def test_everything():
    assert phonemize() works
    assert synthesis() works
    assert export() works

# ✅ 良い例：単一の検証
def test_phonemize_english():
    result = phonemize("Hello", "en-us")
    assert "h" in result
```

### 3. 実用的なアサーション
```python
# ❌ 悪い例：実装詳細に依存
assert result[0] == "^"
assert result[1] == "h"
assert result[2] == "ɛ"

# ✅ 良い例：動作を検証
assert len(result) > 0
assert all(isinstance(p, str) for p in result)
```

## CI/CD での実行可能性

### 必要な対応

1. **モデルファイル**: 小さなテスト用モデルを用意
2. **オプション依存**: `pytest.mark.requires_*` でスキップ
3. **プラットフォーム**: Linux/macOS/Windowsで動作確認

### GitHub Actions での実行

```yaml
- name: Run tests
  run: |
    # 基本的なユニットテスト
    pytest -m "not requires_model and not slow"
    
    # モデルが必要なテストは別ジョブで
    if [ -f test/models/test.onnx ]; then
      pytest -m requires_model
    fi
```

## まとめ

現在のテストは過剰に複雑で重複が多い。実装を直接テストし、シンプルで実用的なテストに整理することで、保守性と信頼性が向上する。