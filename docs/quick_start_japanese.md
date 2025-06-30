# Piper 日本語音声合成 クイックスタート

## 1. ダウンロード

### macOS (Apple Silicon) の場合
```bash
# Piperをダウンロード
curl -L https://github.com/ayutaz/piper-plus/releases/latest/download/piper_macos_aarch64.tar.gz -o piper.tar.gz
tar -xzf piper.tar.gz

# セキュリティ警告を回避
xattr -cr piper/
```

### macOS (Intel) の場合
```bash
curl -L https://github.com/ayutaz/piper-plus/releases/latest/download/piper_macos_x64.tar.gz -o piper.tar.gz
tar -xzf piper.tar.gz
xattr -cr piper/
```

## 2. 日本語モデルの準備

日本語対応のモデル（.onnxファイル）が必要です。モデルには以下が必要：
- `.onnx` ファイル（モデル本体）
- `.onnx.json` ファイル（設定ファイル）

## 3. 実行

```bash
# シンプルな例
# espeak-ngのデータパスを設定
export ESPEAK_DATA_PATH="$(pwd)/piper/espeak-ng-data"
echo "こんにちは" | ./piper/bin/piper --model your_model.onnx --output_file hello.wav

# 再生（macOSの場合）
afplay hello.wav
```

## 4. 実用的な例

### テキストファイルから音声生成
```bash
# sample.txt を作成
cat > sample.txt << EOF
吾輩は猫である。
名前はまだ無い。
どこで生れたかとんと見当がつかぬ。
EOF

# 音声に変換
./piper/bin/piper --model your_model.onnx --output_file natsume.wav < sample.txt
```

### Pythonスクリプトから使用
```python
import subprocess

def tts(text, output_file):
    cmd = ["./piper/bin/piper", "--model", "your_model.onnx", "--output_file", output_file]
    subprocess.run(cmd, input=text, text=True)

# 使用例
tts("Python から音声合成ができます", "python_test.wav")
```

## トラブルシューティング

### 「開発元を検証できません」エラー（macOS）
```bash
xattr -d com.apple.quarantine piper/bin/piper
```

### OpenJTalk辞書が見つからないエラー
辞書は自動的に `piper/share/piper/openjtalk-dict/` に含まれています。
エラーが出る場合は、ファイルが正しく解凍されているか確認してください。

## 詳細情報

より詳しい情報は [日本語音声合成ガイド](JAPANESE_USAGE.md) を参照してください。