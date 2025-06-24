# Piper 日本語音声合成ガイド

このガイドでは、Piperを使用して日本語の音声合成を行う方法を説明します。

## 必要なもの

- macOS、Linux、またはWindows（※Windows版は現在開発中）
- 日本語対応のPiperモデル（.onnxファイル）

## インストール方法

### macOSの場合

1. 最新のリリースから、お使いのMacに対応したバイナリをダウンロードします：
   - Apple Silicon (M1/M2/M3)の場合: `piper_macos_aarch64.tar.gz`
   - Intel Macの場合: `piper_macos_x64.tar.gz`

   ```bash
   # Apple Siliconの例
   curl -L https://github.com/ayutaz/piper-plus/releases/latest/download/piper_macos_aarch64.tar.gz -o piper_macos_aarch64.tar.gz
   ```

2. ダウンロードしたファイルを解凍します：
   ```bash
   tar -xzf piper_macos_aarch64.tar.gz
   ```

3. 解凍されたディレクトリ構造を確認します：
   ```
   piper/
   ├── bin/
   │   └── piper          # 実行ファイル
   ├── lib/               # 必要なライブラリ
   ├── share/
   │   └── piper/
   │       └── openjtalk-dict/  # 日本語辞書
   └── その他のファイル
   ```

### Linuxの場合

1. 最新のリリースから、お使いのアーキテクチャに対応したバイナリをダウンロードします：
   - AMD64: `piper_linux_amd64.tar.gz`
   - ARM64: `piper_linux_arm64.tar.gz`

   ```bash
   # AMD64の例
   curl -L https://github.com/ayutaz/piper-plus/releases/latest/download/piper_linux_amd64.tar.gz -o piper_linux_amd64.tar.gz
   tar -xzf piper_linux_amd64.tar.gz
   ```

## 日本語モデルの入手

日本語の音声合成を行うには、OpenJTalk形式の音素を使用するモデルが必要です。

### モデルの要件

モデルの設定ファイル（.onnx.json）に以下の設定が必要です：
```json
{
  "phoneme_type": "openjtalk",
  "language": {
    "code": "ja"
  }
}
```

## 使用方法

### 重要：初回セットアップ

espeak-ngのデータパスを設定する必要があります：

```bash
# piperディレクトリに移動した後
export ESPEAK_DATA_PATH="$(pwd)/piper/espeak-ng-data"
```

または、シェルの設定ファイル（~/.bashrc や ~/.zshrc）に追加：
```bash
export ESPEAK_DATA_PATH="/path/to/piper/espeak-ng-data"
```

### 基本的な使い方

```bash
# 環境変数を設定（毎回必要）
export ESPEAK_DATA_PATH="$(pwd)/piper/espeak-ng-data"

# テキストファイルから音声を生成
./piper/bin/piper --model path/to/model.onnx --output_file output.wav < input.txt

# 直接テキストを入力
echo "こんにちは、世界" | ./piper/bin/piper --model path/to/model.onnx --output_file hello.wav

# 標準出力に音声データを出力（他のプログラムにパイプ）
echo "おはようございます" | ./piper/bin/piper --model path/to/model.onnx --output_raw | aplay -r 22050 -f S16_LE -t raw -
```

### コマンドラインオプション

- `--model <path>`: 使用するモデルファイル（.onnx）のパス
- `--output_file <path>`: 出力する音声ファイルのパス
- `--output_raw`: WAVヘッダーなしの生の音声データを出力
- `--debug`: デバッグ情報を表示

### 環境変数

辞書ファイルの場所をカスタマイズする場合：
```bash
export OPENJTALK_DICTIONARY_DIR=/path/to/custom/dictionary
./piper/bin/piper --model model.onnx --output_file output.wav < input.txt
```

## トラブルシューティング

### 「OpenJTalk: Failed to initialize」エラー

このエラーは辞書ファイルが見つからない場合に発生します。以下を確認してください：

1. `piper/share/piper/openjtalk-dict/`ディレクトリが存在するか
2. 環境変数`OPENJTALK_DICTIONARY_DIR`が正しく設定されているか

### 音声が生成されない

- モデルファイルが日本語対応（OpenJTalk形式）であることを確認
- 入力テキストがUTF-8エンコーディングであることを確認

### macOSでの「開発元が検証できない」警告

初回実行時にこの警告が表示される場合：
1. システム環境設定 → セキュリティとプライバシー
2. 「このまま開く」をクリック

または、ターミナルで以下を実行：
```bash
xattr -d com.apple.quarantine piper/bin/piper
```

## サンプルスクリプト

### バッチ処理スクリプト

```bash
#!/bin/bash
# batch_tts.sh - 複数のテキストファイルを音声に変換

MODEL_PATH="path/to/your/model.onnx"
PIPER_BIN="./piper/bin/piper"

for txt_file in *.txt; do
    base_name="${txt_file%.txt}"
    echo "処理中: $txt_file -> ${base_name}.wav"
    $PIPER_BIN --model "$MODEL_PATH" --output_file "${base_name}.wav" < "$txt_file"
done
```

### Pythonからの使用例

```python
import subprocess
import os

def text_to_speech(text, model_path, output_path):
    """テキストを音声ファイルに変換"""
    piper_bin = "./piper/bin/piper"
    
    # Piperを実行
    process = subprocess.Popen(
        [piper_bin, "--model", model_path, "--output_file", output_path],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    
    stdout, stderr = process.communicate(input=text)
    
    if process.returncode != 0:
        print(f"エラー: {stderr}")
        return False
    
    return True

# 使用例
text_to_speech(
    "本日は晴天なり",
    "path/to/model.onnx",
    "output.wav"
)
```

## 技術詳細

### OpenJTalkについて

PiperはOpenJTalkを使用して日本語テキストを音素に変換します。OpenJTalkは以下の処理を行います：

1. 形態素解析（MeCab）
2. 読み付与
3. アクセント処理
4. フルコンテキストラベル生成

### 音素について

OpenJTalkが生成する音素は、モデルの学習時に単一文字に変換されます。例：
- "ch" → "c", "h"
- "ts" → "t", "s"
- "sh" → "s", "h"

これにより、より柔軟な音素マッピングが可能になっています。

## ライセンス

- Piper: MIT License
- OpenJTalk: Modified BSD license
- naist-jdic: Modified BSD license

## モデルの互換性について

現在の実装では、既存のモデルをそのまま使用できますが、音素マッピングに関する警告が表示される場合があります。
詳細は[音素マッピングについて](PHONEME_MAPPING.md)を参照してください。

## 関連リンク

- [Piper公式リポジトリ](https://github.com/rhasspy/piper)
- [OpenJTalk](http://open-jtalk.sourceforge.net/)
- [日本語音声合成の詳細](README_JAPANESE.md)
- [音素マッピングについて](PHONEME_MAPPING.md)