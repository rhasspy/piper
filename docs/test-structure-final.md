# Piper テスト構造（最終版）

## 簡潔で実用的なテスト構造

### Python テスト
```
src/python/tests/
├── test_phonemize.py         # 音素変換の統合テスト（全言語対応）
├── test_token_mapper_impl.py # PUAマッピングの実装テスト
├── test_vits.py             # VITS必須ユーティリティのテスト
└── test_integration.py       # E2E統合テスト

src/python_run/tests/
├── test_runtime.py          # ランタイムユーティリティテスト
└── test_util_impl.py        # 音声変換ユーティリティテスト
```

### C++ テスト
```
src/cpp/tests/
├── test_phonemize.cpp       # 音素マッピングテスト
├── test_piper_core.cpp      # コア機能の基本テスト
└── CMakeLists.txt          # ビルド設定
```

## テストの特徴

### 1. 実装重視
- モックは最小限
- 実際の動作を検証
- 実用的なアサーション

### 2. 簡潔性
- 1ファイル1機能
- 重複を排除
- 必要十分なテストケース

### 3. CI/CD対応
- プラットフォーム互換
- オプション機能のスキップ
- 高速実行

## 削除したファイル（60%削減）

- TDDスキャフォールディング（6ファイル）
- 重複した音素変換テスト（4ファイル）  
- モック中心のテスト（4ファイル）
- 不要な学習/エクスポートテスト（2ファイル）
- 過度に複雑なC++テスト（4ファイル）

合計：20ファイル削除 → 10ファイルに集約

## テスト実行

```bash
# すべてのテスト
make test

# Pythonテストのみ
make test-python

# C++テストのみ（要ビルド）
make test-cpp

# 単体テストのみ
python run_tests.py --unit

# 統合テストを含む
python run_tests.py --integration
```

## CI/CD

GitHub Actionsで自動実行：
- Python 3.9, 3.10, 3.11
- Ubuntu, macOS
- 単体テスト → 統合テスト → 日本語テスト